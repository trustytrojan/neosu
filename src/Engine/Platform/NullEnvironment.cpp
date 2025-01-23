//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		empty environment, for debugging and new OS implementations
//
// $NoKeywords: $ne
//===============================================================================//

#include "NullEnvironment.h"

#include "Engine.h"
#include "NullContextMenu.h"
#include "NullGraphicsInterface.h"

extern bool g_bRunning;

NullEnvironment::NullEnvironment() {}

Graphics *NullEnvironment::createRenderer() { return new NullGraphicsInterface(); }

ContextMenu *NullEnvironment::createContextMenu() { return new NullContextMenu(); }

void NullEnvironment::shutdown() { g_bRunning = false; }

void NullEnvironment::restart() { this->shutdown(); }
