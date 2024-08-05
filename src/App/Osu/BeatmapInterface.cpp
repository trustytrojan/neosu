#include "BeatmapInterface.h"

#include "GameRules.h"

f32 BeatmapInterface::getHitWindow300() {
    return GameRules::mapDifficultyRange(getOD(), GameRules::getMinHitWindow300(), GameRules::getMidHitWindow300(),
                                         GameRules::getMaxHitWindow300());
}

f32 BeatmapInterface::getRawHitWindow300() {
    return GameRules::mapDifficultyRange(getRawOD(), GameRules::getMinHitWindow300(), GameRules::getMidHitWindow300(),
                                         GameRules::getMaxHitWindow300());
}

f32 BeatmapInterface::getHitWindow100() {
    return GameRules::mapDifficultyRange(getOD(), GameRules::getMinHitWindow100(), GameRules::getMidHitWindow100(),
                                         GameRules::getMaxHitWindow100());
}

f32 BeatmapInterface::getHitWindow50() {
    return GameRules::mapDifficultyRange(getOD(), GameRules::getMinHitWindow50(), GameRules::getMidHitWindow50(),
                                         GameRules::getMaxHitWindow50());
}

f32 BeatmapInterface::getApproachRateForSpeedMultiplier(f32 speedMultiplier) {
    return GameRules::mapDifficultyRangeInv((f32)getApproachTime() * (1.0f / speedMultiplier),
                                            GameRules::getMinApproachTime(), GameRules::getMidApproachTime(),
                                            GameRules::getMaxApproachTime());
}

f32 BeatmapInterface::getRawApproachRateForSpeedMultiplier(f32 speedMultiplier) {
    return GameRules::mapDifficultyRangeInv((f32)getRawApproachTime() * (1.0f / speedMultiplier),
                                            GameRules::getMinApproachTime(), GameRules::getMidApproachTime(),
                                            GameRules::getMaxApproachTime());
}

f32 BeatmapInterface::getConstantApproachRateForSpeedMultiplier(f32 speedMultiplier) {
    return GameRules::mapDifficultyRangeInv((f32)getRawApproachTime() * speedMultiplier,
                                            GameRules::getMinApproachTime(), GameRules::getMidApproachTime(),
                                            GameRules::getMaxApproachTime());
}

f32 BeatmapInterface::getOverallDifficultyForSpeedMultiplier(f32 speedMultiplier) {
    return GameRules::mapDifficultyRangeInv((f32)getHitWindow300() * (1.0f / speedMultiplier),
                                            GameRules::getMinHitWindow300(), GameRules::getMidHitWindow300(),
                                            GameRules::getMaxHitWindow300());
}

f32 BeatmapInterface::getRawOverallDifficultyForSpeedMultiplier(f32 speedMultiplier) {
    return GameRules::mapDifficultyRangeInv((f32)getRawHitWindow300() * (1.0f / speedMultiplier),
                                            GameRules::getMinHitWindow300(), GameRules::getMidHitWindow300(),
                                            GameRules::getMaxHitWindow300());
}

f32 BeatmapInterface::getConstantOverallDifficultyForSpeedMultiplier(f32 speedMultiplier) {
    return GameRules::mapDifficultyRangeInv((f32)getRawHitWindow300() * speedMultiplier,
                                            GameRules::getMinHitWindow300(), GameRules::getMidHitWindow300(),
                                            GameRules::getMaxHitWindow300());
}

LiveScore::HIT BeatmapInterface::getHitResult(i32 delta) {
    if(mod_halfwindow && delta > 0 && delta <= (i32)GameRules::getHitWindowMiss()) {
        if(!mod_halfwindow_allow_300s)
            return LiveScore::HIT::HIT_MISS;
        else if(delta > (i32)getHitWindow300())
            return LiveScore::HIT::HIT_MISS;
    }

    delta = std::abs(delta);

    LiveScore::HIT result = LiveScore::HIT::HIT_NULL;

    if(mod_ming3012) {
        if(delta <= (i32)getHitWindow300())
            result = LiveScore::HIT::HIT_300;
        else if(delta <= (i32)getHitWindow50())
            result = LiveScore::HIT::HIT_50;
        else if(delta <= (i32)GameRules::getHitWindowMiss())
            result = LiveScore::HIT::HIT_MISS;
    } else if(mod_no100s) {
        if(delta <= (i32)getHitWindow300())
            result = LiveScore::HIT::HIT_300;
        else if(delta <= (i32)GameRules::getHitWindowMiss())
            result = LiveScore::HIT::HIT_MISS;
    } else if(mod_no50s) {
        if(delta <= (i32)getHitWindow300())
            result = LiveScore::HIT::HIT_300;
        else if(delta <= (i32)getHitWindow100())
            result = LiveScore::HIT::HIT_100;
        else if(delta <= (i32)GameRules::getHitWindowMiss())
            result = LiveScore::HIT::HIT_MISS;
    } else {
        if(delta <= (i32)getHitWindow300())
            result = LiveScore::HIT::HIT_300;
        else if(delta <= (i32)getHitWindow100())
            result = LiveScore::HIT::HIT_100;
        else if(delta <= (i32)getHitWindow50())
            result = LiveScore::HIT::HIT_50;
        else if(delta <= (i32)GameRules::getHitWindowMiss())
            result = LiveScore::HIT::HIT_MISS;
    }

    return result;
}
