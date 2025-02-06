#include "types.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/random.h>
#endif

void get_random_bytes(u8 *out, size_t s_out) {
#ifdef _WIN32
    HCRYPTPROV hCryptProv;
    CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    CryptGenRandom(hCryptProv, s_out, &out);
    CryptReleaseContext(hCryptProv, 0);
#else
    getrandom(&out, s_out, 0);
#endif
}
