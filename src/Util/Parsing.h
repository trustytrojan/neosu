#pragma once
// Copyright (c) 2025, kiwec, All rights reserved.
#include "types.h"
#include "SString.h"

namespace Parsing {

template <typename T>
const char* parse(const char* str, T* arg) {
    char* new_str = nullptr;

    if constexpr(std::is_same<T, f32>::value) {
        f32 f = strtof(str, &new_str);
        if(errno == ERANGE) return nullptr;
        *arg = f;
    } else if constexpr(std::is_same<T, f64>::value) {
        f64 d = strtod(str, &new_str);
        if(errno == ERANGE) return nullptr;
        *arg = d;
    } else if constexpr(std::is_same<T, i32>::value) {
        i32 l = strtol(str, &new_str, 10);
        if(errno == ERANGE) return nullptr;
        *arg = l;
    } else if constexpr(std::is_same<T, i64>::value) {
        i64 ll = strtoll(str, &new_str, 10);
        if(errno == ERANGE) return nullptr;
        *arg = ll;
    } else if constexpr(std::is_same<T, u32>::value) {
        u32 ul = strtoul(str, &new_str, 10);
        if(errno == ERANGE) return nullptr;
        *arg = ul;
    } else if constexpr(std::is_same<T, u64>::value) {
        u64 ull = strtoull(str, &new_str, 10);
        if(errno == ERANGE) return nullptr;
        *arg = ull;
    } else if constexpr(std::is_same<T, bool>::value) {
        i32 l = strtol(str, &new_str, 10);
        if(errno == ERANGE) return nullptr;
        *arg = (l > 0);  // 'bool's are just either 0 or 1
    } else if constexpr(std::is_same<T, u8>::value) {
        u32 b = strtoul(str, &new_str, 10);
        if(errno == ERANGE || b > 255) return nullptr;
        *arg = (u8)b;
    } else if constexpr(std::is_same<T, std::string>::value) {
        // Special handling for quoted string values
        if(*str == '"') {
            str++;

            std::string out;
            for(; *str != '\0' && *str != '"'; str++) {
                out.push_back(*str);
            }

            // Expected closing '"', got '\0' instead
            if(*str == '\0') return nullptr;

            *arg = out;
            str++;
            new_str = const_cast<char*>(str);
        } else {
            *arg = str;
            SString::trim(arg);
            new_str = const_cast<char*>(arg->c_str()) + arg->length();
        }
    } else {
        static_assert(Env::always_false_v<T>, "parsing for this type is not implemented");
        return nullptr;
    }

    return new_str;
}

// For parsing comma-separated values
template <typename T, typename... Extra>
const char* parse(const char* str, T* arg, Extra... extra) {
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

    *arg = arg_tmp;
    return str;
}

template <typename... Args>
bool parse_with_label(const char* str, const char* label, const char separator, Args... args) {
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
bool parse_setting(const std::string& str, const std::string& label, Args... args) {
    return parse_with_label(str.c_str(), label.c_str(), '=', args...);
}

// For parsing 'Foo: Bar' lines
template <typename... Args>
bool parse_value(const std::string& str, const std::string& label, Args... args) {
    return parse_with_label(str.c_str(), label.c_str(), ':', args...);
}

}  // namespace Parsing
