#pragma once

// ENVIRONMENT

#include "BaseEnvironment.h"

#ifdef _WIN32
// Windows build
#ifdef _WIN64
#define OS_NAME "win64"
// #define WINVER 0x0A00  // Windows 10, to enable the ifdefs in winuser.h for touch
// #define MCENGINE_WINDOWS_REALTIMESTYLUS_SUPPORT
// #define MCENGINE_WINDOWS_TOUCH_SUPPORT
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
#include "Color.h"
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

#define COLOR_GET_Ri(color) (((Channel)(color >> 16)))

#define COLOR_GET_Gi(color) (((Channel)(color >> 8)))

#define COLOR_GET_Bi(color) (((Channel)(color >> 0)))

#define COLOR_GET_Ai(color) (((Channel)(color >> 24)))

#define COLOR_GET_Rf(color) (((Channel)(color >> 16)) / 255.0f)

#define COLOR_GET_Gf(color) (((Channel)(color >> 8)) / 255.0f)

#define COLOR_GET_Bf(color) (((Channel)(color >> 0)) / 255.0f)

#define COLOR_GET_Af(color) (((Channel)(color >> 24)) / 255.0f)

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

// "bad" because a can be lower than b
template <class T>
inline T bad_clamp(T x, T a, T b) {
    return x < a ? a : (x > b ? b : x);
}

template <class T>
inline T lerp(T x1, T x2, T percent) {
    return x1 * (1 - percent) + x2 * percent;
}

template <class T>
inline int sign(T val) {
    return val > 0 ? 1 : (val == 0 ? 0 : -1);
}

inline float deg2rad(float deg) { return deg * (f32)PI / 180.0f; }

inline float rad2deg(float rad) { return rad * 180.0f / (f32)PI; }

inline bool isInt(float f) { return (f == static_cast<float>(static_cast<int>(f))); }

// ANSI/IEEE 754-1985

inline unsigned long &floatBits(float &f) { return *reinterpret_cast<unsigned long *>(&f); }

inline bool isFinite(float f) { return ((floatBits(f) & 0x7F800000) != 0x7F800000); }

char *strtok_x(char d, char **str);

// zero-initialized dynamic array, similar to std::vector but way faster when you don't need constructors
// obviously don't use it on complex types :)
template <class T>
struct zarray {
    zarray(size_t nb_initial = 0) {
        if(nb_initial > 0) {
            this->reserve(nb_initial);
            this->nb = nb_initial;
        }
    }
    ~zarray() { free(this->memory); }

    void push_back(T t) {
        if(this->nb + 1 > this->max) {
            this->reserve(this->max + this->max / 2 + 1);
        }

        this->memory[this->nb] = t;
        this->nb++;
    }

    void reserve(size_t new_max) {
        if(this->max == 0) {
            this->memory = (T *)calloc(new_max, sizeof(T));
        } else {
            this->memory = (T *)reallocarray(this->memory, new_max, sizeof(T));
            memset(&this->memory[this->max], 0, (new_max - this->max) * sizeof(T));
        }

        this->max = new_max;
    }

    void resize(size_t new_nb) {
        if(new_nb < this->nb) {
            memset(&this->memory[new_nb], 0, (this->nb - new_nb) * sizeof(T));
        } else if(new_nb > this->max) {
            this->reserve(new_nb);
        }

        this->nb = new_nb;
    }

    void swap(zarray<T> &other) {
        size_t omax = this->max;
        size_t onb = this->nb;
        T *omemory = this->memory;

        this->max = other.max;
        this->nb = other.nb;
        this->memory = other.memory;

        other.max = omax;
        other.nb = onb;
        other.memory = omemory;
    }

    T &operator[](size_t index) const { return this->memory[index]; }
    void clear() { this->nb = 0; }
    T *begin() const { return this->memory; }
    T *data() { return this->memory; }
    [[nodiscard]] bool empty() const { return this->nb == 0; }
    T *end() const { return &this->memory[this->nb]; }
    [[nodiscard]] size_t size() const { return this->nb; }

   private:
    size_t max = 0;
    size_t nb = 0;
    T *memory = NULL;
};
