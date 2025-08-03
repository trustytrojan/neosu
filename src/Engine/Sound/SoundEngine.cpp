#include "BassSoundEngine.h"
#include "SoLoudSoundEngine.h"
#include "SoundEngine.h"

#include "Engine.h"
#include "ConVar.h"

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

std::vector<SoundEngine::OUTPUT_DEVICE> SoundEngine::getOutputDevices() {
    std::vector<SoundEngine::OUTPUT_DEVICE> outputDevices;

    for(auto &outputDevice : this->outputDevices) {
        if(outputDevice.enabled) {
            outputDevices.push_back(outputDevice);
        }
    }

    return outputDevices;
}

SoundEngine::OUTPUT_DEVICE SoundEngine::getWantedDevice() {
    auto wanted_name = cv::snd_output_device.getString();
    for(auto device : this->outputDevices) {
        if(device.enabled && device.name.utf8View() == wanted_name) {
            return device;
        }
    }

    debugLog("Could not find sound device '{:s}', initializing default one instead.\n", wanted_name);
    return this->getDefaultDevice();
}

SoundEngine::OUTPUT_DEVICE SoundEngine::getDefaultDevice() {
    for(auto device : this->outputDevices) {
        if(device.enabled && device.isDefault) {
            return device;
        }
    }

    debugLog("Could not find a working sound device!\n");
    return {
        .id = 0,
        .enabled = true,
        .isDefault = true,
        .name = "No sound",
        .driver = OutputDriver::NONE,
    };
}
