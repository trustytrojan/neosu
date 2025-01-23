#pragma once
#ifdef _WIN32

#include "WinGL3Interface.h"

#ifdef MCENGINE_FEATURE_OPENGL

#include "Engine.h"
#include "OpenGLHeaders.h"
#include "WinEnvironment.h"

PIXELFORMATDESCRIPTOR WinGL3Interface::getPixelFormatDescriptor() {
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_COMPOSITION;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 1;
    pfd.iLayerType = PFD_MAIN_PLANE;

    // experimental, for ghost mode
    /// pfd.cAlphaBits = 8;

    return pfd;
}

WinGL3Interface::WinGL3Interface(HWND hwnd) : OpenGL3Interface() {
    this->hwnd = hwnd;
    this->hglrc = NULL;
    this->hdc = NULL;

    if(!checkGLHardwareAcceleration()) {
        engine->showMessageErrorFatal("Fatal Engine Error",
                                      "No OpenGL hardware acceleration available!\nThe engine will quit now.");
        exit(0);
    }

    // get device context
    this->hdc = GetDC(this->hwnd);

    // get pixel format
    PIXELFORMATDESCRIPTOR pfd = getPixelFormatDescriptor();
    int pixelFormat = ChoosePixelFormat(this->hdc, &pfd);
    debugLog("OpenGL: PixelFormat = %i\n", pixelFormat);

    if(pixelFormat == 0) {
        engine->showMessageErrorFatal(
            "Fatal Engine Error",
            UString::format("ChoosePixelFormat() returned 0, GetLastError() = %i!\nThe engine will quit now.",
                            GetLastError()));
        exit(0);
    }

    // set pixel format
    BOOL result = SetPixelFormat(this->hdc, pixelFormat, &pfd);
    debugLog("OpenGL: SetPixelFormat() = %i\n", result);

    if(result == FALSE) {
        engine->showMessageErrorFatal(
            "Fatal Engine Error",
            UString::format("SetPixelFormat() returned 0, GetLastError() = %i!\nThe engine will quit now.",
                            GetLastError()));
        exit(0);
    }

    // WINDOWS: create temp context and make current
    HGLRC tempContext = wglCreateContext(this->hdc);
    wglMakeCurrent(this->hdc, tempContext);

    // so that we can get to wglCreateContextAttribsARB
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribs =
        (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    if(wglCreateContextAttribs == NULL) {
        engine->showMessageErrorFatal("Fatal Engine Error",
                                      "Couldn't get wglCreateContextAttribsARB()!\nThe engine will quit now.");
        exit(0);
    }

    const int attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB,          3, WGL_CONTEXT_MINOR_VERSION_ARB, 0, WGL_CONTEXT_FLAGS_ARB,
        WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, 0};

    this->hglrc = wglCreateContextAttribs(this->hdc, 0, attribs);
    wglMakeCurrent(this->hdc, this->hglrc);
    wglDeleteContext(tempContext);

    if(!this->hglrc) {
        engine->showMessageErrorFatal("Fatal OpenGL Error", "Couldn't wglCreateContext()!\nThe engine will quit now.");
        exit(0);
    }
}

WinGL3Interface::~WinGL3Interface() {
    if(this->hdc != NULL) wglMakeCurrent(this->hdc, NULL);  // deselect gl
    if(this->hglrc != NULL) wglDeleteContext(this->hglrc);  // delete gl
    if(this->hdc != NULL) DeleteDC(this->hdc);              // delete hdc
}

void WinGL3Interface::endScene() {
    OpenGL3Interface::endScene();
    SwapBuffers(this->hdc);
}

void WinGL3Interface::setVSync(bool vsync) {
    typedef BOOL(__stdcall * PFNWGLSWAPINTERVALPROC)(int);
    PFNWGLSWAPINTERVALPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if(wglSwapIntervalEXT)
        wglSwapIntervalEXT((int)vsync);
    else
        debugLog("OpenGL: Can't set VSync, wglSwapIntervalEXT not supported!\n");
}

bool WinGL3Interface::checkGLHardwareAcceleration() {
    HDC hdc = GetDC(((WinEnvironment*)env)->getHwnd());
    PIXELFORMATDESCRIPTOR pfd = getPixelFormatDescriptor();
    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    ReleaseDC(((WinEnvironment*)env)->getHwnd(), hdc);
    return pixelFormat;
}

#endif

#endif
