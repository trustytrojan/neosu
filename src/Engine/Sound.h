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
    Sound(std::string filepath, bool stream, bool overlayable, bool loop, bool prescan);
    virtual ~Sound() { destroy(); }

    std::vector<HCHANNEL> mixer_channels;
    std::vector<HCHANNEL> getActiveChannels();
    HCHANNEL getChannel();

    void setPosition(double percent);
    void setPositionMS(unsigned long ms);
    void setVolume(float volume);
    void setSpeed(float speed);
    void setFrequency(float frequency);
    void setPan(float pan);
    void setLoop(bool loop);
    void setLastPlayTime(double lastPlayTime) { m_fLastPlayTime = lastPlayTime; }

    float getPosition();
    u32 getPositionMS();
    u32 getLengthMS();
    float getPan() { return m_fPan; }
    float getSpeed();
    float getFrequency();

    inline double getLastPlayTime() const { return m_fLastPlayTime; }

    bool isPlaying();
    bool isFinished();

    inline bool isStream() const { return m_bStream; }
    inline bool isLooped() const { return m_bIsLooped; }
    inline bool isOverlayable() const { return m_bIsOverlayable; }

    void rebuild(std::string newFilePath);

   private:
    virtual void init();
    virtual void initAsync();
    virtual void destroy();

    SOUNDHANDLE m_stream = 0;
    SOUNDHANDLE m_sample = 0;

    bool m_bStarted = false;
    bool m_bPaused = false;
    bool m_bStream;
    bool m_bIsLooped;
    bool m_bPrescan;
    bool m_bIsOverlayable;

    float m_fPan;
    float m_fSpeed;
    float m_fVolume;
    f64 m_fLastPlayTime = 0.0;
    u32 m_length = 0;
};
