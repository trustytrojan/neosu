#pragma once

// This file is just an SSL CA certificate bundle, which we use to make secure requests with curl
// (with CURLOPT_CAINFO_BLOB), without needing to rely on this data being found by curl/OpenSSL on the host
static constexpr const unsigned char curl_ca_embed[] = {
#ifdef _WIN32 // only used on windows currently
#embed CURL_CERT_BUNDLE_PATH
#endif
};
