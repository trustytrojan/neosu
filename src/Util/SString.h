#pragma once
#include "BaseEnvironment.h"
#include <algorithm>
#include <string>
#include <vector>

// non-UString-related fast and small string manipulation helpers

namespace SString {

// std string splitting, for if we don't want to create UStrings everywhere (slow and heavy)
static constexpr forceinline std::vector<std::string> split(const std::string& s, const std::string& d) {
    std::vector<std::string> r;
    size_t i = 0, j = 0;
    while((j = s.find(d, i)) != s.npos) r.emplace_back(s, i, j - i), i = j + d.size();
    r.emplace_back(s, i);
    return r;
};

// in-place whitespace trimming
static constexpr forceinline void trim(std::string* str) {
    str->erase(0, str->find_first_not_of(" \t\r\n"));
    str->erase(str->find_last_not_of(" \t\r\n") + 1);
}

static constexpr forceinline bool find_ncase(const std::string& haystack, const std::string& needle) {
    return !haystack.empty() && !std::ranges::search(haystack, needle, [](char ch1, char ch2) {
                return std::tolower(ch1) == std::tolower(ch2);
            }).empty();
}

static constexpr forceinline bool whitespace_only(const std::string& str) {
    return str.empty() || std::ranges::all_of(str, [](char c) { return std::isspace(c) != 0; });
}

}  // namespace SString
