//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		handles sounds, bass library wrapper atm
//
// $NoKeywords: $snd
//===============================================================================//

// TODO: async audio implementation still needs changes in Sound class playing-state handling
// TODO: async audio thread needs proper delay timing
// TODO: finish dynamic audio device updating, but can only do async due to potential lag, disabled by default

#include "SoundEngine.h"

#include <bass.h>
#include <bassmix.h>
#include <basswasapi.h>

#include "Bancho.h"
#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"
#include "HorizonSDLEnvironment.h"
#include "Osu.h"
#include "OsuOptionsMenu.h"
#include "Sound.h"
#include "Thread.h"
#include "WinEnvironment.h"

// removed from latest headers. not sure if it's handled at all
#ifndef BASS_CONFIG_MP3_OLDGAPS
#define BASS_CONFIG_MP3_OLDGAPS 68
#endif

Sound::SOUNDHANDLE g_wasapiOutputMixer = 0;

ConVar snd_output_device("snd_output_device", "Default", FCVAR_NONE);
ConVar snd_restart("snd_restart");

ConVar snd_freq("snd_freq", 44100, FCVAR_NONE, "output sampling rate in Hz");
ConVar snd_updateperiod("snd_updateperiod", 10, FCVAR_NONE, "BASS_CONFIG_UPDATEPERIOD length in milliseconds");
ConVar snd_dev_period("snd_dev_period", 10, FCVAR_NONE,
                      "BASS_CONFIG_DEV_PERIOD length in milliseconds, or if negative then in samples");
ConVar snd_dev_buffer("snd_dev_buffer", 30, FCVAR_NONE, "BASS_CONFIG_DEV_BUFFER length in milliseconds");
ConVar snd_chunk_size("snd_chunk_size", 256, FCVAR_NONE, "only used in horizon builds with sdl mixer audio");

ConVar snd_restrict_play_frame(
    "snd_restrict_play_frame", true, FCVAR_NONE,
    "only allow one new channel per frame for overlayable sounds (prevents lag and earrape)");
ConVar snd_change_check_interval("snd_change_check_interval", 0.0f, FCVAR_NONE,
                                 "check for output device changes every this many seconds. 0 = disabled (default)");

ConVar win_snd_wasapi_buffer_size(
    "win_snd_wasapi_buffer_size", 0.011f, FCVAR_NONE,
    "buffer size/length in seconds (e.g. 0.011 = 11 ms), directly responsible for audio delay and crackling",
    _WIN_SND_WASAPI_BUFFER_SIZE_CHANGE);
ConVar win_snd_wasapi_period_size(
    "win_snd_wasapi_period_size", 0.0f, FCVAR_NONE,
    "interval between OutputWasapiProc calls in seconds (e.g. 0.016 = 16 ms) (0 = use default)",
    _WIN_SND_WASAPI_PERIOD_SIZE_CHANGE);
ConVar win_snd_wasapi_exclusive("win_snd_wasapi_exclusive", false, FCVAR_NONE,
                                "whether to use exclusive device mode to further reduce latency",
                                _WIN_SND_WASAPI_EXCLUSIVE_CHANGE);

ConVar osu_universal_offset_hardcoded("osu_universal_offset_hardcoded", 0.0f, FCVAR_NONE);

DWORD CALLBACK OutputWasapiProc(void *buffer, DWORD length, void *user) {
    if(g_wasapiOutputMixer != 0) {
        const int c = BASS_ChannelGetData(g_wasapiOutputMixer, buffer, length);

        if(c < 0) return 0;

        return c;
    }

    return 0;
}

void _WIN_SND_WASAPI_BUFFER_SIZE_CHANGE(UString oldValue, UString newValue) {
    const int oldValueMS = std::round(oldValue.toFloat() * 1000.0f);
    const int newValueMS = std::round(newValue.toFloat() * 1000.0f);

    if(oldValueMS != newValueMS) engine->getSound()->restart();
}

void _WIN_SND_WASAPI_PERIOD_SIZE_CHANGE(UString oldValue, UString newValue) {
    const int oldValueMS = std::round(oldValue.toFloat() * 1000.0f);
    const int newValueMS = std::round(newValue.toFloat() * 1000.0f);

    if(oldValueMS != newValueMS) engine->getSound()->restart();
}

