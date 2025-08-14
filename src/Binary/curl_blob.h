// Copyright (c) 2025, WH, All rights reserved.
#pragma once

// unneeded for MSVC because it uses schannel instead of OpenSSL
#ifndef _MSC_VER

#include "incbin.h"

// data in curl_blob.cpp
INCBIN_H(curl_ca_embed)

// shorthand macro
#define curl_easy_setopt_CAINFO_BLOB_embedded(curl) \
    struct curl_blob blob{};                        \
    blob.data = (void *)curl_ca_embed;              \
    blob.len = curl_ca_embed_size();                \
    blob.flags = CURL_BLOB_NOCOPY;                  \
    curl_easy_setopt((curl), CURLOPT_CAINFO_BLOB, &blob)
#else
#define curl_easy_setopt_CAINFO_BLOB_embedded(curl)
#endif
