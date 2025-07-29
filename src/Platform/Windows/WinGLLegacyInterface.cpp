#ifdef _WIN32

#include "WindowsMain.h"
#include "WinGLLegacyInterface.h"

#ifdef MCENGINE_FEATURE_OPENGL

#include "ConVar.h"
#include "Engine.h"
#include "OpenGLHeaders.h"
#include "WinEnvironment.h"
#include "Profiler.h"

bool g_bARBMultisampleSupported = false;
int g_iARBMultisampleFormat = 0;

//****************//
//	Pixel Format  //
//****************//

PIXELFORMATDESCRIPTOR getPixelFormatDescriptor() {
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

//************************************//
//	handle enabling of multisampling  //
//************************************//
// NOTE: this is entirely unused, currently?

bool initWinGLMultisample(HDC hDC, HINSTANCE /*hInstance*/, HWND /*hWnd*/, int factor) {
    // check if the multisampling extension is available
    if(!GLAD_WGL_ARB_multisample || !wglChoosePixelFormatARB) return false;

    int pixelFormat;
    bool valid;
    UINT numFormats;
    float fAttributes[] = {0, 0};

    // attributes we want to test
    int iAttributes[] = {WGL_DRAW_TO_WINDOW_ARB,
                         GL_TRUE,
                         WGL_SUPPORT_OPENGL_ARB,
                         GL_TRUE,
                         WGL_ACCELERATION_ARB,
                         WGL_FULL_ACCELERATION_ARB,
                         WGL_COLOR_BITS_ARB,
                         24,
                         WGL_ALPHA_BITS_ARB,
                         8,
                         WGL_DEPTH_BITS_ARB,
                         24,
                         WGL_STENCIL_BITS_ARB,
                         1,
                         WGL_DOUBLE_BUFFER_ARB,
                         GL_TRUE,
                         WGL_SAMPLE_BUFFERS_ARB,
                         GL_TRUE,
                         WGL_SAMPLES_ARB,
                         factor,
                         0,
                         0};

    while(iAttributes[19] > 1) {
        // check if we can get a pixel format for the msaa value (iAttributes[19])
        valid = wglChoosePixelFormatARB(hDC, iAttributes, fAttributes, 1, &pixelFormat, &numFormats);

        // if it succeeded, and numFormats is >= 1, then we're done
        if(valid && numFormats >= 1) {
            g_bARBMultisampleSupported = true;
            g_iARBMultisampleFormat = pixelFormat;
            return true;
        }

        // otherwise, divide by two to get the next possible msaa value, and try again (until < 2)
        iAttributes[19] = iAttributes[19] / 2;
    }

    return false;
}

FAKE_CONTEXT WinGLLegacyInterface::createAndMakeCurrentWGLContext(HWND hwnd, PIXELFORMATDESCRIPTOR pfdIn) {
    FAKE_CONTEXT context;

    // get device context
    HDC tempHDC = GetDC(hwnd);
    context.hdc = tempHDC;

    // get pixel format
    int pixelFormat = ChoosePixelFormat(tempHDC, &pfdIn);
    debugLog("OpenGL: PixelFormat = %i\n", pixelFormat);

    // set pixel format
    BOOL result = SetPixelFormat(tempHDC, pixelFormat, &pfdIn);
    debugLog("OpenGL: SetPixelFormat() = %i\n", result);

    // create temp context and make current
    context.hglrc = wglCreateContext(tempHDC);
    wglMakeCurrent(tempHDC, context.hglrc);

    return context;
}

WinGLLegacyInterface::WinGLLegacyInterface(HWND new_hwnd) : OpenGLLegacyInterface() {
    this->hwnd = new_hwnd;

    if(!checkGLHardwareAcceleration()) {
        engine->showMessageErrorFatal("Fatal Engine Error",
                                      "No OpenGL hardware acceleration available!\nThe engine will quit now.");
        exit(0);
    }

    // get device context
    this->hdc = GetDC(this->hwnd);

    // get pixel format, use the MSAA one if it is supported
    PIXELFORMATDESCRIPTOR pfd = getPixelFormatDescriptor();
    int pixelFormat = 0;
    if(g_bARBMultisampleSupported)
        pixelFormat = g_iARBMultisampleFormat;
    else
        pixelFormat = ChoosePixelFormat(this->hdc, &pfd);

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

    // WINDOWS: HACKHACK: create temp context and make current
    this->hglrc = wglCreateContext(this->hdc);
    wglMakeCurrent(this->hdc, this->hglrc);

    if(!this->hglrc) {
        engine->showMessageErrorFatal("OpenGL Error", "Couldn't wglCreateContext()!\nThe engine will quit now.");
        engine->shutdown();
    }

    if(!gladLoadWGL(this->hdc)) {
        printf("FATAL ERROR: Couldn't resolve WGL functions (gladLoadWGL() failed)!\n\n");
        engine->showMessageErrorFatal("Fatal Engine Error", "Couldn't resolve WGL functions (gladLoadWGL() failed)");
        exit(0);
    }
}

WinGLLegacyInterface::~WinGLLegacyInterface() {
    if(this->hdc != NULL) wglMakeCurrent(this->hdc, NULL);  // deselect gl
    if(this->hglrc != NULL) wglDeleteContext(this->hglrc);  // delete gl
    if(this->hdc != NULL) DeleteDC(this->hdc);              // delete hdc
    gladUnloadWGL();                                        // unload wgl
}

void WinGLLegacyInterface::endScene() {
    OpenGLLegacyInterface::endScene();
    {
        VPROF_BUDGET("WinGLLegacyInterface::endScene", VPROF_BUDGETGROUP_DRAW_SWAPBUFFERS);
        SwapBuffers(this->hdc);
    }
}

void WinGLLegacyInterface::setVSync(bool vsync) {
    if(wglSwapIntervalEXT)
        wglSwapIntervalEXT((int)vsync);
    else
        debugLog("OpenGL: Can't set VSync, wglSwapIntervalEXT not supported!\n");
}

bool WinGLLegacyInterface::checkGLHardwareAcceleration() {
    HDC hdc = GetDC(baseEnv->getHwnd());
    PIXELFORMATDESCRIPTOR pfd = getPixelFormatDescriptor();
    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    ReleaseDC(baseEnv->getHwnd(), hdc);
    return pixelFormat;
}

#endif

#endif
