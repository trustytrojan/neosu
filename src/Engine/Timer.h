//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		fps timer
//
// $NoKeywords: $time
//===============================================================================//

#ifndef TIMER_H
#define TIMER_H

#include "cbase.h"  // for u64

class BaseTimer {
   public:
    virtual ~BaseTimer() { ; }

    virtual void start() = 0;
    virtual void update() = 0;

    virtual double getDelta() const = 0;
    virtual double getElapsedTime() const = 0;
    virtual u64 getElapsedTimeMS() const = 0;
};

class Timer {
   public:
    Timer();
    ~Timer();

    inline void start() { this->timer->start(); }
    inline void update() { this->timer->update(); }

    inline double getDelta() const { return this->timer->getDelta(); }
    inline double getElapsedTime() const { return this->timer->getElapsedTime(); }
    inline u64 getElapsedTimeMS() const { return this->timer->getElapsedTimeMS(); }

   private:
    BaseTimer *timer;
};

#endif
