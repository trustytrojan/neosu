#if !defined(_MSC_VER)

#include "curl_blob.h"

// clang++ for some reason doesn't define this feature test macro but supports it anyways... (past ver 19)
#if !((defined(__cpp_pp_embed) && __cpp_pp_embed >= 202502L) || (defined(__clang__) && __clang_major__ >= 19))

asm(".section .rodata\n"
    ".balign 1\n"
    ".globl curl_ca_embed\n"
    ".type curl_ca_embed, @object\n"
    "curl_ca_embed:\n"
    ".incbin \"cacert.pem\"\n"
    ".globl curl_ca_embed_end\n"
    ".type curl_ca_embed_end, @object\n"
    "curl_ca_embed_end:\n"
    ".size curl_ca_embed, curl_ca_embed_end - curl_ca_embed\n");

unsigned long long curl_ca_embed_size() { return &curl_ca_embed_end[0] - &curl_ca_embed[0]; }
#else

// This file is just an SSL CA certificate bundle, which we use to make secure requests with curl
// (with CURLOPT_CAINFO_BLOB), without needing to rely on this data being found by curl/OpenSSL on the host
constexpr const unsigned char curl_ca_embed[] = {
// should be cached in <top-level>/build-aux/cache/curl-X.Y.Z/certbundle/cacert.pem
#embed CURL_CERT_BUNDLE_PATH
};

constexpr const unsigned char curl_ca_embed_end[]{};

unsigned long long curl_ca_embed_size() { return sizeof(curl_ca_embed); }

#endif
#endif