void _WIN_SND_WASAPI_EXCLUSIVE_CHANGE(UString oldValue, UString newValue) {
    const bool oldValueBool = oldValue.toInt();
    const bool newValueBool = newValue.toInt();

    if(oldValueBool != newValueBool) engine->getSound()->restart();
}

SoundEngine::SoundEngine() {
    auto bass_version = BASS_GetVersion();
    debugLog("SoundEngine: BASS version = 0x%08x\n", bass_version);
    if(HIWORD(bass_version) != BASSVERSION) {
        engine->showMessageErrorFatal("Fatal Sound Error", "An incorrect version of the BASS library file was loaded!");
        engine->shutdown();
        return;
    }

    BASS_SetConfig(BASS_CONFIG_BUFFER, 100);
    BASS_SetConfig(BASS_CONFIG_NET_BUFFER, 500);

    // all beatmaps timed to non-iTunesSMPB + 529 sample deletion offsets on old dlls pre 2015
    BASS_SetConfig(BASS_CONFIG_MP3_OLDGAPS, 1);

    // avoids lag/jitter in BASS_ChannelGetPosition() shortly after a BASS_ChannelPlay() after loading/silence
    BASS_SetConfig(BASS_CONFIG_DEV_NONSTOP, 1);

    // if set to 1, increases sample playback latency by 10 ms
    BASS_SetConfig(BASS_CONFIG_VISTA_TRUEPOS, 0);

    // add default output device
    OUTPUT_DEVICE defaultOutputDevice;
    defaultOutputDevice.id = -1;
    defaultOutputDevice.name = "Default";
    defaultOutputDevice.enabled = true;
    defaultOutputDevice.isDefault = false;  // custom -1 can never have default
    defaultOutputDevice.driver = OutputDriver::BASS;

    snd_output_device.setValue(defaultOutputDevice.name);
    m_outputDevices.push_back(defaultOutputDevice);

    // add all other output devices
    updateOutputDevices(true);

    if(!initializeOutputDevice(defaultOutputDevice)) {
        m_currentOutputDevice = {
            .id = 0,
            .enabled = false,
            .isDefault = false,
            .name = "No sound",
            .driver = OutputDriver::NONE,
        };
    }

    // convar callbacks
    snd_freq.setCallback(fastdelegate::MakeDelegate(this, &SoundEngine::onFreqChanged));
    snd_restart.setCallback(fastdelegate::MakeDelegate(this, &SoundEngine::restart));
    snd_output_device.setCallback(fastdelegate::MakeDelegate(this, &SoundEngine::setOutputDevice));
}

