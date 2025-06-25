//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		windows opengl 3.x interface
//
// $NoKeywords: $wingl3i
//===============================================================================//

#ifdef _WIN32

#ifndef WINGL3INTERFACE_H
#define WINGL3INTERFACE_H

#include "OpenGL3Interface.h"

#ifdef MCENGINE_FEATURE_OPENGL
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class WinGL3Interface : public OpenGL3Interface {
   public:
    static PIXELFORMATDESCRIPTOR getPixelFormatDescriptor();

   public:
    WinGL3Interface(HWND hwnd);
    virtual ~WinGL3Interface();

    // scene
    void endScene();

    // device settings
    void setVSync(bool vsync);

    // ILLEGAL:
    bool checkGLHardwareAcceleration();
    inline HGLRC getGLContext() const { return this->hglrc; }
    inline HDC getHDC() const { return this->hdc; }

   private:
    // device context
    HWND hwnd;
    HGLRC hglrc;
    HDC hdc;
};

#endif

#endif

#endif
