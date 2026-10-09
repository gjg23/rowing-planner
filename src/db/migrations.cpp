// src/db/migrations.cpp
#include <format>
#include <iterator>

#include "db/database.hpp"

namespace {

// Append only. Never edit a migration that has shipped.
constexpr const char* kMigrations[] = {
    // 1
    R"sql(
        CREATE TABLE events (
            id          INTEGER PRIMARY KEY,
            title       TEXT    NOT NULL,
            date        TEXT    NOT NULL,
            start_time  TEXT,
            done        INTEGER NOT NULL DEFAULT 0
        );
        CREATE INDEX events_date ON events(date);
    )sql",
    
    // 2
    R"sql(
        CREATE TABLE users (
            id             INTEGER PRIMARY KEY,
            username       TEXT    NOT NULL UNIQUE COLLATE NOCASE,
            password_hash  TEXT    NOT NULL,
            failed_logins  INTEGER NOT NULL DEFAULT 0,
            locked_until   INTEGER NOT NULL DEFAULT 0,
            created_at     INTEGER NOT NULL DEFAULT (unixepoch())
        );
        CREATE TABLE sessions (
            token_hash  TEXT    PRIMARY KEY,
            user_id     INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
            expires_at  INTEGER NOT NULL
        );
        CREATE INDEX sessions_user ON sessions(user_id);

        ALTER TABLE events ADD COLUMN user_id INTEGER REFERENCES users(id) ON DELETE CASCADE;
        CREATE INDEX events_user_date ON events(user_id, date);
    )sql",
};

} // namespace

void run_migrations(Database& db) {
    const int current = db.user_version();
    const int target  = static_cast<int>(std::size(kMigrations));

    for (int v = current; v < target; ++v) {
        db.exec("BEGIN");
        try {
            db.exec(kMigrations[v]);
            db.exec(std::format("PRAGMA user_version = {}", v + 1).c_str());
            db.exec("COMMIT");
        } catch (...) {
            db.exec("ROLLBACK");
            throw;
        }
    }
}