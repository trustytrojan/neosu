#pragma once

#include "UString.h"

struct MD5Hash final {
    std::array<char, 33> hash{};

    MD5Hash() = default;
    MD5Hash(const char *str) {
        strncpy(this->hash.data(), str, 32);
        this->hash[32] = '\0';
    }

    inline void clear() { hash.fill((char)0); }
    [[nodiscard]] inline size_t length() const { return strnlen(this->hash.data(), 32); }
    bool operator==(const MD5Hash &other) const { return memcmp(this->hash.data(), other.hash.data(), 32) == 0; }
    bool operator==(const UString &other) const { return strncmp(this->hash.data(), other.toUtf8(), 32) == 0; }
};

namespace std {
template <>
struct hash<MD5Hash> {
    size_t operator()(const MD5Hash &md5) const { return std::hash<std::string_view>()({md5.hash.data(), 32}); }
};
}  // namespace std
