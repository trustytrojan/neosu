//========== Copyright (c) 2015, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		stopwatch/timer
//
// $NoKeywords: $time $chrono
//===============================================================================//

#pragma once
#ifndef TIMER_H
#define TIMER_H

#include "BaseEnvironment.h"

#include <SDL3/SDL_timer.h>

#include <thread>
#include <concepts>
#include <cstdint>

namespace Timing {
// conversion constants
constexpr uint64_t NS_PER_SECOND = 1'000'000'000;
constexpr uint64_t NS_PER_MS = 1'000'000;
constexpr uint64_t NS_PER_US = 1'000;
constexpr uint64_t US_PER_MS = 1'000;
constexpr uint64_t MS_PER_SECOND = 1'000;

namespace detail {
static forceinline INLINE_BODY void yield_internal() noexcept {
    if constexpr(Env::cfg(OS::WASM))
        SDL_DelayNS(0);
    else
        std::this_thread::yield();
}

template <uint64_t Ratio>
static constexpr forceinline INLINE_BODY uint64_t convert_time(uint64_t ns) noexcept {
    return ns / Ratio;
}

}  // namespace detail

static forceinline INLINE_BODY uint64_t getTicksNS() noexcept { return SDL_GetTicksNS(); }

static constexpr forceinline INLINE_BODY uint64_t ticksNSToMS(uint64_t ns) noexcept {
    return detail::convert_time<NS_PER_MS>(ns);
}

static forceinline INLINE_BODY uint64_t getTicksMS() noexcept { return ticksNSToMS(getTicksNS()); }

static forceinline INLINE_BODY void sleep(uint64_t us) noexcept {
    us > 0 ? SDL_DelayNS(us * NS_PER_US) : detail::yield_internal();
}

static forceinline INLINE_BODY void sleepNS(uint64_t ns) noexcept {
    ns > 0 ? SDL_DelayNS(ns) : detail::yield_internal();
}

static forceinline INLINE_BODY void sleepMS(uint64_t ms) noexcept {
    ms > 0 ? SDL_DelayNS(ms * NS_PER_MS) : detail::yield_internal();
}

static forceinline INLINE_BODY void sleepPrecise(uint64_t us) noexcept {
    us > 0 ? SDL_DelayPrecise(us * NS_PER_US) : detail::yield_internal();
}

static forceinline INLINE_BODY void sleepNSPrecise(uint64_t ns) noexcept {
    ns > 0 ? SDL_DelayPrecise(ns) : detail::yield_internal();
}

static forceinline INLINE_BODY void sleepMSPrecise(uint64_t ms) noexcept {
    ms > 0 ? SDL_DelayPrecise(ms * NS_PER_MS) : detail::yield_internal();
}


template <typename T = double>
    requires(std::floating_point<T>)
static constexpr forceinline INLINE_BODY T timeNSToSeconds(uint64_t ns) noexcept {
    return static_cast<T>(ns) / static_cast<T>(NS_PER_SECOND);
}

// current time (since init.) in seconds as float
// decoupled from engine updates!
template <typename T = double>
    requires(std::floating_point<T>)
static forceinline INLINE_BODY T getTimeReal() noexcept {
    return timeNSToSeconds<T>(getTicksNS());
}

class Timer {
   public:
    explicit Timer(bool startOnCtor = false) noexcept {
        if(startOnCtor) start();
    }

    ~Timer() = default;
    Timer(const Timer &) = default;
    Timer &operator=(const Timer &) = default;
    Timer(Timer &&) = default;
    Timer &operator=(Timer &&) = default;

    inline void start() noexcept {
        this->startTimeNS = getTicksNS();
        this->lastUpdateNS = this->startTimeNS;
        this->deltaSeconds = 0.0;
    }

    inline void update() noexcept {
        const uint64_t now = getTicksNS();
        this->deltaSeconds = timeNSToSeconds<double>(now - this->lastUpdateNS);
        this->lastUpdateNS = now;
    }

    inline void reset() noexcept {
        this->startTimeNS = getTicksNS();
        this->lastUpdateNS = this->startTimeNS;
        this->deltaSeconds = 0.0;
    }

    [[nodiscard]] constexpr double getDelta() const noexcept { return this->deltaSeconds; }

    [[nodiscard]] inline double getElapsedTime() const noexcept {
        return timeNSToSeconds<double>(this->lastUpdateNS - this->startTimeNS);
    }

    [[nodiscard]] inline uint64_t getElapsedTimeMS() const noexcept {
        return ticksNSToMS(this->lastUpdateNS - this->startTimeNS);
    }

    [[nodiscard]] inline uint64_t getElapsedTimeNS() const noexcept { return this->lastUpdateNS - this->startTimeNS; }

    // get elapsed time without needing update()
    [[nodiscard]] inline double getLiveElapsedTime() const noexcept {
        return timeNSToSeconds<double>(getTicksNS() - this->startTimeNS);
    }

    [[nodiscard]] inline uint64_t getLiveElapsedTimeNS() const noexcept { return getTicksNS() - this->startTimeNS; }

   private:
    uint64_t startTimeNS{};
    uint64_t lastUpdateNS{};
    double deltaSeconds{};
};

};  // namespace Timing

using Timer = Timing::Timer;

#endif
