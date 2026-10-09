// src/domain/event.hpp
#pragma once

#include <cstdint>
#include <string>
#include <optional>

struct Event {
    std::int64_t                id;
    std::string                 title;
    std::string                 date;       // "YYYY-MM-DD"
    std::optional<std::string>  start_time; // nullopt = all-day
    bool                        done;
};