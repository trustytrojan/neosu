//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		linux opengl interface
//
// $NoKeywords: $linuxgli
//===============================================================================//

#ifdef __linux__

#include "LinuxGLLegacyInterface.h"

#ifdef MCENGINE_FEATURE_OPENGL

#include "Engine.h"
#include "LinuxEnvironment.h"
#include "Profiler.h"

#include <SDL3/SDL_video.h>

void LinuxGLLegacyInterface::endScene() {
    OpenGLLegacyInterface::endScene();
    {
        VPROF_BUDGET("SwapBuffers", VPROF_BUDGETGROUP_DRAW_SWAPBUFFERS);
        SDL_GL_SwapWindow(this->sdl_window);
    }
}

void LinuxGLLegacyInterface::setVSync(bool vsync) { SDL_GL_SetSwapInterval(vsync ? 1 : 0); }

#endif

#endif
