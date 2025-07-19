//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		SoLoud-specific sound implementation
//
// $NoKeywords: $snd $soloud
//================================================================================//

#pragma once
#ifndef SOLOUD_SOUND_H
#define SOLOUD_SOUND_H

#include "Sound.h"
#ifdef MCENGINE_FEATURE_SOLOUD

#include <memory>

// fwd decls to avoid include external soloud headers here
namespace SoLoud {
class Soloud;
class AudioSource;
class SLFXStream;
}  // namespace SoLoud

// defined in SoLoudSoundEngine, soloud instance singleton pointer
extern std::unique_ptr<SoLoud::Soloud> soloud;

class SoLoudSound final : public Sound {
    friend class SoLoudSoundEngine;

   public:
    SoLoudSound(std::string filepath, bool stream, bool overlayable, bool loop)
        : Sound(std::move(filepath), stream, overlayable, loop) {}
    ~SoLoudSound() override;

    SoLoudSound &operator=(const SoLoudSound &) = delete;
    SoLoudSound &operator=(SoLoudSound &&) = delete;

    SoLoudSound(const SoLoudSound &) = delete;
    SoLoudSound(SoLoudSound &&) = delete;

    // Sound interface implementation
    u32 setPosition(f64 percent) override;
    void setPositionMS(unsigned long ms) override;
    void setVolume(float volume) override;
    void setSpeed(float speed) override;
    void setPitch(float pitch) override;
    void setFrequency(float frequency) override;
    void setPan(float pan) override;
    void setLoop(bool loop) override;

    float getPosition() override;
    u32 getPositionMS() override;
    u32 getLengthMS() override;
    float getSpeed() override;
    float getPitch() override;
    inline float getFrequency() override { return this->fFrequency; }

    bool isPlaying() override;
    bool isFinished() override;

    void rebuild(std::string newFilePath) override;

    // inspection
    SOUND_TYPE(SoLoudSound, SOLOUD, Sound)
   private:
    void init() override;
    void initAsync() override;
    void destroy() override;

    void setOverlayable(bool overlayable);
    SOUNDHANDLE getHandle();

    // helpers to access Wav/SLFXStream internals
    [[nodiscard]] double getSourceLengthInSeconds() const;
    [[nodiscard]] double getStreamPositionInSeconds() const;

    // current playback parameters
    float fFrequency{44100.0f};  // sample rate in Hz
    bool bIsLoopingActuallySet{false};  // looping has been set on the soloud audio handle

    // SoLoud-specific members
    SoLoud::AudioSource *audioSource{nullptr};  // base class pointer, could be either SLFXStream or Wav
    SOUNDHANDLE handle{0};                      // current voice (i.e. "Sound") handle
};

#else
class SoLoudSound : public Sound {};
#endif
#endif
