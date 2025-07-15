#pragma once

#include "types.h"
#include <vector>

namespace crypto {

namespace rng {
void get_bytes(u8* out, size_t s_out);
}  // namespace rng

namespace hash {
void sha256(const void* data, size_t size, u8* hash);
void md5(const void* data, size_t size, u8* hash);
}  // namespace hash

namespace baseconv {
std::vector<u8> encode64(const u8* src, size_t len);
std::vector<u8> decode64(const u8* src, size_t len);
}  // namespace baseconv

}  // namespace crypto
