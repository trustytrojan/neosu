#include "random.h"

#ifdef _WIN32
#include "WinDebloatDefs.h"
#include <windows.h>
#include <wincrypt.h>
#else
#include <sys/random.h>
#endif

#if !defined(_MSC_VER) && __has_include(<openssl/rand.h>)
#include <openssl/rand.h>
#define USE_OPENSSL
#endif

void get_random_bytes(u8 *out, std::size_t s_out) {
#ifdef USE_OPENSSL
    if(RAND_bytes(out, static_cast<i32>(s_out)) == 1) {
        return;
    }
#endif

#ifdef _WIN32
    HCRYPTPROV hCryptProv;
    if(CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hCryptProv, s_out, out);
        CryptReleaseContext(hCryptProv, 0);
    }
#else
    getrandom(out, s_out, 0);
#endif
}
