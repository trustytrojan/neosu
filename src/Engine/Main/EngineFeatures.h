#pragma once

#include "config.h"

#ifndef MCENGINE_DATA_DIR

#ifdef _WIN32
#define MCENGINE_DATA_DIR ".\\"
#else
#define MCENGINE_DATA_DIR "./"
#endif

#endif

/*
 * OpenGL graphics (Desktop, legacy + modern)
 */
#ifndef MCENGINE_FEATURE_OPENGL
#define MCENGINE_FEATURE_OPENGL
#endif

/*
 * OpenGL graphics (Mobile, ES/EGL)
 */
// #define MCENGINE_FEATURE_OPENGLES
