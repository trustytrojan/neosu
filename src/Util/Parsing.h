#pragma once
// Copyright (c) 2025, kiwec, All rights reserved.
#include <cerrno>   // errno
#include <cstdlib>  // strto*
#include <cstring>  // strlen
#include <limits>

#include "types.h"
#include "SString.h"

namespace Parsing {

// Since strtok_r SUCKS I'll just make my own
// Returns the token start, and edits str to after the token end (unless '\0').
inline char* strtok_x(char d, char** str) {
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

template <typename T>
const char* parse(const char* str, T* arg) {
    char* end_ptr = nullptr;
    errno = 0;  // strto* doesn't reset errno

    if constexpr(std::is_same_v<T, f32>) {
        f32 f = strtof(str, &end_ptr);
        if(end_ptr == str) return nullptr;  // end_ptr not moving means nothing was parsed
        if(errno == ERANGE) return nullptr;
        *arg = f;
    } else if constexpr(std::is_same_v<T, f64>) {
        f64 d = strtod(str, &end_ptr);
        if(end_ptr == str) return nullptr;
        if(errno == ERANGE) return nullptr;
        *arg = d;
    } else if constexpr(std::is_same_v<T, i32>) {
        long l = strtol(str, &end_ptr, 10);
        if(end_ptr == str) return nullptr;
        if(errno == ERANGE) return nullptr;
        // LP64 (64-bit long) wouldn't result in ERANGE
        if(l < std::numeric_limits<i32>::min() || l > std::numeric_limits<i32>::max()) return nullptr;
        *arg = static_cast<i32>(l);
    } else if constexpr(std::is_same_v<T, i64>) {
        i64 ll = strtoll(str, &end_ptr, 10);
        if(end_ptr == str) return nullptr;
        if(errno == ERANGE) return nullptr;
        *arg = ll;
    } else if constexpr(std::is_same_v<T, u32>) {
        unsigned long ul = strtoul(str, &end_ptr, 10);
        if(end_ptr == str) return nullptr;
        if(errno == ERANGE) return nullptr;
        // more LP64 handling
        if(ul > std::numeric_limits<u32>::max()) return nullptr;
        *arg = static_cast<u32>(ul);
    } else if constexpr(std::is_same_v<T, u64>) {
        u64 ull = strtoull(str, &end_ptr, 10);
        if(end_ptr == str) return nullptr;
        if(errno == ERANGE) return nullptr;
        *arg = ull;
    } else if constexpr(std::is_same_v<T, bool>) {
        long l = strtol(str, &end_ptr, 10);
        if(end_ptr == str) return nullptr;
        if(errno == ERANGE) return nullptr;
        *arg = (l > 0);  // 'bool's are just either 0 or 1
    } else if constexpr(std::is_same_v<T, u8>) {
        unsigned long b = strtoul(str, &end_ptr, 10);
        if(end_ptr == str) return nullptr;
        if(errno == ERANGE || b > 255) return nullptr;
        *arg = static_cast<u8>(b);
    } else if constexpr(std::is_same_v<T, std::string>) {
        // Special handling for quoted string values
        if(*str == '"') {
            str++;

            const char* start = str;
            while(*str != '\0' && *str != '"') {
                str++;
            }

            // Expected closing '"', got '\0' instead
            if(*str == '\0') return nullptr;

            *arg = std::string(start, str - start);
            str++;
            end_ptr = const_cast<char*>(str);
        } else {
            *arg = str;
            SString::trim(arg);
            end_ptr = const_cast<char*>(str + strlen(str));  // one-past-the-end
        }
    } else {
        static_assert(Env::always_false_v<T>, "parsing for this type is not implemented");
        return nullptr;
    }

    return end_ptr;
}

// For parsing comma-separated values
template <typename T, typename... Extra>
const char* parse(const char* str, T* arg, Extra*... extra) {
    // Storing result in tmp var, so we only modify *arg once parsing fully succeeded
    T arg_tmp;
    str = parse(str, &arg_tmp);
    if(str == nullptr) return nullptr;

    while((*str == ' ') || (*str == '\t')) str++;
    if(*str != ',') return nullptr;
    str++;
    while((*str == ' ') || (*str == '\t')) str++;

    str = parse(str, extra...);
    if(str == nullptr) return nullptr;

    *arg = std::move(arg_tmp);
    return str;
}

template <typename... Args>
bool parse_with_label(const char* str, const char* label, const char separator, Args*... args) {
    while((*str == ' ') || (*str == '\t')) str++;

    auto label_len = strlen(label);
    if(strncmp(str, label, label_len) != 0) return false;
    str += label_len;

    while((*str == ' ') || (*str == '\t')) str++;
    if(*str != separator) return false;
    str++;

    while((*str == ' ') || (*str == '\t')) str++;
    return parse(str, args...) != nullptr;
}

// For parsing 'Foo = Bar' lines
template <typename... Args>
bool parse_setting(const std::string& str, const std::string& label, Args*... args) {
    return parse_with_label(str.c_str(), label.c_str(), '=', args...);
}

// For parsing 'Foo: Bar' lines
template <typename... Args>
bool parse_value(const std::string& str, const std::string& label, Args*... args) {
    return parse_with_label(str.c_str(), label.c_str(), ':', args...);
}

template <typename First, typename... Rest>
bool parse_with_label_numbered(const char* str, const char* label, const char separator, First* first, Rest*... rest) {
    while((*str == ' ') || (*str == '\t')) str++;

    auto label_len = strlen(label);
    if(strncmp(str, label, label_len) != 0) return false;
    str += label_len;

    // "number" part
    while((*str == ' ') || (*str == '\t')) str++;
    str = parse(str, first);
    if(!str) return false;

    while((*str == ' ') || (*str == '\t')) str++;
    if(*str != separator) return false;
    str++;

    while((*str == ' ') || (*str == '\t')) str++;
    return parse(str, rest...) != nullptr;
}

// For parsing 'Foo# = Bar' lines (not currently used)
template <typename... Args>
bool parse_numbered_setting(const std::string& str, const std::string& label, Args*... args) {
    return parse_with_label_numbered(str.c_str(), label.c_str(), '=', args...);
}

// For parsing 'Foo#: Bar' lines
template <typename... Args>
bool parse_numbered_value(const std::string& str, const std::string& label, Args*... args) {
    return parse_with_label_numbered(str.c_str(), label.c_str(), ':', args...);
}

}  // namespace Parsing
