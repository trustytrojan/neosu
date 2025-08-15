#pragma once
// Copyright (c) 2025, WH, All rights reserved.
#ifndef OPENGLSTATECACHE_H
#define OPENGLSTATECACHE_H

#include "BaseEnvironment.h"

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)
#include <array>

class OpenGLStateCache final {
   public:
    // entirely static class so no ctors/dtors necessary
    OpenGLStateCache() = delete;
    ~OpenGLStateCache() = delete;

    OpenGLStateCache &operator=(const OpenGLStateCache &) = delete;
    OpenGLStateCache &operator=(OpenGLStateCache &&) = delete;
    OpenGLStateCache(const OpenGLStateCache &) = delete;
    OpenGLStateCache(OpenGLStateCache &&) = delete;

    // program state
    static void setCurrentProgram(int program);
    [[nodiscard]] static int getCurrentProgram();

    // framebuffer state
    static void setCurrentFramebuffer(int framebuffer);
    [[nodiscard]] static int getCurrentFramebuffer();

    // viewport state
    static void setCurrentViewport(int x, int y, int width, int height);
    static void getCurrentViewport(int &x, int &y, int &width, int &height);
    [[nodiscard]] inline static const std::array<int, 4> &getCurrentViewport() { return iViewport; }

    // initialize cache with actual GL states (once at startup)
    static void initialize();

    // force a refresh of cached states from actual GL state (expensive, avoid)
    static void refresh();

   private:
    static std::array<int, 4> iViewport;

    // cache
    static int iCurrentProgram;
    static int iCurrentFramebuffer;
};

#endif

#endif
