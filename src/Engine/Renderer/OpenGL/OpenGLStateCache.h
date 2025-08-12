#pragma once
// Copyright (c) 2025, WH, All rights reserved.
#ifndef OPENGLSTATECACHE_H
#define OPENGLSTATECACHE_H

#include "BaseEnvironment.h"

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)

#include <array>

class OpenGLStateCache final {
   public:
    OpenGLStateCache &operator=(const OpenGLStateCache &) = delete;
    OpenGLStateCache &operator=(OpenGLStateCache &&) = delete;
    OpenGLStateCache(const OpenGLStateCache &) = delete;
    OpenGLStateCache(OpenGLStateCache &&) = delete;

    static OpenGLStateCache &getInstance();

    // program state
    void setCurrentProgram(int program);
    [[nodiscard]] int getCurrentProgram() const;

    // framebuffer state
    void setCurrentFramebuffer(int framebuffer);
    [[nodiscard]] int getCurrentFramebuffer() const;

    // viewport state
    void setCurrentViewport(int x, int y, int width, int height);
    void getCurrentViewport(int &x, int &y, int &width, int &height) const;
    [[nodiscard]] inline const std::array<int, 4> &getCurrentViewport() const { return this->iViewport; }

    // initialize cache with actual GL states (once at startup)
    void initialize();

    // force a refresh of cached states from actual GL state (expensive, avoid)
    void refresh();

   private:
    OpenGLStateCache() = default;
    ~OpenGLStateCache() = default;

    std::array<int, 4> iViewport{};

    // singleton pattern
    static OpenGLStateCache *s_instance;

    // cache
    int iCurrentProgram{0};
    int iCurrentFramebuffer{0};

    bool bInitialized{false};
};

#endif

#endif
