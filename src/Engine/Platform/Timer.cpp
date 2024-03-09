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

#elif defined __SWITCH__

#include "HorizonTimer.h"

#endif

Timer::Timer() {
    m_timer = NULL;

#ifdef _WIN32

    m_timer = new WinTimer();

#elif defined __linux__

    m_timer = new LinuxTimer();

#elif defined __APPLE__

    m_timer = new MacOSTimer();

#elif defined __SWITCH__

    m_timer = new HorizonTimer();

#else

#error Missing Timer implementation for OS!

#endif
}

Timer::~Timer() { SAFE_DELETE(m_timer); }
