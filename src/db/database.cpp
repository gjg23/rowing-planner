// src/db/database.cpp
#include <filesystem>

#include "db/database.hpp"

Database::Database(const std::string& path) {
    // Make database dir if not exists
    if (const auto dir = std::filesystem::path(path).parent_path(); !dir.empty())
        std::filesystem::create_directories(dir);

    constexpr int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    int rc = sqlite3_open_v2(path.c_str(), &db_, flags, nullptr);
    if (rc != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "out of memory";
        sqlite3_close(db_);
        throw std::runtime_error("cannot open db: " + msg);
    }
    exec("PRAGMA journal_mode=WAL; PRAGMA busy_timeout=5000; PRAGMA foreign_keys=ON;");
}

void Database::exec(const char* sql) {
    char* err = nullptr;
    if (sqlite3_exec(db_, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "sqlite error";
        sqlite3_free(err);
        throw std::runtime_error(msg);
    }
}

int Database::user_version() {
    Stmt q(db_, "PRAGMA user_version");
    q.step();
    return static_cast<int>(q.integer(0));
}

// ---------- events ----------

std::vector<Event> Database::events_between(std::int64_t user_id, std::string_view from, std::string_view to) {
    Stmt q(db_, "SELECT id,title,date,start_time,done FROM events "
                "WHERE user_id = ?1 AND date >= ?2 AND date < ?3 ORDER BY date, start_time");
    q.bind(1, user_id).bind(2, from).bind(3, to);

    std::vector<Event> out;
    while (q.step())
        out.push_back({q.integer(0), q.text(1), q.text(2), q.opt_text(3), q.integer(4) != 0});
    return out;
}

void Database::add_event(std::int64_t user_id, std::string_view title, std::string_view date) {
    Stmt q(db_, "INSERT INTO events (user_id, title, date) VALUES (?1, ?2, ?3)");
    q.bind(1, user_id).bind(2, title).bind(3, date);
    q.step();
}

// ---------- users ----------

std::int64_t Database::user_count() {
    Stmt q(db_, "SELECT COUNT(*) FROM users");
    q.step();
    return q.integer(0);
}

std::optional<std::int64_t> Database::create_user(std::string_view username, std::string_view password_hash) {
    Stmt q(db_, "INSERT INTO users (username, password_hash) VALUES (?1, ?2) "
                "ON CONFLICT(username) DO NOTHING RETURNING id");
    q.bind(1, username).bind(2, password_hash);
    if (!q.step()) return std::nullopt;
    return q.integer(0);
}

std::optional<UserAuth> Database::find_user(std::string_view username) {
    Stmt q(db_, "SELECT id, password_hash, failed_logins, locked_until FROM users WHERE username = ?1");
    q.bind(1, username);
    if (!q.step()) return std::nullopt;
    return UserAuth{q.integer(0), q.text(1), q.integer(2), q.integer(3)};
}

void Database::set_login_failures(std::int64_t user_id, std::int64_t failed, std::int64_t locked_until) {
    Stmt q(db_, "UPDATE users SET failed_logins = ?2, locked_until = ?3 WHERE id = ?1");
    q.bind(1, user_id).bind(2, failed).bind(3, locked_until);
    q.step();
}

// ---------- sessions ----------

void Database::create_session(std::string_view token_hash, std::int64_t user_id, std::int64_t expires_at) {
    Stmt q(db_, "INSERT INTO sessions (token_hash, user_id, expires_at) VALUES (?1, ?2, ?3)");
    q.bind(1, token_hash).bind(2, user_id).bind(3, expires_at);
    q.step();
}

std::optional<std::int64_t> Database::session_user(std::string_view token_hash, std::int64_t now) {
    Stmt q(db_, "SELECT user_id FROM sessions WHERE token_hash = ?1 AND expires_at > ?2");
    q.bind(1, token_hash).bind(2, now);
    if (!q.step()) return std::nullopt;
    return q.integer(0);
}

void Database::delete_session(std::string_view token_hash) {
    Stmt q(db_, "DELETE FROM sessions WHERE token_hash = ?1");
    q.bind(1, token_hash);
    q.step();
}

void Database::delete_expired_sessions(std::int64_t now) {
    Stmt q(db_, "DELETE FROM sessions WHERE expires_at <= ?1");
    q.bind(1, now);
    q.step();
}