void SoundEngine::updateOutputDevices(bool printInfo) {
    m_outputDevices.clear();

    BASS_DEVICEINFO deviceInfo;
    for(int d = 0; (BASS_GetDeviceInfo(d, &deviceInfo) == true); d++) {
        const bool isEnabled = (deviceInfo.flags & BASS_DEVICE_ENABLED);
        const bool isDefault = (deviceInfo.flags & BASS_DEVICE_DEFAULT);

        OUTPUT_DEVICE soundDevice;
        soundDevice.id = d;
        soundDevice.name = deviceInfo.name;
        soundDevice.enabled = isEnabled;
        soundDevice.isDefault = isDefault;

        // avoid duplicate names
        int duplicateNameCounter = 2;
        while(true) {
            bool foundDuplicateName = false;
            for(size_t i = 0; i < m_outputDevices.size(); i++) {
                if(m_outputDevices[i].name == soundDevice.name) {
                    foundDuplicateName = true;

                    soundDevice.name = deviceInfo.name;
                    soundDevice.name.append(UString::format(" (%i)", duplicateNameCounter));

                    duplicateNameCounter++;

                    break;
                }
            }

            if(!foundDuplicateName) break;
        }

        soundDevice.driver = OutputDriver::BASS;
        if(env->getOS() == Environment::OS::OS_WINDOWS && soundDevice.name != UString("No sound")) {
            soundDevice.name.append(" [DirectSound]");
        }

        m_outputDevices.push_back(soundDevice);

        debugLog("SoundEngine: Device %i = \"%s\", enabled = %i, default = %i\n", d, deviceInfo.name, (int)isEnabled,
                 (int)isDefault);
    }

#ifdef _WIN32
    BASS_WASAPI_DEVICEINFO wasapiDeviceInfo;
    for(int d = 0; (BASS_WASAPI_GetDeviceInfo(d, &wasapiDeviceInfo) == true); d++) {
        const bool isEnabled = (wasapiDeviceInfo.flags & BASS_DEVICE_ENABLED);
        const bool isDefault = (wasapiDeviceInfo.flags & BASS_DEVICE_DEFAULT);
        const bool isInput = (wasapiDeviceInfo.flags & BASS_DEVICE_INPUT);
        if(isInput) continue;

        OUTPUT_DEVICE soundDevice;
        soundDevice.id = d;
        soundDevice.name = wasapiDeviceInfo.name;
        soundDevice.enabled = isEnabled;
        soundDevice.isDefault = isDefault;

        // avoid duplicate names
        int duplicateNameCounter = 2;
        while(true) {
            bool foundDuplicateName = false;
            for(size_t i = 0; i < m_outputDevices.size(); i++) {
                if(m_outputDevices[i].name == soundDevice.name) {
                    foundDuplicateName = true;

                    soundDevice.name = wasapiDeviceInfo.name;
                    soundDevice.name.append(UString::format(" (%i)", duplicateNameCounter));

                    duplicateNameCounter++;

                    break;
                }
            }

            if(!foundDuplicateName) break;
        }

        soundDevice.driver = OutputDriver::BASS_WASAPI;
        soundDevice.name.append(" [WASAPI]");
        m_outputDevices.push_back(soundDevice);

        debugLog("SoundEngine: Device %i = \"%s\", enabled = %i, default = %i\n", d, wasapiDeviceInfo.name,
                 (int)isEnabled, (int)isDefault);
    }
#endif
}

bool SoundEngine::initializeOutputDevice(OUTPUT_DEVICE device) {
    debugLog("SoundEngine: initializeOutputDevice( %s ) ...\n", device.name.toUtf8());

    if(m_currentOutputDevice.driver == OutputDriver::BASS) {
        BASS_Free();
    } else if(m_currentOutputDevice.driver == OutputDriver::BASS_WASAPI) {
        BASS_Free();
#ifdef _WIN32
        BASS_WASAPI_Free();
#endif
    }

    if(device.driver == OutputDriver::BASS) {
        // NOTE: only used by osu atm (new osu uses 5 instead of 10, but not tested enough for offset problems)
        BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 10);
        BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 1);
    } else if(device.driver == OutputDriver::BASS_WASAPI) {
        BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
        BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 0);
    }

    // allow users to override some defaults (but which may cause beatmap desyncs)
    // we only want to set these if their values have been explicitly modified (to avoid sideeffects in the default
    // case, and for my sanity)
    {
        if(snd_updateperiod.getFloat() != snd_updateperiod.getDefaultFloat())
            BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, snd_updateperiod.getInt());

        if(snd_dev_buffer.getFloat() != snd_dev_buffer.getDefaultFloat())
            BASS_SetConfig(BASS_CONFIG_DEV_BUFFER, snd_dev_buffer.getInt());

        if(snd_dev_period.getFloat() != snd_dev_period.getDefaultFloat())
            BASS_SetConfig(BASS_CONFIG_DEV_PERIOD, snd_dev_period.getInt());
    }

    // dynamic runtime flags
    unsigned int runtimeFlags = BASS_DEVICE_STEREO | BASS_DEVICE_FREQ;
    if(device.driver == OutputDriver::BASS) {
        runtimeFlags |= BASS_DEVICE_DSOUND;
    } else if(device.driver == OutputDriver::BASS_WASAPI) {
        runtimeFlags |= BASS_DEVICE_NOSPEAKER;
    }

    // init
    const int freq = snd_freq.getInt();
    HWND hwnd = NULL;

#ifdef _WIN32
    const WinEnvironment *winEnv = dynamic_cast<WinEnvironment *>(env);
    hwnd = winEnv->getHwnd();
