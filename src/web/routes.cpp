// src/web/routes.cpp
#include "web/routes.hpp"

#include <httplib.h>

#include <algorithm>
#include <chrono>
#include <format>
#include <functional>
#include <optional>
#include <string>

#include "auth/auth.hpp"
#include "config.hpp"
#include "db/database.hpp"
#include "domain/calendar.hpp"
#include "web/rate_limit.hpp"
#include "web/views.hpp"

namespace chr = std::chrono;
using httplib::Request;
using httplib::Response;

namespace {

constexpr std::int64_t kSessionSeconds = 30 * 24 * 3600;
constexpr std::string_view kCookie = "session";

RateLimiter auth_limiter(20, chr::minutes{10});

void html(Response& res, const std::string& body, int status = 200) {
    res.status = status;
    res.set_content(body, "text/html; charset=utf-8");
}

std::string cookie_value(const Request& req, std::string_view name) {
    const std::string header = req.get_header_value("Cookie");
    std::string_view rest = header;
    while (!rest.empty()) {
        const auto end = rest.find(';');
        auto pair = rest.substr(0, end);
        while (!pair.empty() && pair.front() == ' ') pair.remove_prefix(1);
        const auto eq = pair.find('=');
        if (eq != std::string_view::npos && pair.substr(0, eq) == name)
            return std::string(pair.substr(eq + 1));
        if (end == std::string_view::npos) break;
        rest.remove_prefix(end + 1);
    }
    return {};
}

void set_session_cookie(Response& res, std::string_view token, std::int64_t max_age) {
    res.set_header("Set-Cookie", std::format(
        "{}={}; Path=/; HttpOnly; Secure; SameSite=Lax; Max-Age={}", kCookie, token, max_age));
}

// Behind Caddy every request arrives from loopback; only then trust X-Forwarded-For.
std::string client_ip(const Request& req) {
    if (req.remote_addr == "127.0.0.1" || req.remote_addr == "::1") {
        const auto xff = req.get_header_value("X-Forwarded-For");
        if (!xff.empty()) {
            auto last = xff.substr(xff.rfind(',') + 1);
            last.erase(0, last.find_first_not_of(' '));
            return last;
        }
    }
    return req.remote_addr;
}

std::optional<std::int64_t> current_user(Database& db, const Request& req) {
    const auto token = cookie_value(req, kCookie);
    if (token.empty()) return std::nullopt;
    return db.session_user(auth::hash_token(token), auth::now());
}

void start_session(Database& db, Response& res, std::int64_t user_id) {
    const auto token = auth::new_session_token();
    db.create_session(auth::hash_token(token), user_id, auth::now() + kSessionSeconds);
    set_session_cookie(res, token, kSessionSeconds);
    res.set_redirect("/", 303);
}

bool signup_open(Database& db, const Config& cfg) {
    return cfg.allow_signup || db.user_count() == 0;
}

using AuthedHandler = std::function<void(const Request&, Response&, std::int64_t user_id)>;

httplib::Server::Handler authed(Database& db, AuthedHandler h) {
    return [&db, h = std::move(h)](const Request& req, Response& res) {
        const auto uid = current_user(db, req);
        if (!uid) { res.set_redirect("/login", 303); return; }
        h(req, res, *uid);
    };
}

} // namespace

