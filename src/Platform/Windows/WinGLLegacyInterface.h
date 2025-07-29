//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		windows opengl interface
//
// $NoKeywords: $wingli
//===============================================================================//

#ifdef _WIN32

#ifndef WINGLINTERFACE_H
#define WINGLINTERFACE_H

#include "OpenGLLegacyInterface.h"

#ifdef MCENGINE_FEATURE_OPENGL

#include "WinDebloatDefs.h"

#include <windows.h>

class ConVar;

PIXELFORMATDESCRIPTOR getPixelFormatDescriptor();
bool initWinGLMultisample(HDC hDC, HINSTANCE hInstance, HWND hWnd, int factor);

struct FAKE_CONTEXT {
    HGLRC hglrc;
    HDC hdc;
};

class WinGLLegacyInterface final : public OpenGLLegacyInterface {
   public:
    static FAKE_CONTEXT createAndMakeCurrentWGLContext(HWND hwnd, PIXELFORMATDESCRIPTOR pfdIn);

   public:
    WinGLLegacyInterface(HWND hwnd);
    ~WinGLLegacyInterface() override;

    // scene
    void endScene() override;

    // device settings
    void setVSync(bool vsync) override;

    // ILLEGAL:
    bool checkGLHardwareAcceleration();
    inline HGLRC getGLContext() const { return this->hglrc; }
    inline HDC getGLHDC() const { return this->hdc; }

   private:
    // device context
    HWND hwnd;
    HGLRC hglrc;
    HDC hdc;
};

#endif

#endif

#endif
