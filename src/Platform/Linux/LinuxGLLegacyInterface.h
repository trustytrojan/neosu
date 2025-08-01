//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		linux opengl interface
//
// $NoKeywords: $linuxgli
//===============================================================================//

#ifdef __linux__

#ifndef LINUXGLINTERFACE_H
#define LINUXGLINTERFACE_H

typedef unsigned char BYTE;

#include "OpenGLLegacyInterface.h"

#ifdef MCENGINE_FEATURE_OPENGL

#include "OpenGLHeaders.h"

typedef struct SDL_Window SDL_Window;

class LinuxGLLegacyInterface final : public OpenGLLegacyInterface {
   public:
    LinuxGLLegacyInterface(SDL_Window *window)
        : OpenGLLegacyInterface(), sdl_window(window) {}

    // scene
    void endScene() override;

    // device settings
    void setVSync(bool vsync) override;

   private:
    SDL_Window *sdl_window;
};

#endif

#endif

#endif
