// src/web/html.hpp
#pragma once

#include <string>
#include <string_view>

inline std::string esc(std::string_view s) {
    std::string o; o.reserve(s.size());
    for (char c : s) switch (c) {
        case '&': o += "&amp;";  break;
        case '<': o += "&lt;";   break;
        case '>': o += "&gt;";   break;
        case '"': o += "&quot;"; break;
        case '\'': o += "&#39;"; break;
        default:  o += c;
    }
    return o;
}