// tests/calendar_test.cpp
#include <chrono>
#include <vector>

struct Cell {
    // Single calendar cell
    std::chrono::year_month_day date;
    bool in_month;
};

// Generate single month grid of cells
std::vector<Cell> month_grid(std::chrono::year y, std::chrono::month m) {
    std::chrono::sys_days first{std::chrono::year_month_day{y, m, std::chrono::day{1}}};
    std::chrono::days lead = std::chrono::weekday{first} - std::chrono::Monday;
    std::chrono::sys_days start = first - lead;

    std::vector<Cell> cells;
    cells.reserve(42);  // static allocation
    for (int i = 0; i < 42; ++i) {
        std::chrono::year_month_day d{start + std::chrono::days{i}};
        cells.push_back({d, d.month() == m});
    }
    return cells;
}