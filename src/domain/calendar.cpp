// src/domain/calendar.cpp
#include "domain/calendar.hpp"

#include <charconv>
#include <format>

namespace chr = std::chrono;

std::vector<Cell> month_grid(chr::year y, chr::month m) {
    chr::sys_days first{chr::year_month_day{y, m, chr::day{1}}};
    chr::days lead = chr::weekday{first} - chr::Monday;
    chr::sys_days start = first - lead;

    std::vector<Cell> cells;
    cells.reserve(42);
    for (int i = 0; i < 42; ++i) {
        chr::year_month_day d{start + chr::days{i}};
        cells.push_back({d, d.month() == m});
    }
    return cells;
}

std::string iso(chr::year_month_day d) {
    return std::format("{:04}-{:02}-{:02}",
                       int(d.year()), unsigned(d.month()), unsigned(d.day()));
}

bool valid_iso_date(std::string_view s) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;

    auto parse = [&](std::size_t pos, std::size_t len, unsigned& out) {
        const char* b = s.data() + pos;
        const char* e = b + len;
        auto [ptr, ec] = std::from_chars(b, e, out);
        return ec == std::errc{} && ptr == e;
    };

    unsigned y = 0, m = 0, d = 0;
    if (!parse(0, 4, y) || !parse(5, 2, m) || !parse(8, 2, d)) return false;

    return chr::year_month_day{chr::year{int(y)}, chr::month{m}, chr::day{d}}.ok();
}