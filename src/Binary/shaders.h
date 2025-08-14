// Copyright (c) 2025, WH, All rights reserved.
#pragma once

#include "config.h"
#include "incbin.h"

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)

/* .glsl files located in assets/shaders */
#define BASE_SHADER_NAMES(VorF, X)                                      \
    X(cursortrail_##VorF##sh, "cursortrail_" #VorF ".glsl")             \
    X(slider_##VorF##sh, "slider_" #VorF ".glsl")                       \
    X(flashlight_##VorF##sh, "flashlight_" #VorF ".glsl")               \
    X(actual_flashlight_##VorF##sh, "actual_flashlight_" #VorF ".glsl") \
    X(smoothclip_##VorF##sh, "smoothclip_" #VorF ".glsl")

#define ALL_SHADER_BINARIES(X) \
    BASE_SHADER_NAMES(v, X)    \
    BASE_SHADER_NAMES(f, X)

ALL_SHADER_BINARIES(INCBIN_H)

#endif
