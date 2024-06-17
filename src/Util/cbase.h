#pragma once

#ifdef _WIN32
// Windows build
#ifdef _WIN64
#define OS_NAME "win64"
#define WINVER 0x0A00  // Windows 10, to enable the ifdefs in winuser.h for touch
#define MCENGINE_WINDOWS_REALTIMESTYLUS_SUPPORT
#define MCENGINE_WINDOWS_TOUCH_SUPPORT
#else
#define OS_NAME "win32"
#endif
// End of Windows build
#else
// Linux build
#ifdef __x86_64
#define OS_NAME "linux-x64"
#else
#define OS_NAME "linux-i686"
#endif
// End of Linux build
#endif

#ifdef _WIN32
// clang-format off
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <shlwapi.h> // for StrStrIA
#include <wincrypt.h> // for random number generation
// clang-format on
#endif

// STD INCLUDES

#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include <algorithm>
#include <atomic>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "types.h"

// ENGINE INCLUDES

#include "EngineFeatures.h"
#include "Environment.h"
#include "FastDelegate.h"
#include "Graphics.h"
#include "KeyboardEvent.h"
#include "Matrices.h"
#include "Rect.h"
#include "UString.h"
#include "Vectors.h"

// DEFS

#ifdef _WIN32
#define reallocarray(ptr, a, b) realloc(ptr, a *b)
#define strcasestr(a, b) StrStrIA(a, b)
#define strcasecmp(a, b) _stricmp(a, b)
#endif

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

typedef unsigned char COLORPART;

#define SAFE_DELETE(p)  \
    {                   \
        if(p) {         \
            delete(p);  \
            (p) = NULL; \
        }               \
    }

#define COLOR(a, r, g, b) ((Color)((((a) & 0xff) << 24) | (((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff)))

#define COLORf(a, r, g, b)                                                    \
    ((Color)(((((int)(clamp<float>(a, 0.0f, 1.0f) * 255.0f)) & 0xff) << 24) | \
             ((((int)(clamp<float>(r, 0.0f, 1.0f) * 255.0f)) & 0xff) << 16) | \
             ((((int)(clamp<float>(g, 0.0f, 1.0f) * 255.0f)) & 0xff) << 8) |  \
             (((int)(clamp<float>(b, 0.0f, 1.0f) * 255.0f)) & 0xff)))

#define COLOR_GET_Ri(color) (((COLORPART)(color >> 16)))

#define COLOR_GET_Gi(color) (((COLORPART)(color >> 8)))

#define COLOR_GET_Bi(color) (((COLORPART)(color >> 0)))

#define COLOR_GET_Ai(color) (((COLORPART)(color >> 24)))

#define COLOR_GET_Rf(color) (((COLORPART)(color >> 16)) / 255.0f)

#define COLOR_GET_Gf(color) (((COLORPART)(color >> 8)) / 255.0f)

#define COLOR_GET_Bf(color) (((COLORPART)(color >> 0)) / 255.0f)

#define COLOR_GET_Af(color) (((COLORPART)(color >> 24)) / 255.0f)

#define COLOR_INVERT(color) \
    (COLOR(COLOR_GET_Ai(color), 255 - COLOR_GET_Ri(color), 255 - COLOR_GET_Gi(color), 255 - COLOR_GET_Bi(color)))

#define COLOR_MULTIPLY(color1, color2)                                                                      \
    (COLORf(1.0f, COLOR_GET_Rf(color1) * COLOR_GET_Rf(color2), COLOR_GET_Gf(color1) * COLOR_GET_Gf(color2), \
            COLOR_GET_Bf(color1) * COLOR_GET_Bf(color2)))

#define COLOR_ADD(color1, color2)                                                        \
    (COLORf(1.0f, clamp<float>(COLOR_GET_Rf(color1) + COLOR_GET_Rf(color2), 0.0f, 1.0f), \
            clamp<float>(COLOR_GET_Gf(color1) + COLOR_GET_Gf(color2), 0.0f, 1.0f),       \
            clamp<float>(COLOR_GET_Bf(color1) + COLOR_GET_Bf(color2), 0.0f, 1.0f)))

#define COLOR_SUBTRACT(color1, color2)                                                   \
    (COLORf(1.0f, clamp<float>(COLOR_GET_Rf(color1) - COLOR_GET_Rf(color2), 0.0f, 1.0f), \
            clamp<float>(COLOR_GET_Gf(color1) - COLOR_GET_Gf(color2), 0.0f, 1.0f),       \
            clamp<float>(COLOR_GET_Bf(color1) - COLOR_GET_Bf(color2), 0.0f, 1.0f)))

#define PI 3.1415926535897932384626433832795

#define PIOVER180 0.01745329251994329576923690768489

// UTIL

template <class T>
inline T lerp(T x1, T x2, T percent) {
    return x1 * (1 - percent) + x2 * percent;
}

template <class T>
inline int sign(T val) {
    return val > 0 ? 1 : (val == 0 ? 0 : -1);
}

inline float deg2rad(float deg) { return deg * PI / 180.0f; }

inline float rad2deg(float rad) { return rad * 180.0f / PI; }

inline bool isInt(float f) { return (f == static_cast<float>(static_cast<int>(f))); }

// ANSI/IEEE 754-1985

inline unsigned long &floatBits(float &f) { return *reinterpret_cast<unsigned long *>(&f); }

inline bool isFinite(float f) { return ((floatBits(f) & 0x7F800000) != 0x7F800000); }

// zero-initialized dynamic array, similar to std::vector but way faster when you don't need constructors
// obviously don't use it on complex types :)
template <class T>
struct zarray {
    zarray(size_t nb_initial = 0) {
        if(nb_initial > 0) {
            reserve(nb_initial);
            nb = nb_initial;
        }
    }
    ~zarray() { free(memory); }

    void push_back(T t) {
        if(nb + 1 > max) {
            reserve(max + max / 2 + 1);
        }

        memory[nb] = t;
        nb++;
    }

    void reserve(size_t new_max) {
        if(max == 0) {
            memory = (T *)calloc(new_max, sizeof(T));
        } else {
            memory = (T *)reallocarray(memory, new_max, sizeof(T));
            memset(&memory[max], 0, (new_max - max) * sizeof(T));
        }

        max = new_max;
    }

    void resize(size_t new_nb) {
        if(new_nb < nb) {
            memset(&memory[new_nb], 0, (nb - new_nb) * sizeof(T));
        } else if(new_nb > max) {
            reserve(new_nb);
        }

        nb = new_nb;
    }

    void swap(zarray<T> &other) {
        size_t omax = max;
        size_t onb = nb;
        T *omemory = memory;

        max = other.max;
        nb = other.nb;
        memory = other.memory;

        other.max = omax;
        other.nb = onb;
        other.memory = omemory;
    }

    T &operator[](size_t index) const { return memory[index]; }
    void clear() { nb = 0; }
    T *begin() const { return memory; }
    T *data() { return memory; }
    bool empty() const { return nb == 0; }
    T *end() const { return &memory[nb]; }
    size_t size() const { return nb; }

   private:
    size_t max = 0;
    size_t nb = 0;
    T *memory = NULL;
};
