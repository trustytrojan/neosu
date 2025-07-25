#pragma once

// ENVIRONMENT

#include "BaseEnvironment.h"

#include "types.h"

// ENGINE INCLUDES

#include "Environment.h"
#include "MultiCastDelegate.h"
#include "MakeDelegateWrapper.h"
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

#define PI 3.1415926535897932384626433832795
#define PIOVER180 0.01745329251994329576923690768489

// UTIL

inline bool isInt(float f) { return (f == static_cast<float>(static_cast<int>(f))); }
