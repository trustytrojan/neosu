#pragma once

#include "cbase.h"

#define NOBASSOVERLOADS
#include <bass.h>
#include <bass_fx.h>
#include <bassasio.h>
#include <bassloud.h>
#include <bassmix.h>
#include <basswasapi.h>

#include "Resource.h"

class SoundEngine;

class Sound : public Resource {
    friend class SoundEngine;

   public:
    typedef unsigned long SOUNDHANDLE;

   public:
    Sound(std::string filepath, bool stream, bool overlayable, bool loop);
    virtual ~Sound() { this->destroy(); }

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

    inline bool isStream() const { return this->bStream; }
    inline bool isLooped() const { return this->bIsLooped; }
    inline bool isOverlayable() const { return this->bIsOverlayable; }

    void rebuild(std::string newFilePath);

   private:
    virtual void init();
    virtual void initAsync();
    virtual void destroy();

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
