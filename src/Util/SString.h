#pragma once
#include "BaseEnvironment.h"
#include <algorithm>
#include <string>
#include <vector>

// non-UString-related fast and small string manipulation helpers

namespace SString {

// Since strtok_r SUCKS I'll just make my own
// Returns the token start, and edits str to after the token end (unless '\0').
static inline char* strtok_x(char d, char** str) {
    char* old = *str;
    while(**str != '\0' && **str != d) {
        (*str)++;
    }
    if(**str != '\0') {
        **str = '\0';
        (*str)++;
    }
    return old;
}

// alphanumeric string comparator that ignores special characters at the start of strings
static constexpr bool alnum_comp(const std::string& a, const std::string& b) {
    int i = 0;
    int j = 0;
    while(i < a.length() && j < b.length()) {
        if(!isalnum((uint8_t)a[i])) {
            i++;
            continue;
        }
        if(!isalnum((uint8_t)b[j])) {
            j++;
            continue;
        }
        auto la = tolower(a[i]);
        auto lb = tolower(b[j]);
        if(la != lb) return la < lb;
        i++;
        j++;
    }
    return false;
}

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

static constexpr forceinline bool contains_ncase(const std::string& haystack, const std::string& needle) {
    return !haystack.empty() && !std::ranges::search(haystack, needle, [](char ch1, char ch2) {
                                     return std::tolower(ch1) == std::tolower(ch2);
                                 }).empty();
}

static constexpr forceinline bool whitespace_only(const std::string& str) {
    return str.empty() || std::ranges::all_of(str, [](char c) { return std::isspace(c) != 0; });
}

static constexpr forceinline void to_lower(std::string& str) {
    if(str.empty()) return;
    std::ranges::transform(str, str.begin(), [](char c) { return std::tolower(c); });
}

static constexpr forceinline std::string lower(const std::string& str) {
    if(str.empty()) return str;
    auto lstr{str};
    to_lower(lstr);
    return lstr;
}

}  // namespace SString
