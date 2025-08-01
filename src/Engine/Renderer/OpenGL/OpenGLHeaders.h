#pragma once

#ifndef __EMSCRIPTEN__
#include "glad/glad.h"
#ifdef _WIN32
#include "glad_wgl/glad_wgl.h"
#endif
#else
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#include <GLES3/gl3platform.h>
#endif
