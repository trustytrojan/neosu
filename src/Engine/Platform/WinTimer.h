#pragma once
#ifdef _WIN32

#include "Timer.h"

// #define WIN32_LEAN_AND_MEAN
// #define NOCRYPT
#include <windows.h>

#include <basetsd.h>

class WinTimer : public BaseTimer {
   public:
    WinTimer();
    virtual ~WinTimer() { ; }

    virtual void start() override;
    virtual void update() override;

    virtual inline double getDelta() const override { return this->delta; }
    virtual inline double getElapsedTime() const override { return this->elapsedTime; }
    virtual inline u64 getElapsedTimeMS() const override { return this->elapsedTimeMS; }

   private:
    double secondsPerTick;
    LONGLONG ticksPerSecond;

    LARGE_INTEGER currentTime;
    LARGE_INTEGER startTime;

    double delta;
    double elapsedTime;
    u64 elapsedTimeMS;
};

#endif
