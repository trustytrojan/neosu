// Copyright (c) 2025, WH, All rights reserved.
#include "PlaybackInterpolator.h"
#include <algorithm>
#include <cmath>

u32 PlaybackInterpolator::update(f64 rawPositionMS, f64 currentTime, f64 playbackSpeed, bool isLooped, u64 lengthMS,
                                 bool isPlaying) {
    // reset state if not initialized or not playing
    if(this->dLastPositionTime <= 0.0 || !isPlaying) {
        reset(rawPositionMS, currentTime, playbackSpeed);
        return this->dLastInterpolatedPosition;
    }

    // update rate estimate when position changes
    if(this->dLastRawPosition != rawPositionMS) {
        const f64 timeDelta = currentTime - this->dLastPositionTime;

        // only update rate if enough time has passed (5ms minimum)
        if(timeDelta > 0.005) {
            f64 newRate = 1000.0;

            if(rawPositionMS >= this->dLastRawPosition) {
                // normal forward movement
                newRate = (rawPositionMS - this->dLastRawPosition) / timeDelta;
            } else if(isLooped && lengthMS > 0) {
                // handle loop wraparound
                f64 length = static_cast<f64>(lengthMS);
                f64 wrappedChange = (length - this->dLastRawPosition) + rawPositionMS;
                newRate = wrappedChange / timeDelta;
            } else {
                // backward movement (seeking), keep current rate
                newRate = this->iEstimatedRate;
            }

            // sanity check against expected rate (allow 20% deviation)
            const f64 expectedRate = 1000.0 * playbackSpeed;
            if(newRate < expectedRate * 0.8 || newRate > expectedRate * 1.2) {
                newRate = expectedRate * 0.7 + newRate * 0.3;  // blend back toward expected
            }

            // smooth the rate estimate
            this->iEstimatedRate = this->iEstimatedRate * 0.6 + newRate * 0.4;
        }

        this->dLastRawPosition = rawPositionMS;
        this->dLastPositionTime = currentTime;
    } else {
        // gradual adjustment when position hasn't changed for a while
        const f64 timeSinceLastChange = currentTime - this->dLastPositionTime;
        if(timeSinceLastChange > 0.1) {
            const f64 expectedRate = 1000.0 * playbackSpeed;
            this->iEstimatedRate = this->iEstimatedRate * 0.95 + expectedRate * 0.05;
        }
    }

    // interpolate position based on estimated rate
    const f64 timeSinceLastReading = currentTime - this->dLastPositionTime;
    const f64 interpolatedPosition = this->dLastRawPosition + (timeSinceLastReading * this->iEstimatedRate);

    // handle looping
    if(isLooped && lengthMS > 0) {
        f64 length = static_cast<f64>(lengthMS);
        if(interpolatedPosition >= length) {
            this->dLastInterpolatedPosition = static_cast<u32>(fmod(interpolatedPosition, length));
            return this->dLastInterpolatedPosition;
        }
    }

    this->dLastInterpolatedPosition = static_cast<u32>(std::max(0.0, interpolatedPosition));
    return this->dLastInterpolatedPosition;
}
