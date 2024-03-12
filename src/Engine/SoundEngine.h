//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		handles sounds, bass library wrapper atm
//
// $NoKeywords: $snd
//===============================================================================//

#ifndef SOUNDENGINE_H
#define SOUNDENGINE_H

#include "cbase.h"
#include "Sound.h"

enum class OutputDriver {
    NONE,
    BASS,
    BASS_WASAPI,
    BASS_ASIO,
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
    ~SoundEngine();

    void restart();

    void update();

    bool play(Sound *snd, float pan = 0.0f, float pitch = 1.0f);
    bool play3d(Sound *snd, Vector3 pos);
    void pause(Sound *snd);
    void stop(Sound *snd);

    bool isASIO() { return m_currentOutputDevice.driver == OutputDriver::BASS_ASIO; }
    bool isWASAPI() { return m_currentOutputDevice.driver == OutputDriver::BASS_WASAPI; }
    bool isMixing() { return m_currentOutputDevice.driver == OutputDriver::BASS_ASIO || m_currentOutputDevice.driver == OutputDriver::BASS_WASAPI; }

    void setOnOutputDeviceChange(std::function<void()> callback);

    bool setOutputDevice(OUTPUT_DEVICE device);
    void setVolume(float volume);
    void set3dPosition(Vector3 headPos, Vector3 viewDir, Vector3 viewUp);

    OUTPUT_DEVICE getDefaultDevice();
    OUTPUT_DEVICE getWantedDevice();
    std::vector<OUTPUT_DEVICE> getOutputDevices();

    inline const UString &getOutputDeviceName() const { return m_currentOutputDevice.name; }
    inline float getVolume() const { return m_fVolume; }

    void updateOutputDevices(bool printInfo);
    bool initializeOutputDevice(OUTPUT_DEVICE device);

    Sound::SOUNDHANDLE g_bassOutputMixer = 0;

   private:
    void onFreqChanged(UString oldValue, UString newValue);

    std::function<void()> m_outputDeviceChangeCallback = nullptr;
    std::vector<OUTPUT_DEVICE> m_outputDevices;

    OUTPUT_DEVICE m_currentOutputDevice;

    bool m_bReady = false;
    float m_fVolume = 1.0f;
};

DWORD ASIO_clamp(BASS_ASIO_INFO info, DWORD buflen);

void _RESTART_SOUND_ENGINE_ON_CHANGE(UString oldValue, UString newValue);

#endif
