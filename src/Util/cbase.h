#pragma once

// ENVIRONMENT

#include "BaseEnvironment.h"

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

// EXTERNAL INCLUDES

#include <glm/trigonometric.hpp> // for glm::radians/glm::degrees

// DEFS

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

constexpr void runtime_assert(bool cond, const char *reason)
{
	if (cond) return;
	std::fprintf(stderr, "%s\n", reason);
	std::abort();
}

#define SAFE_DELETE(p)  \
    {                   \
        if(p) {         \
            delete(p);  \
            (p) = NULL; \
        }               \
    }

#define COLOR(a, r, g, b) ((Color)((((a) & 0xff) << 24) | (((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff)))

#define COLORf(a, r, g, b)                                                    \
    ((Color)(((((int)(std::clamp<float>(a, 0.0f, 1.0f) * 255.0f)) & 0xff) << 24) | \
             ((((int)(std::clamp<float>(r, 0.0f, 1.0f) * 255.0f)) & 0xff) << 16) | \
             ((((int)(std::clamp<float>(g, 0.0f, 1.0f) * 255.0f)) & 0xff) << 8) |  \
             (((int)(std::clamp<float>(b, 0.0f, 1.0f) * 255.0f)) & 0xff)))

#define COLOR_GET_Ri(color) (((Channel)((color) >> 16)))

#define COLOR_GET_Gi(color) (((Channel)((color) >> 8)))

#define COLOR_GET_Bi(color) (((Channel)((color) >> 0)))

#define COLOR_GET_Ai(color) (((Channel)((color) >> 24)))

#define COLOR_GET_Rf(color) (((Channel)((color) >> 16)) / 255.0f)

#define COLOR_GET_Gf(color) (((Channel)((color) >> 8)) / 255.0f)

#define COLOR_GET_Bf(color) (((Channel)((color) >> 0)) / 255.0f)

#define COLOR_GET_Af(color) (((Channel)((color) >> 24)) / 255.0f)

#define COLOR_INVERT(color) \
    (COLOR(COLOR_GET_Ai(color), 255 - COLOR_GET_Ri(color), 255 - COLOR_GET_Gi(color), 255 - COLOR_GET_Bi(color)))

#define COLOR_MULTIPLY(color1, color2)                                                                      \
    (COLORf(1.0f, COLOR_GET_Rf(color1) * COLOR_GET_Rf(color2), COLOR_GET_Gf(color1) * COLOR_GET_Gf(color2), \
            COLOR_GET_Bf(color1) * COLOR_GET_Bf(color2)))

#define COLOR_ADD(color1, color2)                                                        \
    (COLORf(1.0f, std::clamp<float>(COLOR_GET_Rf(color1) + COLOR_GET_Rf(color2), 0.0f, 1.0f), \
            std::clamp<float>(COLOR_GET_Gf(color1) + COLOR_GET_Gf(color2), 0.0f, 1.0f),       \
            std::clamp<float>(COLOR_GET_Bf(color1) + COLOR_GET_Bf(color2), 0.0f, 1.0f)))

#define COLOR_SUBTRACT(color1, color2)                                                   \
    (COLORf(1.0f, std::clamp<float>(COLOR_GET_Rf(color1) - COLOR_GET_Rf(color2), 0.0f, 1.0f), \
            std::clamp<float>(COLOR_GET_Gf(color1) - COLOR_GET_Gf(color2), 0.0f, 1.0f),       \
            std::clamp<float>(COLOR_GET_Bf(color1) - COLOR_GET_Bf(color2), 0.0f, 1.0f)))

constexpr const auto PI = std::numbers::pi;
constexpr const auto PIOVER180 = (PI/180.0f);

// UTIL

inline bool isInt(float f) { return (f == static_cast<float>(static_cast<int>(f))); }

// ANSI/IEEE 754-1985

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
            this->memory = (T *)realloc(this->memory, new_max * sizeof(T));
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
