// src/web/rate_limit.hpp
#pragma once

#include <chrono>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>

// Sliding window: at most `max` hits per `window` per key.
class RateLimiter {
public:
    RateLimiter(std::size_t max, std::chrono::seconds window) : max_(max), window_(window) {}

    bool allow(const std::string& key) {
        const auto now = std::chrono::steady_clock::now();
        std::lock_guard lock(mu_);
        if (hits_.size() > 10'000) hits_.clear();  // crude memory bound
        auto& q = hits_[key];
        while (!q.empty() && now - q.front() > window_) q.pop_front();
        if (q.size() >= max_) return false;
        q.push_back(now);
        return true;
    }

private:
    std::size_t max_;
    std::chrono::seconds window_;
    std::mutex mu_;
    std::unordered_map<std::string, std::deque<std::chrono::steady_clock::time_point>> hits_;
};