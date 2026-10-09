// src/main.cpp
#include <httplib.h>

#include <cstdio>
#include <exception>

#include "auth/auth.hpp"
#include "config.hpp"
#include "db/database.hpp"
#include "web/routes.hpp"

int main() {
    try {
        auth::init();
        const Config cfg = Config::from_env();
        Database db(cfg.db_path);
        run_migrations(db);

        httplib::Server svr;
        svr.new_task_queue = [] { return new httplib::ThreadPool(4); };
        if (!svr.set_mount_point("/static", cfg.static_dir))
            std::fprintf(stderr, "warning: static dir '%s' not found\n", cfg.static_dir.c_str());
        svr.set_payload_max_length(64 * 1024);
        register_routes(svr, db, cfg);

        std::printf("listening on http://%s:%d\n", cfg.bind.c_str(), cfg.port);
        std::fflush(stdout);
        if (!svr.listen(cfg.bind, cfg.port)) {
            std::fprintf(stderr, "failed to bind %s:%d\n", cfg.bind.c_str(), cfg.port);
            return 1;
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "fatal: %s\n", e.what());
        return 1;
    }
}