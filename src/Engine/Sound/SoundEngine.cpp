#include "BassSoundEngine.h"
// #include "SoLoudSoundEngine.h"
#include "SoundEngine.h"

#include "Engine.h"

SoundEngine::SoundEngine() {
    this->currentOutputDevice = {
        .id = 0,
        .enabled = true,
        .isDefault = true,
        .name = "No sound",
        .driver = OutputDriver::NONE,
    };
}

SoundEngine *SoundEngine::createSoundEngine(SndEngineType type) {
#if !defined(MCENGINE_FEATURE_BASS) && !defined(MCENGINE_FEATURE_SOLOUD)
#error No sound backend available!
#endif
#ifdef MCENGINE_FEATURE_BASS
    if(type == BASS) return new BassSoundEngine();
#endif
#ifdef MCENGINE_FEATURE_SOLOUD
    if(type == SOLOUD) return new SoLoudSoundEngine();
#endif
    return nullptr;
}
