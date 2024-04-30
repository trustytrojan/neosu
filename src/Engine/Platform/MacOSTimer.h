//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		fps timer
//
// $NoKeywords: $mactime $os
//===============================================================================//

#ifdef __APPLE__

#ifndef MACOSTIMER_H
#define MACOSTIMER_H

#include <mach/mach.h>
#include <mach/mach_time.h>

#include "Timer.h"

class MacOSTimer : public BaseTimer {
   public:
    MacOSTimer();
    virtual ~MacOSTimer() { ; }

    virtual void start() override;
    virtual void update() override;

    virtual inline double getDelta() const override { return m_delta; }
    virtual inline double getElapsedTime() const override { return m_elapsedTime; }
    virtual inline u64 getElapsedTimeMS() const override { return m_elapsedTimeMS; }

   private:
    mach_timebase_info_data_t m_timebaseInfo;
    u64 m_currentTime;
    u64 m_startTime;

    double m_delta;
    double m_elapsedTime;
    u64 m_elapsedTimeMS;
};

#endif

#endif
