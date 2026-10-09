// src/domain/calendar.hpp
#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

struct Cell {
    std::chrono::year_month_day date;
    bool in_month;
};

// 6x7 grid starting on the Monday on/before the 1st.
std::vector<Cell> month_grid(std::chrono::year y, std::chrono::month m);

// "YYYY-MM-DD"
std::string iso(std::chrono::year_month_day d);

bool valid_iso_date(std::string_view s);