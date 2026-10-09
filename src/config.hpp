// src/config.hpp
#pragma once

#include <cstdlib>
#include <string>
#include <string_view>

struct Config {
    std::string db_path         = "database/rowtracker.db";
    std::string bind            = "127.0.0.1";
    int         port            = 8080;
    std::string static_dir      = "./static";
    bool        allow_signup    = false;

    static Config from_env() {
        Config c;
        if (const char* v = std::getenv("ROWTRACK_DB"))             c.db_path      = v;
        if (const char* v = std::getenv("ROWTRACK_BIND"))           c.bind         = v;
        if (const char* v = std::getenv("ROWTRACK_PORT"))           c.port         = std::stoi(v);
        if (const char* v = std::getenv("ROWTRACK_STATIC"))         c.static_dir   = v;
        if (const char* v = std::getenv("ROWTRACK_ALLOW_SIGNUP"))   c.allow_signup = std::string_view(v) == "1";
        return c;
    }
};