#pragma once

#include "config.h"

#if !(defined(MCENGINE_PLATFORM_WINDOWS) || defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || \
      defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__))

#ifdef __linux__
#ifdef __x86_64__
#define OS_NAME "linux-x64"
#else
#define OS_NAME "linux-i686"
#endif
#endif

typedef void* HWND;

#else  // Windows build

// #define WINVER 0x0A00  // Windows 10, to enable the ifdefs in winuser.h for touch
// #define MCENGINE_WINDOWS_REALTIMESTYLUS_SUPPORT
// #define MCENGINE_WINDOWS_TOUCH_SUPPORT

#if defined(_MSC_VER)
#ifdef _WIN64
#define _AMD64_
#elif defined(_WIN32)
#define _X86_
#endif
#endif

#ifdef NOMINMAX
#undef NOMINMAX
#endif

#define NOMINMAX
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
#define NOCRYPT
#define NOMETAFILE
#define MMNOSOUND

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN

#include <basetsd.h>
#include <windef.h>

#ifdef _WIN64
#define OS_NAME "win64"
#else
#define OS_NAME "win32"
#endif

#ifndef fileno
#define fileno _fileno
#endif

#ifndef isatty
#define isatty _isatty
#endif

#if defined(_MSC_VER)
typedef SSIZE_T ssize_t;
#endif

#endif

#ifndef OS_NAME
#error "OS not currently supported"
#endif
