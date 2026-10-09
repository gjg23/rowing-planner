// src/web/views.cpp
#include "web/views.hpp"

#include <chrono>
#include <format>
#include <map>
#include <vector>

#include "db/database.hpp"
#include "domain/calendar.hpp"
#include "web/html.hpp"

namespace chr = std::chrono;

namespace views {

namespace {
std::string error_html(std::string_view error) {
    return error.empty() ? std::string{} : "<p class=\"error\">" + esc(error) + "</p>";
}
} // namespace

std::string layout(std::string_view title, std::string_view body) {
    return std::format(
        "<!doctype html>\n<html lang=\"en\"><head><meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
        "<title>{}</title><link rel=\"stylesheet\" href=\"/static/style.css\">"
        "</head><body>{}</body></html>",
        esc(title), body);
}

std::string login_page(std::string_view error) {
    return layout("Log in", "<h1>Log in</h1>" + error_html(error) + R"(
<form method="post" action="/login" class="auth">
  <label>Username <input name="username" autocomplete="username" required maxlength="32"></label>
  <label>Password <input name="password" type="password" autocomplete="current-password" required></label>
  <button>Log in</button>
</form>
<p><a href="/register">Create account</a></p>)");
}

std::string register_page(std::string_view error) {
    return layout("Create account", "<h1>Create account</h1>" + error_html(error) + R"(
<form method="post" action="/register" class="auth">
  <label>Username <input name="username" autocomplete="username" required maxlength="32"></label>
  <label>Password <input name="password" type="password" autocomplete="new-password" required minlength="10" maxlength="256"></label>
  <label>Confirm <input name="confirm" type="password" autocomplete="new-password" required></label>
  <button>Create account</button>
</form>
<p><a href="/login">Log in</a></p>)");
}
std::string month_page(Database& db, std::int64_t user_id, int year, int month) {
    const chr::year_month ym{chr::year{year}, chr::month{static_cast<unsigned>(month)}};
    const auto cells = month_grid(ym.year(), ym.month());

    // Query the whole visible grid so dimmed neighbour days show events too.
    const auto from = iso(cells.front().date);
    const auto to   = iso(chr::year_month_day{chr::sys_days{cells.back().date} + chr::days{1}});

    std::map<std::string, std::vector<Event>> by_day;
    for (auto& e : db.events_between(user_id, from, to))
        by_day[e.date].push_back(std::move(e));

    const auto prev = ym - chr::months{1};
    const auto next = ym + chr::months{1};

    std::string h;
    h += std::format(
        "<header><a href=\"/month/{}/{}\">&larr;</a>"
        "<h1>{:04}-{:02}</h1>"
        "<a href=\"/month/{}/{}\">&rarr;</a></header>",
        int(prev.year()), unsigned(prev.month()),
        year, month,
        int(next.year()), unsigned(next.month()));

    h += "<div class=\"grid\">";
    for (const char* wd : {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"})
        h += std::format("<div class=\"wd\">{}</div>", wd);

    for (const auto& c : cells) {
        h += std::format("<div class=\"cell{}\"><b>{}</b>",
                         c.in_month ? "" : " dim", unsigned(c.date.day()));
        for (const auto& e : by_day[iso(c.date)])
            h += "<p>" + esc(e.title) + "</p>";
        h += "</div>";
    }
    h += "</div>";

    h += std::format(
        "<form method=\"post\" action=\"/events\">"
        "<input name=\"title\" placeholder=\"Workout\" required maxlength=\"200\"> "
        "<input name=\"date\" type=\"date\" value=\"{:04}-{:02}-01\" required> "
        "<button>Add</button></form>",
        year, month);

    return layout(std::format("{:04}-{:02}", year, month), h);
}

} // namespace views