// src/web/views.hpp
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

class Database;

namespace views {
std::string layout(std::string_view title, std::string_view body);
std::string login_page(std::string_view error = {});
std::string register_page(std::string_view error = {});
std::string month_page(Database& db, std::int64_t user_id, int year, int month);
}