#endif

    if(!BASS_Init(device.driver == OutputDriver::BASS_WASAPI ? 0 : device.id, freq, runtimeFlags, hwnd, NULL)) {
        m_bReady = false;
        engine->showMessageError("Sound Error", UString::format("BASS_Init() failed (%i)!", BASS_ErrorGetCode()));
        return false;
    }

#ifdef _WIN32
    if(device.driver == OutputDriver::BASS_WASAPI) {
        const float bufferSize = std::round(win_snd_wasapi_buffer_size.getFloat() * 1000.0f) / 1000.0f;    // in seconds
        const float updatePeriod = std::round(win_snd_wasapi_period_size.getFloat() * 1000.0f) / 1000.0f;  // in seconds

        debugLog("WASAPI Exclusive Mode = %i, bufferSize = %f, updatePeriod = %f\n",
                 (int)win_snd_wasapi_exclusive.getBool(), bufferSize, updatePeriod);
        if(!BASS_WASAPI_Init(device.id, 0, 0, (win_snd_wasapi_exclusive.getBool() ? BASS_WASAPI_EXCLUSIVE : 0),
                             bufferSize, updatePeriod, OutputWasapiProc, NULL)) {
            m_bReady = false;

            const int errorCode = BASS_ErrorGetCode();

            if(errorCode == BASS_ERROR_WASAPI_BUFFER)
                debugLog("Sound Error: BASS_WASAPI_Init() failed with BASS_ERROR_WASAPI_BUFFER!");
            else
                engine->showMessageError("Sound Error", UString::format("BASS_WASAPI_Init() failed (%i)!", errorCode));

            return false;
        }

        if(!BASS_WASAPI_Start()) {
            m_bReady = false;
            engine->showMessageError("Sound Error",
                                     UString::format("BASS_WASAPI_Start() failed (%i)!", BASS_ErrorGetCode()));
            return false;
        }

        BASS_WASAPI_INFO wasapiInfo;
        BASS_WASAPI_GetInfo(&wasapiInfo);

        g_wasapiOutputMixer = BASS_Mixer_StreamCreate(wasapiInfo.freq, wasapiInfo.chans,
                                                      BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | BASS_MIXER_NONSTOP);

        if(g_wasapiOutputMixer == 0) {
            m_bReady = false;
            engine->showMessageError("Sound Error",
                                     UString::format("BASS_Mixer_StreamCreate() failed (%i)!", BASS_ErrorGetCode()));
            return false;
        }
    }
#endif

    if(device.driver == OutputDriver::BASS) {
        // starting with bass 2020 2.4.15.2 which has all offset problems fixed, this is the non-dsound backend
        // compensation NOTE: this depends on BASS_CONFIG_UPDATEPERIOD/BASS_CONFIG_DEV_BUFFER
        osu_universal_offset_hardcoded.setValue(15.0f);
    } else if(device.driver == OutputDriver::BASS_WASAPI) {
        // since we use the newer bass/fx dlls for wasapi builds anyway (which have different time handling)
        osu_universal_offset_hardcoded.setValue(-25.0f);
    }

    if(env->getOS() == Environment::OS::OS_HORIZON) {
        osu_universal_offset_hardcoded.setValue(-45.0f);
    }

    m_bReady = true;
    m_currentOutputDevice = device;
    debugLog("SoundEngine: Output Device = \"%s\"\n", m_currentOutputDevice.name.toUtf8());

    if(bancho.osu && bancho.osu->m_optionsMenu) {
        bancho.osu->m_optionsMenu->updateLayout();
    }

    return true;
}

SoundEngine::~SoundEngine() {
    if(m_currentOutputDevice.driver == OutputDriver::BASS) {
        BASS_Free();
    } else if(m_currentOutputDevice.driver == OutputDriver::BASS_WASAPI) {
#ifdef _WIN32
        BASS_WASAPI_Free();
#endif
        BASS_Free();
    }
}

void SoundEngine::restart() { initializeOutputDevice(m_currentOutputDevice); }

void SoundEngine::update() {}

