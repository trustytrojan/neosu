#pragma once

// unsupported with MSVC, unneeded anyways because it uses schannel instead of OpenSSL
#ifndef _MSC_VER
// This file is just an SSL CA certificate bundle, which we use to make secure requests with curl
// (with CURLOPT_CAINFO_BLOB), without needing to rely on this data being found by curl/OpenSSL on the host
static constexpr const unsigned char curl_ca_embed[] = {
#embed CURL_CERT_BUNDLE_PATH
};

#define curl_easy_setopt_CAINFO_BLOB_embedded(curl) \
    struct curl_blob blob{}; \
    blob.data = (void *)curl_ca_embed; \
    blob.len = sizeof(curl_ca_embed); \
    blob.flags = CURL_BLOB_NOCOPY; \
    curl_easy_setopt((curl), CURLOPT_CAINFO_BLOB, &blob)
#else
#define curl_easy_setopt_CAINFO_BLOB_embedded(curl)
#endif
