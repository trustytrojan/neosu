#pragma once

// unsupported with MSVC, unneeded anyways because it uses schannel instead of OpenSSL
#ifndef _MSC_VER

// data in curl_blob.c
extern "C" {
extern const unsigned char curl_ca_embed[];
extern const unsigned char curl_ca_embed_end[];
}

unsigned long long curl_ca_embed_size();

#define curl_easy_setopt_CAINFO_BLOB_embedded(curl) \
    struct curl_blob blob{};                        \
    blob.data = (void *)curl_ca_embed;              \
    blob.len = curl_ca_embed_size();                \
    blob.flags = CURL_BLOB_NOCOPY;                  \
    curl_easy_setopt((curl), CURLOPT_CAINFO_BLOB, &blob)
#else
#define curl_easy_setopt_CAINFO_BLOB_embedded(curl)
#endif