void register_routes(httplib::Server& svr, Database& db, const Config& cfg) {
    // Reject cross-site form posts (CSRF).
    svr.set_pre_routing_handler([](const Request& req, Response& res) {
        if (req.method == "POST") {
            const auto origin = req.get_header_value("Origin");
            const auto host   = req.get_header_value("Host");
            if (!origin.empty() && origin != "https://" + host && origin != "http://" + host) {
                res.status = 403;
                return httplib::Server::HandlerResponse::Handled;
            }
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    svr.Get("/healthz", [](const Request&, Response& res) {
        res.set_content("ok", "text/plain");
    });

    // ---------- auth ----------

    svr.Get("/login", [](const Request&, Response& res) { html(res, views::login_page()); });

    svr.Post("/login", [&db](const Request& req, Response& res) {
        if (!auth_limiter.allow(client_ip(req))) {
            html(res, views::login_page("Too many attempts. Try again later."), 429);
            return;
        }
        const auto username = req.get_param_value("username");
        const auto password = req.get_param_value("password");
        const auto now = auth::now();
        const auto fail = [&] { html(res, views::login_page("Invalid username or password."), 401); };

        const auto user = db.find_user(username);
        if (!user) { auth::dummy_verify(password); fail(); return; }
        if (user->locked_until > now) { fail(); return; }

        if (!auth::verify_password(user->password_hash, password)) {
            const auto fails = user->failed_logins + 1;
            // After 5 misses: 30s, 60s, 120s ... capped at 15 min.
            const auto lock = fails >= 5
                ? now + std::min<std::int64_t>(900, 30LL << std::min<std::int64_t>(fails - 5, 5))
                : 0;
            db.set_login_failures(user->id, fails, lock);
            fail();
            return;
        }

        db.set_login_failures(user->id, 0, 0);
        db.delete_expired_sessions(now);
        start_session(db, res, user->id);
    });

    svr.Get("/register", [&db, &cfg](const Request&, Response& res) {
        if (!signup_open(db, cfg)) { res.status = 404; return; }
        html(res, views::register_page());
    });

    svr.Post("/register", [&db, &cfg](const Request& req, Response& res) {
        if (!signup_open(db, cfg)) { res.status = 404; return; }
        if (!auth_limiter.allow(client_ip(req))) {
            html(res, views::register_page("Too many attempts. Try again later."), 429);
            return;
        }
        const auto username = req.get_param_value("username");
        const auto password = req.get_param_value("password");
        const auto confirm  = req.get_param_value("confirm");

        if (!auth::valid_username(username))
            return html(res, views::register_page("Username: 1-32 letters, digits, _ - ."), 400);
        if (!auth::valid_password(password))
            return html(res, views::register_page("Password must be 10-256 characters."), 400);
        if (password != confirm)
            return html(res, views::register_page("Passwords do not match."), 400);

        const auto id = db.create_user(username, auth::hash_password(password));
        if (!id) return html(res, views::register_page("Username is taken."), 409);
        start_session(db, res, *id);
    });

    svr.Post("/logout", [&db](const Request& req, Response& res) {
        if (const auto token = cookie_value(req, kCookie); !token.empty())
            db.delete_session(auth::hash_token(token));
        set_session_cookie(res, "", 0);
        res.set_redirect("/login", 303);
    });

    // ---------- app (login required) ----------

    svr.Get("/", authed(db, [](const Request&, Response& res, std::int64_t) {
        const chr::year_month_day today{chr::floor<chr::days>(chr::system_clock::now())};
        res.set_redirect(std::format("/month/{}/{}", int(today.year()), unsigned(today.month())));
    }));

    svr.Get(R"(/month/(\d{4})/(\d{1,2}))", authed(db, [&db](const Request& req, Response& res, std::int64_t uid) {
        const int y = std::stoi(req.matches[1].str());
        const int m = std::stoi(req.matches[2].str());
        if (m < 1 || m > 12) { res.status = 404; return; }
        html(res, views::month_page(db, uid, y, m));
    }));

    svr.Post("/events", authed(db, [&db](const Request& req, Response& res, std::int64_t uid) {
        const auto title = req.get_param_value("title");
        const auto date  = req.get_param_value("date");
        if (title.empty() || title.size() > 200 || !valid_iso_date(date)) {
            res.status = 400;
            return;
        }
        db.add_event(uid, title, date);
        res.set_redirect("/month/" + date.substr(0, 4) + "/" + date.substr(5, 2), 303);
    }));
}