bool SoundEngine::play(Sound *snd, float pan, float pitch) {
    if(!m_bReady || snd == NULL || !snd->isReady()) return false;

    pan = clamp<float>(pan, -1.0f, 1.0f);
    pitch = clamp<float>(pitch, 0.0f, 2.0f);

    const bool allowPlayFrame =
        !snd->isOverlayable() || !snd_restrict_play_frame.getBool() || engine->getTime() > snd->getLastPlayTime();

    if(!allowPlayFrame) return false;

    Sound::SOUNDHANDLE handle = snd->getHandle();

    // HACKHACK: force add to output mixer
    if(isWASAPI() && handle != 0) {
#ifdef _WIN32
        if(BASS_Mixer_ChannelGetMixer(handle) == 0) {
            if(!BASS_Mixer_StreamAddChannel(
                   g_wasapiOutputMixer, handle,
                   (!snd->isStream() ? BASS_STREAM_AUTOFREE : 0) | BASS_MIXER_DOWNMIX | BASS_MIXER_NORAMPIN))
                debugLog("BASS_Mixer_StreamAddChannel() failed (%i)!", BASS_ErrorGetCode());
        }
#endif
    }

    if(BASS_ChannelIsActive(handle) != BASS_ACTIVE_PLAYING) {
        BASS_ChannelSetAttribute(handle, BASS_ATTRIB_PAN, pan);
        BASS_ChannelSetAttribute(handle, BASS_ATTRIB_NORAMP, snd->isStream() ? 0 : 1);
        BASS_ChannelFlags(handle, (snd->isStream() && snd->isLooped()) ? BASS_SAMPLE_LOOP : 0, BASS_SAMPLE_LOOP);

        if(pitch != 1.0f) {
            float freq = snd_freq.getFloat();
            BASS_ChannelGetAttribute(handle, BASS_ATTRIB_FREQ, &freq);

            const float semitonesShift = lerp<float>(-60.0f, 60.0f, pitch / 2.0f);
            BASS_ChannelSetAttribute(handle, BASS_ATTRIB_FREQ, std::pow(2.0f, (semitonesShift / 12.0f)) * freq);
        }

        if(!BASS_ChannelPlay(handle, true)) {
            debugLog("SoundEngine::play() couldn't BASS_ChannelPlay(), errorcode %i\n", BASS_ErrorGetCode());
            return false;
        }

        snd->setLastPlayTime(engine->getTime());
        return true;
    }

    return false;
}

bool SoundEngine::play3d(Sound *snd, Vector3 pos) {
    if(!m_bReady || snd == NULL || !snd->isReady() || !snd->is3d()) return false;

    if(!snd_restrict_play_frame.getBool() || engine->getTime() > snd->getLastPlayTime()) {
        Sound::SOUNDHANDLE handle = snd->getHandle();
        if(BASS_ChannelIsActive(handle) != BASS_ACTIVE_PLAYING) {
            BASS_3DVECTOR bassPos = BASS_3DVECTOR(pos.x, pos.y, pos.z);
            if(!BASS_ChannelSet3DPosition(handle, &bassPos, NULL, NULL)) {
                debugLog("SoundEngine::play3d() couldn't BASS_ChannelSet3DPosition(), errorcode %i\n",
                         BASS_ErrorGetCode());
                return false;
            }

            BASS_Apply3D();
            if(!BASS_ChannelPlay(handle, true)) {
                debugLog("SoundEngine::play3d() couldn't BASS_ChannelPlay(), errorcode %i\n", BASS_ErrorGetCode());
                return false;
            }

            snd->setLastPlayTime(engine->getTime());
            return true;
        }
    }

    return false;
}

void SoundEngine::pause(Sound *snd) {
    if(!m_bReady || snd == NULL || !snd->isReady()) return;

#ifdef _WIN32

    Sound::SOUNDHANDLE handle = snd->getHandle();

    if(snd->isStream()) {
        if(snd->isPlaying()) {
            snd->setPrevPosition(snd->getPrevPosition());
            BASS_Mixer_ChannelRemove(handle);
        } else {
            play(snd);
            snd->setPositionMS(snd->getPrevPosition());
        }
    } else {
        if(!BASS_ChannelPause(handle)) {
            debugLog("SoundEngine::pause() couldn't BASS_ChannelPause(), errorcode %i\n", BASS_ErrorGetCode());
        }
    }

#else

    BASS_ChannelPause(snd->getHandle());
    { snd->setLastPlayTime(0.0); }

#endif
}

