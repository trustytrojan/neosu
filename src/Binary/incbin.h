// Copyright (c) 2025, WH, All rights reserved.
#pragma once

#if defined(_MSC_VER) && !defined(__clang__)  // MSVC COFF (needs external assembler)

#define INCBIN_PLAT(sym, file)

#elif defined(_WIN32)  // MinGW PE/COFF

#define INCBIN_PLAT(sym, file)       \
    __asm__(                         \
        ".section .rdata\n"          \
        ".balign 1\n"                \
        ".globl " #sym "\n" #sym     \
        ":\n"                        \
        ".incbin \"" file            \
        "\"\n"                       \
        ".globl " #sym "_end\n" #sym \
        "_end:\n"                    \
        ".balign 1\n"                \
        ".section .text\n");

#else  // ELF

#define INCBIN_PLAT(sym, file)                 \
    __asm__(                                   \
        ".section .rodata\n"                   \
        ".balign 1\n"                          \
        ".globl " #sym                         \
        "\n"                                   \
        ".type " #sym ", @object\n" #sym       \
        ":\n"                                  \
        ".incbin \"" file                      \
        "\"\n"                                 \
        ".globl " #sym                         \
        "_end\n"                               \
        ".type " #sym "_end, @object\n" #sym   \
        "_end:\n"                              \
        ".size " #sym ", " #sym "_end - " #sym \
        "\n"                                   \
        ".balign 1\n"                          \
        ".section \".text\"\n");

#endif

/* declare the symbol in a header file */
#define INCBIN_H(sym, ...)                                                     \
    extern "C" {                                                               \
    extern const unsigned char sym[];                                          \
    extern const unsigned char sym##_end[];                                    \
    inline unsigned long long sym##_size() { return &sym##_end[0] - &sym[0]; } \
    }

/* define in one single source file */
#define INCBIN_C(sym, file) INCBIN_PLAT(sym, file)
