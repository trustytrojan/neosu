#pragma once
// Copyright (c) 2025, WH, All rights reserved.
#ifndef SOLOUD_SOUNDENGINE_H
#define SOLOUD_SOUNDENGINE_H

#include "SoundEngine.h"
#ifdef MCENGINE_FEATURE_SOLOUD

#include "UString.h"

#include <map>
#include <memory>

// fwd decls to avoid include external soloud headers here
namespace SoLoud {
class Soloud;
struct DeviceInfo;
}  // namespace SoLoud

class SoLoudSound;

class SoLoudSoundEngine final : public SoundEngine {
   public:
    SoLoudSoundEngine();
    ~SoLoudSoundEngine() override;

    SoLoudSoundEngine &operator=(const SoLoudSoundEngine &) = delete;
    SoLoudSoundEngine &operator=(SoLoudSoundEngine &&) = delete;

    SoLoudSoundEngine(const SoLoudSoundEngine &) = delete;
    SoLoudSoundEngine(SoLoudSoundEngine &&) = delete;

    void restart() override;

    bool play(Sound *snd, float pan = .0f, float pitch = .0f) override;
    void pause(Sound *snd) override;
    void stop(Sound *snd) override;

    inline bool isReady() override { return this->bReady; }

    void setOutputDevice(const OUTPUT_DEVICE &device) override;
    void setVolume(float volume) override;

    void allowInternalCallbacks() override;

    SOUND_ENGINE_TYPE(SoLoudSoundEngine, SOLOUD, SoundEngine)
   private:
    bool playSound(SoLoudSound *soloudSound, float pan, float pitch);
    unsigned int playDirectSound(SoLoudSound *soloudSound, float pan, float pitch, float volume);

    void setVolumeGradual(unsigned int handle, float targetVol, float fadeTimeMs = 10.0f);
    void updateOutputDevices(bool printInfo) override;

    bool initializeOutputDevice(const OUTPUT_DEVICE &device) override;

    void setOutputDeviceByName(const UString &desiredDeviceName);
    bool setOutputDeviceInt(const OUTPUT_DEVICE &desiredDevice, bool force = false);

    int iMaxActiveVoices;
    void onMaxActiveChange(float newMax);

    std::map<int, SoLoud::DeviceInfo> mSoloudDevices;

    bool bReady{false};
    bool bWasBackendEverReady{false};

    // for backend
    static OutputDriver getMAorSDLCV();
};

// raw pointer access to the s_SLInstance singleton, for SoLoudSound to use
extern std::unique_ptr<SoLoud::Soloud> soloud;

#else
class SoLoudSoundEngine : public SoundEngine {};
#endif
#endif
