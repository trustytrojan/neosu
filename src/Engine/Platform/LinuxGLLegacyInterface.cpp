//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		linux opengl interface
//
// $NoKeywords: $linuxgli
//===============================================================================//

#ifdef __linux__

#include "LinuxGLLegacyInterface.h"

#ifdef MCENGINE_FEATURE_OPENGL

#include "ConVar.h"
#include "Engine.h"
#include "LinuxEnvironment.h"

XVisualInfo *getVisualInfo(Display *display) {
    GLint att[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 1, GLX_DOUBLEBUFFER, None};
    return glXChooseVisual(display, 0, att);
}

LinuxGLLegacyInterface::LinuxGLLegacyInterface(Display *display, Window window) : OpenGLLegacyInterface() {
    this->display = display;
    this->window = window;

    // create opengl context and make current
    this->glc = glXCreateContext(this->display, getVisualInfo(this->display), NULL, GL_TRUE);
    if(this->glc == NULL) {
        engine->showMessageError("OpenGL Error", "Couldn't glXCreateContext()!\nThe engine will quit now.");
        return;
    }
    glXMakeCurrent(this->display, this->window, this->glc);
}

LinuxGLLegacyInterface::~LinuxGLLegacyInterface() {
    glXMakeCurrent(this->display, None, NULL);
    glXDestroyContext(this->display, this->glc);
}

void LinuxGLLegacyInterface::endScene() {
    OpenGLLegacyInterface::endScene();
    glXSwapBuffers(this->display, this->window);
}

void LinuxGLLegacyInterface::setVSync(bool vsync) {
    if(!glXSwapIntervalEXT) {
        debugLog("OpenGL: Can't set VSync, GLX_EXT_swap_control not supported!\n");
        return;
    } else {
        glXSwapIntervalEXT(this->display, glXGetCurrentDrawable(), vsync ? 1 : 0);
    }
}

#endif

#endif
