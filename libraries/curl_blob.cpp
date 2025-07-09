#if !defined(_MSC_VER) && !((defined(__cpp_pp_embed) && __cpp_pp_embed >= 202502L) || (defined(__clang__) && __clang_major__ >= 19))

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

#endif
