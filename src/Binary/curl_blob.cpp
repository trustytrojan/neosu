#if !defined(_MSC_VER)  // not necessary

#include "curl_blob.h"

// This file is just an SSL CA certificate bundle, which we use to make secure requests with curl
// (with CURLOPT_CAINFO_BLOB), without needing to rely on this data being found by curl/OpenSSL on the host
INCBIN_C(curl_ca_embed, "cacert.pem")

#endif  // MSC_VER
