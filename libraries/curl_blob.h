#pragma once

// unsupported with MSVC, unneeded anyways because it uses schannel instead of OpenSSL
#ifndef _MSC_VER
// clang++ for some reason doesn't define this feature test macro but supports it anyways... (past ver 19)
#if (defined(__cpp_pp_embed) && __cpp_pp_embed >= 202502L) || (defined(__clang__) && __clang_major__ >= 19)
// This file is just an SSL CA certificate bundle, which we use to make secure requests with curl
// (with CURLOPT_CAINFO_BLOB), without needing to rely on this data being found by curl/OpenSSL on the host
static constexpr const unsigned char curl_ca_embed[] = {
#embed CURL_CERT_BUNDLE_PATH
};

static constexpr unsigned long long curl_ca_embed_size() { return sizeof(curl_ca_embed); }
#else
// data in curl_blob.c
extern "C" {
extern const unsigned char curl_ca_embed[];
extern const unsigned char curl_ca_embed_end[];
}

static inline unsigned long long curl_ca_embed_size() { return &curl_ca_embed_end[0] - &curl_ca_embed[0]; }

#endif

#define curl_easy_setopt_CAINFO_BLOB_embedded(curl) \
    struct curl_blob blob{};                        \
    blob.data = (void *)curl_ca_embed;              \
    blob.len = curl_ca_embed_size();                \
    blob.flags = CURL_BLOB_NOCOPY;                  \
    curl_easy_setopt((curl), CURLOPT_CAINFO_BLOB, &blob)
#else
#define curl_easy_setopt_CAINFO_BLOB_embedded(curl)
#endif
