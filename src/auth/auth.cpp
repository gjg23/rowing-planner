// src/auth/auth.cpp
#include "auth/auth.hpp"

#include <sodium.h>

#include <cctype>
#include <chrono>
#include <stdexcept>

namespace auth {

namespace {
std::string to_hex(const unsigned char* data, std::size_t len) {
    std::string out(len * 2 + 1, '\0');
    sodium_bin2hex(out.data(), out.size(), data, len);
    out.pop_back();
    return out;
}
} // namespace

void init() {
    if (sodium_init() < 0) throw std::runtime_error("libsodium init failed");
}

std::string hash_password(std::string_view pw) {
    char out[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str(out, pw.data(), pw.size(),
                          crypto_pwhash_OPSLIMIT_INTERACTIVE,
                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
        throw std::runtime_error("password hashing failed (out of memory)");
    return out;
}

bool verify_password(const std::string& stored, std::string_view pw) {
    return crypto_pwhash_str_verify(stored.c_str(), pw.data(), pw.size()) == 0;
}

void dummy_verify(std::string_view pw) {
    static const std::string dummy = hash_password("not-a-real-password");
    verify_password(dummy, pw);
}

std::string new_session_token() {
    unsigned char buf[32];
    randombytes_buf(buf, sizeof buf);
    return to_hex(buf, sizeof buf);
}

std::string hash_token(std::string_view token) {
    unsigned char out[crypto_generichash_BYTES];
    crypto_generichash(out, sizeof out,
                       reinterpret_cast<const unsigned char*>(token.data()), token.size(),
                       nullptr, 0);
    return to_hex(out, sizeof out);
}

std::int64_t now() {
    using namespace std::chrono;
    return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

bool valid_username(std::string_view u) {
    if (u.empty() || u.size() > 32) return false;
    for (char c : u) {
        const auto uc = static_cast<unsigned char>(c);
        if (!std::isalnum(uc) && c != '_' && c != '-' && c != '.') return false;
    }
    return true;
}

bool valid_password(std::string_view p) {
    return p.size() >= 10 && p.size() <= 256;
}

} // namespace auth