void SoundEngine::stop(Sound *snd) {
    if(!m_bReady || snd == NULL || !snd->isReady()) return;

    Sound::SOUNDHANDLE handle = snd->getHandle();

    if(m_currentOutputDevice.driver == OutputDriver::BASS_WASAPI) {
#ifdef _WIN32
        if(BASS_Mixer_ChannelGetMixer(handle) != 0) {
            BASS_Mixer_ChannelRemove(handle);
        }
#endif
    }

    BASS_ChannelStop(handle);
    {
        snd->setPosition(0.0);
        snd->setLastPlayTime(0.0);

        // allow next play()/getHandle() to reallocate (because BASS_ChannelStop() will free the channel)
        snd->m_HCHANNEL = 0;
        snd->m_HCHANNELBACKUP = 0;
    }
}

void SoundEngine::setOnOutputDeviceChange(std::function<void()> callback) { m_outputDeviceChangeCallback = callback; }

void SoundEngine::setOutputDevice(UString outputDeviceName) {
    for(auto device : m_outputDevices) {
        if(device.name != outputDeviceName) continue;
        if(device.name == m_currentOutputDevice.name) {
            debugLog("SoundEngine::setOutputDevice() \"%s\" already is the current device.\n",
                     outputDeviceName.toUtf8());
            return;
        }

        if(!initializeOutputDevice(device)) {
            restart();
        }
        return;
    }

    debugLog("SoundEngine::setOutputDevice() couldn't find output device \"%s\"!\n", outputDeviceName.toUtf8());
}

void SoundEngine::setVolume(float volume) {
    if(!m_bReady) return;

    m_fVolume = clamp<float>(volume, 0.0f, 1.0f);

    // 0 (silent) - 10000 (full).
    BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE, (DWORD)(m_fVolume * 10000));
    BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, (DWORD)(m_fVolume * 10000));
    BASS_SetConfig(BASS_CONFIG_GVOL_MUSIC, (DWORD)(m_fVolume * 10000));

#ifdef _WIN32
    if(m_currentOutputDevice.driver == OutputDriver::BASS_WASAPI) {
        BASS_WASAPI_SetVolume(BASS_WASAPI_CURVE_WINDOWS | BASS_WASAPI_VOL_SESSION, m_fVolume);
    }
#endif
}

void SoundEngine::set3dPosition(Vector3 headPos, Vector3 viewDir, Vector3 viewUp) {
    if(!m_bReady) return;

    BASS_3DVECTOR bassHeadPos = BASS_3DVECTOR(headPos.x, headPos.y, headPos.z);
    BASS_3DVECTOR bassViewDir = BASS_3DVECTOR(viewDir.x, viewDir.y, viewDir.z);
    BASS_3DVECTOR bassViewUp = BASS_3DVECTOR(viewUp.x, viewUp.y, viewUp.z);

    if(!BASS_Set3DPosition(&bassHeadPos, NULL, &bassViewDir, &bassViewUp))
        debugLog("SoundEngine::set3dPosition() couldn't BASS_Set3DPosition(), errorcode %i\n", BASS_ErrorGetCode());
    else
        BASS_Apply3D();  // apply the changes
}

void SoundEngine::onFreqChanged(UString oldValue, UString newValue) {
    (void)oldValue;
    (void)newValue;
    restart();
}

std::vector<UString> SoundEngine::getOutputDevices() {
    std::vector<UString> outputDevices;

    for(size_t i = 0; i < m_outputDevices.size(); i++) {
        if(m_outputDevices[i].enabled) outputDevices.push_back(m_outputDevices[i].name);
    }

    return outputDevices;
}

//*********************//
//	Sound ConCommands  //
//*********************//

void _volume(UString oldValue, UString newValue) {
    (void)oldValue;
    engine->getSound()->setVolume(newValue.toFloat());
}

ConVar _volume_("volume", 1.0f, FCVAR_NONE, _volume);
