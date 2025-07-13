//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		frame limiter, to be called at the end of a main loop interation
//
// $NoKeywords: $fps $limiter
//===============================================================================//

#include "FPSLimiter.h"
#include "Timing.h"
#include "ConVar.h"

#include "types.h"

namespace FPSLimiter {
namespace  // static
{
u64 next_frame_time{0};
}

void limit_frames(int target_fps) {
    if(target_fps > 0) {
        const u64 frame_time_ns = Timing::NS_PER_SECOND / static_cast<u64>(target_fps);
        const u64 now = Timing::getTicksNS();

        // if we're ahead of schedule, sleep until next frame
        if(next_frame_time > now) {
            const u64 sleep_time = next_frame_time - now;
            Timing::sleepNS(sleep_time);
        } else if(cv_fps_max_yield.getBool()) {
            Timing::sleep(0);
            next_frame_time = Timing::getTicksNS();  // update "now" to reflect the time spent in yield
        } else {
            // behind schedule or exactly on time, reset to now
            next_frame_time = now;
        }
        // set time for next frame
        next_frame_time += frame_time_ns;
    } else if(cv_fps_unlimited_yield.getBool()) {
        Timing::sleep(0);
    }
}

void reset() { next_frame_time = 0; }

}  // namespace FPSLimiter
