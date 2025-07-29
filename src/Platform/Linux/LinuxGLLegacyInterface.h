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

class LinuxGLLegacyInterface final : public OpenGLLegacyInterface {
   public:
    LinuxGLLegacyInterface(Display *display, Window window);
    ~LinuxGLLegacyInterface() override;

    // scene
    void endScene() override;

    // device settings
    void setVSync(bool vsync) override;

    // ILLEGAL:
    [[nodiscard]] inline GLXContext getGLXContext() const { return this->glc; }

    static XVisualInfo *getVisualInfo(Display *display);

   private:
    Display *display;
    Window window;
    GLXContext glc;
};

#endif

#endif

#endif
