#pragma once

#include "BassManager.h"
#include "Resource.h"
#include "cbase.h"

class SoundEngine;

class Sound : public Resource {
    friend class SoundEngine;

   public:
    typedef unsigned long SOUNDHANDLE;

   public:
    Sound(std::string filepath, bool stream, bool overlayable, bool loop);
    ~Sound() override { this->destroy(); }

    std::vector<HCHANNEL> mixer_channels;
    std::vector<HCHANNEL> getActiveChannels();
    HCHANNEL getChannel();

    u32 setPosition(f64 percent);
    void setPositionMS(unsigned long ms);
    void setPositionMS_fast(u32 ms);

    void setVolume(float volume);
    void setSpeed(float speed);
    void setFrequency(float frequency);
    void setPan(float pan);
    void setLoop(bool loop);

    float getPosition();
    u32 getPositionMS();
    u32 getLengthMS();
    float getPan() { return this->fPan; }
    float getSpeed();
    float getFrequency();

    bool isPlaying();
    bool isFinished();

    [[nodiscard]] inline bool isStream() const { return this->bStream; }
    [[nodiscard]] inline bool isLooped() const { return this->bIsLooped; }
    [[nodiscard]] inline bool isOverlayable() const { return this->bIsOverlayable; }

    void rebuild(std::string newFilePath);

    // type inspection
    [[nodiscard]] Type getResType() const final { return SOUND; }

    Sound *asSound() final { return this; }
    [[nodiscard]] const Sound *asSound() const final { return this; }

   private:
    void init() override;
    void initAsync() override;
    void destroy() override;

    SOUNDHANDLE stream = 0;
    SOUNDHANDLE sample = 0;

    bool bStarted = false;
    bool bPaused = false;
    bool bStream;
    bool bIsLooped;
    bool bIsOverlayable;

    float fPan;
    float fSpeed;
    float fVolume;
    f64 fLastPlayTime = 0.0;
    f64 fChannelCreationTime = 0.0;
    u32 paused_position_ms = 0;
    u32 length = 0;
};
