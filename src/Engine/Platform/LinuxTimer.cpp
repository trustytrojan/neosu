//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		fps timer
//
// $NoKeywords: $linuxtime $os
//===============================================================================//

#ifdef __linux__

#include "LinuxTimer.h"

static timespec diff(timespec start, timespec end) {
    timespec temp;
    if((end.tv_nsec - start.tv_nsec) < 0) {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

LinuxTimer::LinuxTimer() : BaseTimer() {
    this->startTime.tv_sec = 0;
    this->startTime.tv_nsec = 0;
    this->currentTime.tv_sec = 0;
    this->currentTime.tv_nsec = 0;

    this->delta = 0.0;
    this->elapsedTime = 0.0;
    this->elapsedTimeMS = 0;
}

void LinuxTimer::start() {
    clock_gettime(CLOCK_REALTIME, &this->startTime);
    this->currentTime = this->startTime;

    this->delta = 0.0;
    this->elapsedTime = 0.0;
    this->elapsedTimeMS = 0;
}

void LinuxTimer::update() {
    timespec t;
    clock_gettime(CLOCK_REALTIME, &t);

    const timespec delta = diff(this->currentTime, t);
    this->delta = delta.tv_sec + (double)delta.tv_nsec / 1000000000.0;

    const timespec elapsed = diff(this->startTime, t);
    this->elapsedTime = elapsed.tv_sec + (double)elapsed.tv_nsec / 1000000000.0;
    this->elapsedTimeMS = ((u64)elapsed.tv_sec * (u64)1000) + ((u64)elapsed.tv_nsec / (u64)1000000);

    this->currentTime = t;
}

#endif
