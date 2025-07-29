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
#include "Profiler.h"

XVisualInfo *LinuxGLLegacyInterface::getVisualInfo(Display *display) {
    return glXChooseVisual(
        display, DefaultScreen(display),
        std::array<GLint, 7>{GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 1, GLX_DOUBLEBUFFER, None}.data());
}

LinuxGLLegacyInterface::LinuxGLLegacyInterface(Display *display, Window window) : OpenGLLegacyInterface() {
    this->display = display;
    this->window = window;

    // create opengl context and make current
    auto *vi = getVisualInfo(this->display);
    if(vi == nullptr) {
        engine->showMessageError("OpenGL Error", "Couldn't glXChooseVisual()!\nThe engine will quit now.");
        return;
    }
    this->glc = glXCreateContext(this->display, vi, nullptr, GL_TRUE);
    if(this->glc == nullptr) {
        engine->showMessageError("OpenGL Error", "Couldn't glXCreateContext()!\nThe engine will quit now.");
        return;
    }
    XFree(vi);

    glXMakeCurrent(this->display, this->window, this->glc);
}

LinuxGLLegacyInterface::~LinuxGLLegacyInterface() {
    glXMakeCurrent(this->display, None, nullptr);
    glXDestroyContext(this->display, this->glc);
}

void LinuxGLLegacyInterface::endScene() {
    OpenGLLegacyInterface::endScene();
    {
        VPROF_BUDGET("SwapBuffers", VPROF_BUDGETGROUP_DRAW_SWAPBUFFERS);
        glXSwapBuffers(this->display, this->window);
    }
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
