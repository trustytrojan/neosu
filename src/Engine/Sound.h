//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		sound wrapper, either streamed or preloaded
//
// $NoKeywords: $snd
//===============================================================================//

#ifndef SOUND_H
#define SOUND_H

#define NOBASSOVERLOADS
#include <bass.h>

#include "Resource.h"

#define MAX_OVERLAPPING_SAMPLES 5

class SoundEngine;

class Sound : public Resource {
    friend class SoundEngine;

   public:
    typedef unsigned long SOUNDHANDLE;

   public:
    Sound(std::string filepath, bool stream, bool overlayable, bool threeD, bool loop, bool prescan);
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
    unsigned long getPositionMS();
    unsigned long getLengthMS();
    float getPan() { return m_fPan; }
    float getSpeed();
    float getFrequency();

    inline double getLastPlayTime() const { return m_fLastPlayTime; }

    bool isPlaying();
    bool isFinished();

    inline bool isStream() const { return m_bStream; }
    inline bool is3d() const { return m_bIs3d; }
    inline bool isLooped() const { return m_bIsLooped; }
    inline bool isOverlayable() const { return m_bIsOverlayable; }

    void rebuild(std::string newFilePath);

   private:
    virtual void init();
    virtual void initAsync();
    virtual void destroy();


    SOUNDHANDLE m_stream = 0;
    SOUNDHANDLE m_sample = 0;

    bool m_bStream;
    bool m_bIs3d;
    bool m_bIsLooped;
    bool m_bPrescan;
    bool m_bIsOverlayable;

    float m_fPan;
    float m_fSpeed;
    float m_fVolume;
    double m_fLastPlayTime;
};

#endif
