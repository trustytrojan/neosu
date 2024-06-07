#pragma once
#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define GLEW_STATIC
#include <gl/glew.h>

#define WGL_WGLEXT_PROTOTYPES // only used for wglDX*() interop stuff atm
#include <gl/wglew.h>

#include <gl/glu.h>
#include <gl/gl.h>

#endif

#ifdef __linux__

#ifdef MCENGINE_USE_SYSTEM_GLEW
#include <GL/glew.h>
#include <GL/glxew.h>
#else
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#endif

#endif

#ifdef __APPLE__

#define GLEW_STATIC
#include <glew.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>

#endif
