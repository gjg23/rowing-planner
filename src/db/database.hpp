// src/db/database.hpp
#pragma once

#include <sqlite3.h>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "domain/event.hpp"

class Stmt {
public:
    Stmt(sqlite3* db, const char* sql) {
        if (sqlite3_prepare_v2(db, sql, -1, &s_, nullptr) != SQLITE_OK)
            throw std::runtime_error(sqlite3_errmsg(db));
    }
    ~Stmt() { sqlite3_finalize(s_); }
    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;

    Stmt& bind(int i, std::string_view v) {
        sqlite3_bind_text(s_, i, v.data(), (int)v.size(), SQLITE_TRANSIENT);
        return *this;
    }
    Stmt& bind(int i, std::int64_t v) {
        sqlite3_bind_int64(s_, i, v);
        return *this;
    }
    Stmt& bind_null(int i) { sqlite3_bind_null(s_, i); return *this; }

    bool step() {
        int rc = sqlite3_step(s_);
        if (rc == SQLITE_ROW) return true;
        if (rc == SQLITE_DONE) return false;
        throw std::runtime_error("sqlite step failed");
    }

    std::int64_t integer(int col) const { return sqlite3_column_int64(s_, col); }
    std::string text(int col) const {
        auto p = sqlite3_column_text(s_, col);
        return p ? reinterpret_cast<const char*>(p) : "";
    }
    std::optional<std::string> opt_text(int col) const {
        if (is_null(col)) return std::nullopt;
        return text(col);
    }
    bool is_null(int col) const {
        return sqlite3_column_type(s_, col) == SQLITE_NULL;
    }
private:
    sqlite3_stmt* s_ = nullptr;
};

struct UserAuth {
    std::int64_t id;
    std::string  password_hash;
    std::int64_t failed_logins;
    std::int64_t locked_until;
};

class Database {
public:
    explicit Database(const std::string& path);
    ~Database() { sqlite3_close(db_); }
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    void exec(const char* sql);
    int user_version();

    // events
    std::vector<Event> events_between(std::int64_t user_id, std::string_view from, std::string_view to);
    void add_event(std::int64_t user_id, std::string_view title, std::string_view date);

    // users
    std::int64_t user_count();
    std::optional<std::int64_t> create_user(std::string_view username, std::string_view password_hash);
    std::optional<UserAuth> find_user(std::string_view username);
    void set_login_failures(std::int64_t user_id, std::int64_t failed, std::int64_t locked_until);

    // sessions
    void create_session(std::string_view token_hash, std::int64_t user_id, std::int64_t expires_at);
    std::optional<std::int64_t> session_user(std::string_view token_hash, std::int64_t now);
    void delete_session(std::string_view token_hash);
    void delete_expired_sessions(std::int64_t now);

private:
    sqlite3* db_ = nullptr;
};

void run_migrations(Database& db);