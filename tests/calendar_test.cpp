// tests/calendar_test.cpp
#include <cstdio>

#include "domain/calendar.hpp"

#define CHECK(x) do { if (!(x)) { \
    std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #x); \
    return 1; } } while (0)

using namespace std::chrono;

int main() {
    // Oct 2026 starts on a Thursday; grid should start Mon Sep 28.
    auto cells = month_grid(year{2026}, October);
    CHECK(cells.size() == 42);
    CHECK(cells[0].date == 2026y / September / 28d);
    CHECK(!cells[0].in_month);
    CHECK(cells[3].date == 2026y / October / 1d);
    CHECK(cells[3].in_month);

    CHECK(iso(2026y / March / 5d) == "2026-03-05");

    CHECK(valid_iso_date("2024-02-29"));
    CHECK(!valid_iso_date("2026-02-29"));
    CHECK(!valid_iso_date("2026-1-01"));
    CHECK(!valid_iso_date("abcd-ef-gh"));
    CHECK(!valid_iso_date("2026-13-01"));

    std::puts("all calendar tests passed");
}