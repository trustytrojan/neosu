#ifdef _WIN32

#include "WinTimer.h"

WinTimer::WinTimer() : BaseTimer() {
    LARGE_INTEGER ticks;
    QueryPerformanceFrequency(&ticks);

    this->secondsPerTick = 1.0 / (double)ticks.QuadPart;
    this->ticksPerSecond = ticks.QuadPart;

    this->startTime.QuadPart = 0;
    this->currentTime.QuadPart = 0;

    this->delta = 0.0;
    this->elapsedTime = 0.0;
    this->elapsedTimeMS = 0;
}

void WinTimer::start() {
    QueryPerformanceCounter(&this->startTime);
    this->currentTime = this->startTime;

    this->delta = 0.0;
    this->elapsedTime = 0.0;
    this->elapsedTimeMS = 0;
}

void WinTimer::update() {
    LARGE_INTEGER nowTime;
    QueryPerformanceCounter(&nowTime);

    this->delta = (double)(nowTime.QuadPart - this->currentTime.QuadPart) * this->secondsPerTick;
    this->elapsedTime = (double)(nowTime.QuadPart - this->startTime.QuadPart) * this->secondsPerTick;
    this->elapsedTimeMS = (u64)(((nowTime.QuadPart - this->startTime.QuadPart) * 1000) / this->ticksPerSecond);
    this->currentTime = nowTime;
}

#endif
