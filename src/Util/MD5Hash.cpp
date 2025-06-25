#include "MD5Hash.h"
#include "UString.h"

void trim(std::string *str) {
    while(!str->empty() && str->front() == ' ') {
        str->erase(0, 1);
    }
    while(!str->empty() && (str->back() == ' ' || str->back() == '\r')) {
        str->pop_back();
    }
}

MD5Hash::MD5Hash(const char *str) {
    strncpy(this->hash, str, 32);
    this->hash[32] = 0;
}

bool MD5Hash::operator==(const MD5Hash &other) const { return memcmp(this->hash, other.hash, 32) == 0; }
bool MD5Hash::operator==(const UString &other) const { return strncmp(this->hash, other.toUtf8(), 32) == 0; }
