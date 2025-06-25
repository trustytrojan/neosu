#pragma once

#include <string>

class UString;

struct MD5Hash {
    char hash[33];

    MD5Hash(const char *str);
    MD5Hash() { this->hash[0] = 0; }

    [[nodiscard]] inline const char *toUtf8() const { return this->hash; }
    bool operator==(const MD5Hash &other) const;
    bool operator==(const UString &other) const;
};

void trim(std::string *str);

namespace std {
template <>
struct hash<MD5Hash> {
    size_t operator()(const MD5Hash &md5) const {
        std::string_view tmp(md5.hash, 32);
        return std::hash<std::string_view>()(tmp);
    }
};
}  // namespace std
