//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		fps timer
//
// $NoKeywords: $linuxtime $os
//===============================================================================//

#ifdef __linux__

#ifndef LINUXTIMER_H
#define LINUXTIMER_H

#include <time.h>

#include "Timer.h"

class LinuxTimer : public BaseTimer {
   public:
    LinuxTimer();
    virtual ~LinuxTimer() { ; }

    virtual void start() override;
    virtual void update() override;

    virtual inline double getDelta() const override { return this->delta; }
    virtual inline double getElapsedTime() const override { return this->elapsedTime; }
    virtual inline u64 getElapsedTimeMS() const override { return this->elapsedTimeMS; }

   private:
    timespec startTime;
    timespec currentTime;

    double delta;
    double elapsedTime;
    u64 elapsedTimeMS;
};

#endif

#endif
