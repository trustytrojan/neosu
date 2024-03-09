//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		macOS opengl interface
//
// $NoKeywords: $macgli
//===============================================================================//

#ifdef __APPLE__

#include "MacOSGLLegacyInterface.h"

#ifdef MCENGINE_FEATURE_OPENGL

#include "Engine.h"
#include "MacOSEnvironment.h"
#include "OpenGLHeaders.h"
#include "main_OSX_cpp.h"

MacOSGLLegacyInterface::MacOSGLLegacyInterface() : OpenGLLegacyInterface() {
    // NOTE: context creation is not handled in here, to allow both native implementations as well as SDL2
    // everything related is handed through to the wrapper (like setVSync())
}

MacOSGLLegacyInterface::~MacOSGLLegacyInterface() {}

void MacOSGLLegacyInterface::endScene() {
    glFlush();
    ((MacOSEnvironment *)(env))->getWrapper()->endScene();
}

void MacOSGLLegacyInterface::setVSync(bool vsync) { ((MacOSEnvironment *)(env))->getWrapper()->setVSync(vsync); }

#endif

#endif
