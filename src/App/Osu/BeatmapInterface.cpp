#include "BeatmapInterface.h"

#include "GameRules.h"

f32 BeatmapInterface::getHitWindow300() {
    return GameRules::mapDifficultyRange(this->getOD(), GameRules::getMinHitWindow300(), GameRules::getMidHitWindow300(),
                                         GameRules::getMaxHitWindow300());
}

f32 BeatmapInterface::getRawHitWindow300() {
    return GameRules::mapDifficultyRange(this->getRawOD(), GameRules::getMinHitWindow300(), GameRules::getMidHitWindow300(),
                                         GameRules::getMaxHitWindow300());
}

f32 BeatmapInterface::getHitWindow100() {
    return GameRules::mapDifficultyRange(this->getOD(), GameRules::getMinHitWindow100(), GameRules::getMidHitWindow100(),
                                         GameRules::getMaxHitWindow100());
}

f32 BeatmapInterface::getHitWindow50() {
    return GameRules::mapDifficultyRange(this->getOD(), GameRules::getMinHitWindow50(), GameRules::getMidHitWindow50(),
                                         GameRules::getMaxHitWindow50());
}

f32 BeatmapInterface::getApproachRateForSpeedMultiplier() {
    return GameRules::mapDifficultyRangeInv((f32)this->getApproachTime() * (1.0f / this->getSpeedMultiplier()),
                                            GameRules::getMinApproachTime(), GameRules::getMidApproachTime(),
                                            GameRules::getMaxApproachTime());
}

f32 BeatmapInterface::getRawApproachRateForSpeedMultiplier() {
    return GameRules::mapDifficultyRangeInv((f32)this->getRawApproachTime() * (1.0f / this->getSpeedMultiplier()),
                                            GameRules::getMinApproachTime(), GameRules::getMidApproachTime(),
                                            GameRules::getMaxApproachTime());
}

f32 BeatmapInterface::getConstantApproachRateForSpeedMultiplier() {
    return GameRules::mapDifficultyRangeInv((f32)this->getRawApproachTime() * this->getSpeedMultiplier(),
                                            GameRules::getMinApproachTime(), GameRules::getMidApproachTime(),
                                            GameRules::getMaxApproachTime());
}

f32 BeatmapInterface::getOverallDifficultyForSpeedMultiplier() {
    return GameRules::mapDifficultyRangeInv((f32)this->getHitWindow300() * (1.0f / this->getSpeedMultiplier()),
                                            GameRules::getMinHitWindow300(), GameRules::getMidHitWindow300(),
                                            GameRules::getMaxHitWindow300());
}

f32 BeatmapInterface::getRawOverallDifficultyForSpeedMultiplier() {
    return GameRules::mapDifficultyRangeInv((f32)this->getRawHitWindow300() * (1.0f / this->getSpeedMultiplier()),
                                            GameRules::getMinHitWindow300(), GameRules::getMidHitWindow300(),
                                            GameRules::getMaxHitWindow300());
}

f32 BeatmapInterface::getConstantOverallDifficultyForSpeedMultiplier() {
    return GameRules::mapDifficultyRangeInv((f32)this->getRawHitWindow300() * this->getSpeedMultiplier(),
                                            GameRules::getMinHitWindow300(), GameRules::getMidHitWindow300(),
                                            GameRules::getMaxHitWindow300());
}

LiveScore::HIT BeatmapInterface::getHitResult(i32 delta) {
    if(this->mod_halfwindow && delta > 0 && delta <= (i32)GameRules::getHitWindowMiss()) {
        if(!this->mod_halfwindow_allow_300s)
            return LiveScore::HIT::HIT_MISS;
        else if(delta > (i32)this->getHitWindow300())
            return LiveScore::HIT::HIT_MISS;
    }

    delta = std::abs(delta);

    LiveScore::HIT result = LiveScore::HIT::HIT_NULL;

    if(this->mod_ming3012) {
        if(delta <= (i32)this->getHitWindow300())
            result = LiveScore::HIT::HIT_300;
        else if(delta <= (i32)this->getHitWindow50())
            result = LiveScore::HIT::HIT_50;
        else if(delta <= (i32)GameRules::getHitWindowMiss())
            result = LiveScore::HIT::HIT_MISS;
    } else if(this->mod_no100s) {
        if(delta <= (i32)this->getHitWindow300())
            result = LiveScore::HIT::HIT_300;
        else if(delta <= (i32)GameRules::getHitWindowMiss())
            result = LiveScore::HIT::HIT_MISS;
    } else if(this->mod_no50s) {
        if(delta <= (i32)this->getHitWindow300())
            result = LiveScore::HIT::HIT_300;
        else if(delta <= (i32)this->getHitWindow100())
            result = LiveScore::HIT::HIT_100;
        else if(delta <= (i32)GameRules::getHitWindowMiss())
            result = LiveScore::HIT::HIT_MISS;
    } else {
        if(delta <= (i32)this->getHitWindow300())
            result = LiveScore::HIT::HIT_300;
        else if(delta <= (i32)this->getHitWindow100())
            result = LiveScore::HIT::HIT_100;
        else if(delta <= (i32)this->getHitWindow50())
            result = LiveScore::HIT::HIT_50;
        else if(delta <= (i32)GameRules::getHitWindowMiss())
            result = LiveScore::HIT::HIT_MISS;
    }

    return result;
}
