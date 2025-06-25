//================ Copyright (c) 2019, PG, All rights reserved. =================//
//
// Purpose:		windows sdl opengl es 2.x interface
//
// $NoKeywords: $winsdlgles2i
//===============================================================================//

#ifdef _WIN32

#include "WinSDLGLES2Interface.h"

#if defined(MCENGINE_FEATURE_SDL) && defined(MCENGINE_FEATURE_OPENGLES)

#include "Engine.h"
#include "OpenGLHeaders.h"

WinSDLGLES2Interface::WinSDLGLES2Interface(SDL_Window *window) : SDLGLES2Interface(window) {
	// resolve GL functions
	if (!gladLoadGL())
	{
		debugLog("gladLoadGL() error\n");
		engine->showMessageErrorFatal("OpenGL Error", "Couldn't gladLoadGL()!\nThe engine will exit now.");
		engine->shutdown();
		return;
	}
	debugLogF("OpenGL Version: {}\n", reinterpret_cast<const char *>(glGetString(GL_VERSION)));
}

WinSDLGLES2Interface::~WinSDLGLES2Interface() {}

#endif

#endif
