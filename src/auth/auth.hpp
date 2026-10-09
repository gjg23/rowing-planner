// src/auth/auth.hpp
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace auth {

void init();

std::string hash_password(std::string_view password);
bool verify_password(const std::string& stored_hash, std::string_view password);
// Same cost as a real verify so unknown usernames can't be detected by timing.
void dummy_verify(std::string_view password);

std::string new_session_token();
std::string hash_token(std::string_view token);

std::int64_t now();

bool valid_username(std::string_view u);
bool valid_password(std::string_view p);

} // namespace auth