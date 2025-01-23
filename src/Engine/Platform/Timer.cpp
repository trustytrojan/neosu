//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		fps timer
//
// $NoKeywords: $time $os
//===============================================================================//

#include "Timer.h"

#ifdef _WIN32

#include "WinTimer.h"

#elif defined __linux__

#include "LinuxTimer.h"

#elif defined __APPLE__

#include "MacOSTimer.h"

#endif

Timer::Timer() {
    this->timer = NULL;

#ifdef _WIN32

    this->timer = new WinTimer();

#elif defined __linux__

    this->timer = new LinuxTimer();

#elif defined __APPLE__

    this->timer = new MacOSTimer();

#else

#error Missing Timer implementation for OS!

#endif
}

Timer::~Timer() { SAFE_DELETE(this->timer); }
