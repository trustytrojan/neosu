//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		handles sounds, bass library wrapper atm
//
// $NoKeywords: $snd
//===============================================================================//

#ifndef SOUNDENGINE_H
#define SOUNDENGINE_H

#include "Sound.h"
#include "cbase.h"

enum class OutputDriver {
    NONE,
    BASS,         // directsound/wasapi non-exclusive mode/alsa
    BASS_WASAPI,  // exclusive mode
    BASS_ASIO,    // exclusive move
};

struct OUTPUT_DEVICE {
    int id;
    bool enabled;
    bool isDefault;
    UString name;
    OutputDriver driver;
};

class SoundEngine {
   public:
    SoundEngine();

    void restart();
    void shutdown();

    void update();

    bool play(Sound *snd, float pan = 0.0f, float pitch = 1.0f);
    void pause(Sound *snd);
    void stop(Sound *snd);

    bool isASIO() { return m_currentOutputDevice.driver == OutputDriver::BASS_ASIO; }
    bool isWASAPI() { return m_currentOutputDevice.driver == OutputDriver::BASS_WASAPI; }
    bool hasExclusiveOutput() { return isASIO() || isWASAPI(); }

    bool setOutputDevice(OUTPUT_DEVICE device);
    void setVolume(float volume);

    OUTPUT_DEVICE getDefaultDevice();
    OUTPUT_DEVICE getWantedDevice();
    std::vector<OUTPUT_DEVICE> getOutputDevices();

    inline const UString &getOutputDeviceName() const { return m_currentOutputDevice.name; }
    inline float getVolume() const { return m_fVolume; }

    void updateOutputDevices(bool printInfo);
    bool initializeOutputDevice(OUTPUT_DEVICE device);

    Sound::SOUNDHANDLE g_bassOutputMixer = 0;
    void onFreqChanged(UString oldValue, UString newValue);

   private:
    std::vector<OUTPUT_DEVICE> m_outputDevices;

    OUTPUT_DEVICE m_currentOutputDevice;

    bool m_bReady = false;
    float m_fVolume = 1.0f;
};

DWORD ASIO_clamp(BASS_ASIO_INFO info, DWORD buflen);

void _RESTART_SOUND_ENGINE_ON_CHANGE(UString oldValue, UString newValue);

#endif
