#pragma once
#include "BaseEnvironment.h"
#include <algorithm>
#include <string>
#include <vector>
#include <cassert>

// non-UString-related fast and small string manipulation helpers

namespace SString {

// alphanumeric string comparator that ignores special characters at the start of strings
inline bool alnum_comp(const std::string& a, const std::string& b) {
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
inline std::vector<std::string> split(const std::string& s, const std::string_view& d) {
    std::vector<std::string> r;
    size_t i = 0, j = 0;
    while((j = s.find(d, i)) != s.npos) r.emplace_back(s, i, j - i), i = j + d.size();
    r.emplace_back(s, i);
    return r;
};

// in-place whitespace trimming
inline void trim(std::string* str) {
    assert(str &&
           "null string passed to SString::trim()");  // since asserts are disabled in release builds, this is
                                                      // harmless to add a reason for a null deref crash in debug builds
    if(str->empty()) return;
    str->erase(0, str->find_first_not_of(" \t\r\n"));
    str->erase(str->find_last_not_of(" \t\r\n") + 1);
}

inline bool contains_ncase(const std::string& haystack, const std::string& needle) {
    return !haystack.empty() && !std::ranges::search(haystack, needle, [](char ch1, char ch2) {
                                     return std::tolower(ch1) == std::tolower(ch2);
                                 }).empty();
}

inline bool whitespace_only(const std::string& str) {
    return str.empty() || std::ranges::all_of(str, [](char c) { return std::isspace(c) != 0; });
}

inline void to_lower(std::string& str) {
    if(str.empty()) return;
    std::ranges::transform(str, str.begin(), [](char c) { return std::tolower(c); });
}

inline std::string lower(const std::string& str) {
    if(str.empty()) return str;
    auto lstr{str};
    to_lower(lstr);
    return lstr;
}

}  // namespace SString
