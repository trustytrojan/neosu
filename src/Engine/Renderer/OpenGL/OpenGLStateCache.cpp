// Copyright (c) 2025, WH, All rights reserved.
#include "OpenGLStateCache.h"

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)

#include "OpenGLHeaders.h"

// init static cache
int OpenGLStateCache::iCurrentProgram{INT_MAX};
int OpenGLStateCache::iCurrentFramebuffer{INT_MAX};

std::array<int, 4> OpenGLStateCache::iViewport{};

void OpenGLStateCache::initialize() {
    if(OpenGLStateCache::iCurrentProgram != INT_MAX) return;

    // one-time initialization of cache from actual GL state
    OpenGLStateCache::refresh();
}

void OpenGLStateCache::refresh() {
    // only do the expensive query when necessary
    glGetIntegerv(GL_CURRENT_PROGRAM, &OpenGLStateCache::iCurrentProgram);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &OpenGLStateCache::iCurrentFramebuffer);
    glGetIntegerv(GL_VIEWPORT, OpenGLStateCache::iViewport.data());
}

void OpenGLStateCache::setCurrentProgram(int program) { OpenGLStateCache::iCurrentProgram = program; }

int OpenGLStateCache::getCurrentProgram() { return OpenGLStateCache::iCurrentProgram; }

void OpenGLStateCache::setCurrentFramebuffer(int framebuffer) { OpenGLStateCache::iCurrentFramebuffer = framebuffer; }

int OpenGLStateCache::getCurrentFramebuffer() { return OpenGLStateCache::iCurrentFramebuffer; }

void OpenGLStateCache::setCurrentViewport(int x, int y, int width, int height) {
    OpenGLStateCache::iViewport[0] = x;
    OpenGLStateCache::iViewport[1] = y;
    OpenGLStateCache::iViewport[2] = width;
    OpenGLStateCache::iViewport[3] = height;
}

void OpenGLStateCache::getCurrentViewport(int &x, int &y, int &width, int &height) {
    x = OpenGLStateCache::iViewport[0];
    y = OpenGLStateCache::iViewport[1];
    width = OpenGLStateCache::iViewport[2];
    height = OpenGLStateCache::iViewport[3];
}

#endif
