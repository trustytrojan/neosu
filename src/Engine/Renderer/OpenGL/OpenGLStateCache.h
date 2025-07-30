//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		OpenGL state cache for reducing synchronization points (WIP)
//
// $NoKeywords: $glsc
//===============================================================================//

#pragma once
#ifndef OPENGLSTATECACHE_H
#define OPENGLSTATECACHE_H

#include "BaseEnvironment.h"

#if defined(MCENGINE_FEATURE_OPENGL) || defined(MCENGINE_FEATURE_GLES32)

class OpenGLStateCache final {
   public:
    static OpenGLStateCache &getInstance();

    // program state
    void setCurrentProgram(int program);
    [[nodiscard]] int getCurrentProgram() const;

    // framebuffer state
    void setCurrentFramebuffer(int framebuffer);
    [[nodiscard]] int getCurrentFramebuffer() const;

    // viewport state
    void setCurrentViewport(int x, int y, int width, int height);
    void getCurrentViewport(int &x, int &y, int &width, int &height);

    // initialize cache with actual GL states (once at startup)
    void initialize();

    // force a refresh of cached states from actual GL state (expensive, avoid)
    void refresh();

   private:
    OpenGLStateCache();
    ~OpenGLStateCache() = default;

    // singleton pattern
    static OpenGLStateCache *s_instance;

    // cache
    int iCurrentProgram;
    int iCurrentFramebuffer;
    int iViewport[4];

    bool bInitialized;
};

#endif

#endif
