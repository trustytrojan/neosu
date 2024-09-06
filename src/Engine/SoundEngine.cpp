#include "SoundEngine.h"

#include "Bancho.h"
#include "Beatmap.h"
#include "CBaseUILabel.h"
#include "CBaseUISlider.h"
#include "ConVar.h"
#include "Database.h"
#include "Engine.h"
#include "Environment.h"
#include "OptionsMenu.h"
#include "Osu.h"
#include "PauseMenu.h"
#include "Skin.h"
#include "SongBrowser/LoudnessCalcThread.h"
#include "SongBrowser/SongBrowser.h"
#include "Sound.h"
#include "WinEnvironment.h"

// removed from latest headers. not sure if it's handled at all
#ifndef BASS_CONFIG_MP3_OLDGAPS
#define BASS_CONFIG_MP3_OLDGAPS 68
#endif

DWORD ASIO_clamp(BASS_ASIO_INFO info, DWORD buflen) {
    if(buflen == -1) return info.bufpref;
    if(buflen < info.bufmin) return info.bufmin;
    if(buflen > info.bufmax) return info.bufmax;
    if(info.bufgran == 0) return buflen;

    if(info.bufgran == -1) {
        // Buffer lengths are only allowed in powers of 2
        for(int oksize = info.bufmin; oksize <= info.bufmax; oksize *= 2) {
            if(oksize == buflen) {
                return buflen;
            } else if(oksize > buflen) {
                oksize /= 2;
                return oksize;
            }
        }

        // Unreachable
        return info.bufpref;
    } else {
        // Buffer lengths are only allowed in multiples of info.bufgran
        buflen -= info.bufmin;
        buflen = (buflen / info.bufgran) * info.bufgran;
        buflen += info.bufmin;
        return buflen;
    }
}

SoundEngine::SoundEngine() {
    auto bass_version = BASS_GetVersion();
    debugLog("SoundEngine: BASS version = 0x%08x\n", bass_version);
    if(HIWORD(bass_version) != BASSVERSION) {
        engine->showMessageErrorFatal("Fatal Sound Error", "An incorrect version of the BASS library file was loaded!");
        engine->shutdown();
        return;
    }

    auto mixer_version = BASS_Mixer_GetVersion();
    debugLog("SoundEngine: BASSMIX version = 0x%08x\n", mixer_version);
    if(HIWORD(mixer_version) != BASSVERSION) {
        engine->showMessageErrorFatal("Fatal Sound Error",
                                      "An incorrect version of the BASSMIX library file was loaded!");
        engine->shutdown();
        return;
    }

    auto loud_version = BASS_Loudness_GetVersion();
    debugLog("SoundEngine: BASSloud version = 0x%08x\n", loud_version);
    if(HIWORD(loud_version) != BASSVERSION) {
        engine->showMessageErrorFatal("Fatal Sound Error",
                                      "An incorrect version of the BASSloud library file was loaded!");
        engine->shutdown();
        return;
    }

#ifdef _WIN32
    auto asio_version = BASS_ASIO_GetVersion();
    debugLog("SoundEngine: BASSASIO version = 0x%08x\n", asio_version);
    if(HIWORD(asio_version) != BASSASIOVERSION) {
        engine->showMessageErrorFatal("Fatal Sound Error",
                                      "An incorrect version of the BASSASIO library file was loaded!");
        engine->shutdown();
        return;
    }

    auto wasapi_version = BASS_WASAPI_GetVersion();
    debugLog("SoundEngine: BASSWASAPI version = 0x%08x\n", wasapi_version);
    if(HIWORD(wasapi_version) != BASSVERSION) {
        engine->showMessageErrorFatal("Fatal Sound Error",
                                      "An incorrect version of the BASSWASAPI library file was loaded!");
        engine->shutdown();
        return;
    }
#endif

    BASS_SetConfig(BASS_CONFIG_BUFFER, 100);

    // all beatmaps timed to non-iTunesSMPB + 529 sample deletion offsets on old dlls pre 2015
    BASS_SetConfig(BASS_CONFIG_MP3_OLDGAPS, 1);

    // avoids lag/jitter in BASS_Mixer_ChannelGetPosition() shortly after a BASS_ChannelPlay() after loading/silence
    BASS_SetConfig(BASS_CONFIG_DEV_NONSTOP, 1);

    // if set to 1, increases sample playback latency by 10 ms
    BASS_SetConfig(BASS_CONFIG_VISTA_TRUEPOS, 0);

    m_currentOutputDevice = {
        .id = 0,
        .enabled = true,
        .isDefault = true,
        .name = "No sound",
        .driver = OutputDriver::NONE,
    };
}

OUTPUT_DEVICE SoundEngine::getWantedDevice() {
    auto wanted_name = cv_snd_output_device.getString();
    for(auto device : m_outputDevices) {
        if(device.enabled && device.name == wanted_name) {
            return device;
        }
    }

    debugLog("Could not find sound device '%s', initializing default one instead.\n", wanted_name.toUtf8());
    return getDefaultDevice();
}

OUTPUT_DEVICE SoundEngine::getDefaultDevice() {
    for(auto device : m_outputDevices) {
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
        m_outputDevices.push_back(soundDevice);

        debugLog("DSOUND: Device %i = \"%s\", enabled = %i, default = %i\n", d, deviceInfo.name, (int)isEnabled,
                 (int)isDefault);
    }

#ifdef _WIN32
    BASS_ASIO_DEVICEINFO asioDeviceInfo;
    for(int d = 0; (BASS_ASIO_GetDeviceInfo(d, &asioDeviceInfo) == true); d++) {
        OUTPUT_DEVICE soundDevice;
        soundDevice.id = d;
        soundDevice.name = asioDeviceInfo.name;
        soundDevice.enabled = true;
        soundDevice.isDefault = false;

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

        soundDevice.driver = OutputDriver::BASS_ASIO;
        soundDevice.name.append(" [ASIO]");
        m_outputDevices.push_back(soundDevice);

        debugLog("ASIO: Device %i = \"%s\"\n", d, asioDeviceInfo.name);
    }

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

        debugLog("WASAPI: Device %i = \"%s\", enabled = %i, default = %i\n", d, wasapiDeviceInfo.name, (int)isEnabled,
                 (int)isDefault);
    }
#endif
}

void display_bass_error() {
    auto code = BASS_ErrorGetCode();
    switch(code) {
        case BASS_OK:
            break;
        case BASS_ERROR_MEM:
            osu->getNotificationOverlay()->addNotification("BASS error: Memory error");
            break;
        case BASS_ERROR_FILEOPEN:
            osu->getNotificationOverlay()->addNotification("BASS error: Can't open the file");
            break;
        case BASS_ERROR_DRIVER:
            osu->getNotificationOverlay()->addNotification("BASS error: Can't find an available driver");
            break;
        case BASS_ERROR_BUFLOST:
            osu->getNotificationOverlay()->addNotification("BASS error: The sample buffer was lost");
            break;
        case BASS_ERROR_HANDLE:
            osu->getNotificationOverlay()->addNotification("BASS error: Invalid handle");
            break;
        case BASS_ERROR_FORMAT:
            osu->getNotificationOverlay()->addNotification("BASS error: Unsupported sample format");
            break;
        case BASS_ERROR_POSITION:
            osu->getNotificationOverlay()->addNotification("BASS error: Invalid position");
            break;
        case BASS_ERROR_INIT:
            osu->getNotificationOverlay()->addNotification("BASS error: BASS_Init has not been successfully called");
            break;
        case BASS_ERROR_START:
            osu->getNotificationOverlay()->addNotification("BASS error: BASS_Start has not been successfully called");
            break;
        case BASS_ERROR_SSL:
            osu->getNotificationOverlay()->addNotification("BASS error: SSL/HTTPS support isn't available");
            break;
        case BASS_ERROR_REINIT:
            osu->getNotificationOverlay()->addNotification("BASS error: Device needs to be reinitialized");
            break;
        case BASS_ERROR_ALREADY:
            osu->getNotificationOverlay()->addNotification("BASS error: Already initialized");
            break;
        case BASS_ERROR_NOTAUDIO:
            osu->getNotificationOverlay()->addNotification("BASS error: File does not contain audio");
            break;
        case BASS_ERROR_NOCHAN:
            osu->getNotificationOverlay()->addNotification("BASS error: Can't get a free channel");
            break;
        case BASS_ERROR_ILLTYPE:
            osu->getNotificationOverlay()->addNotification("BASS error: An illegal type was specified");
            break;
        case BASS_ERROR_ILLPARAM:
            osu->getNotificationOverlay()->addNotification("BASS error: An illegal parameter was specified");
            break;
        case BASS_ERROR_NO3D:
            osu->getNotificationOverlay()->addNotification("BASS error: No 3D support");
            break;
        case BASS_ERROR_NOEAX:
            osu->getNotificationOverlay()->addNotification("BASS error: No EAX support");
            break;
        case BASS_ERROR_DEVICE:
            osu->getNotificationOverlay()->addNotification("BASS error: Illegal device number");
            break;
        case BASS_ERROR_NOPLAY:
            osu->getNotificationOverlay()->addNotification("BASS error: Not playing");
            break;
        case BASS_ERROR_FREQ:
            osu->getNotificationOverlay()->addNotification("BASS error: Illegal sample rate");
            break;
        case BASS_ERROR_NOTFILE:
            osu->getNotificationOverlay()->addNotification("BASS error: The stream is not a file stream");
            break;
        case BASS_ERROR_NOHW:
            osu->getNotificationOverlay()->addNotification("BASS error: No hardware voices available");
            break;
        case BASS_ERROR_EMPTY:
            osu->getNotificationOverlay()->addNotification("BASS error: The file has no sample data");
            break;
        case BASS_ERROR_NONET:
            osu->getNotificationOverlay()->addNotification("BASS error: No internet connection could be opened");
            break;
        case BASS_ERROR_CREATE:
            osu->getNotificationOverlay()->addNotification("BASS error: Couldn't create the file");
            break;
        case BASS_ERROR_NOFX:
            osu->getNotificationOverlay()->addNotification("BASS error: Effects are not available");
            break;
        case BASS_ERROR_NOTAVAIL:
            osu->getNotificationOverlay()->addNotification("BASS error: Requested data/action is not available");
            break;
        case BASS_ERROR_DECODE:
            osu->getNotificationOverlay()->addNotification("BASS error: The channel is/isn't a decoding channel");
            break;
        case BASS_ERROR_DX:
            osu->getNotificationOverlay()->addNotification("BASS error: A sufficient DirectX version is not installed");
            break;
        case BASS_ERROR_TIMEOUT:
            osu->getNotificationOverlay()->addNotification("BASS error: Connection timeout");
            break;
        case BASS_ERROR_FILEFORM:
            osu->getNotificationOverlay()->addNotification("BASS error: Unsupported file format");
            break;
        case BASS_ERROR_SPEAKER:
            osu->getNotificationOverlay()->addNotification("BASS error: Unavailable speaker");
            break;
        case BASS_ERROR_VERSION:
            osu->getNotificationOverlay()->addNotification("BASS error: Invalid BASS version");
            break;
        case BASS_ERROR_CODEC:
            osu->getNotificationOverlay()->addNotification("BASS error: Codec is not available/supported");
            break;
        case BASS_ERROR_ENDED:
            osu->getNotificationOverlay()->addNotification("BASS error: The channel/file has ended");
            break;
        case BASS_ERROR_BUSY:
            osu->getNotificationOverlay()->addNotification("BASS error: The device is busy");
            break;
        case BASS_ERROR_UNSTREAMABLE:
            osu->getNotificationOverlay()->addNotification("BASS error: Unstreamable file");
            break;
        case BASS_ERROR_PROTOCOL:
            osu->getNotificationOverlay()->addNotification("BASS error: Unsupported protocol");
            break;
        case BASS_ERROR_DENIED:
            osu->getNotificationOverlay()->addNotification("BASS error: Access Denied");
            break;
        case BASS_ERROR_WASAPI:
            osu->getNotificationOverlay()->addNotification("WASAPI error: No WASAPI");
            break;
        case BASS_ERROR_WASAPI_BUFFER:
            osu->getNotificationOverlay()->addNotification("WASAPI error: Invalid buffer size");
            break;
        case BASS_ERROR_WASAPI_CATEGORY:
            osu->getNotificationOverlay()->addNotification("WASAPI error: Can't set category");
            break;
        case BASS_ERROR_WASAPI_DENIED:
            osu->getNotificationOverlay()->addNotification("WASAPI error: Access denied");
            break;
        case BASS_ERROR_UNKNOWN:  // fallthrough
        default:
            osu->getNotificationOverlay()->addNotification("Unknown BASS error (%i)!", code);
            break;
    }
}

// The BASS mixer is used for every sound driver, but it's useful to be able to
// initialize it later on some drivers where we know the best available frequency.
bool SoundEngine::init_bass_mixer(OUTPUT_DEVICE device) {
    auto bass_flags = BASS_DEVICE_STEREO | BASS_DEVICE_FREQ | BASS_DEVICE_NOSPEAKER;
    auto freq = cv_snd_freq.getInt();

    // We initialize a "No sound" device for measuring loudness and mixing sounds,
    // regardless of the device we'll use for actual output.
    if(!BASS_Init(0, freq, bass_flags | BASS_DEVICE_SOFTWARE, NULL, NULL)) {
        auto code = BASS_ErrorGetCode();
        if(code != BASS_ERROR_ALREADY) {
            ready_since = -1.0;
            debugLog("BASS_Init(0) failed.\n");
            display_bass_error();
            return false;
        }
    }

    if(device.driver == OutputDriver::BASS) {
        if(!BASS_Init(device.id, freq, bass_flags | BASS_DEVICE_SOFTWARE, NULL, NULL)) {
            ready_since = -1.0;
            debugLog("BASS_Init(%d) errored out.\n", device.id);
            display_bass_error();
            return false;
        }
    }

    auto mixer_flags = BASS_SAMPLE_FLOAT | BASS_MIXER_NONSTOP | BASS_MIXER_RESUME;
    if(device.driver != OutputDriver::BASS) mixer_flags |= BASS_STREAM_DECODE | BASS_MIXER_POSEX;
    g_bassOutputMixer = BASS_Mixer_StreamCreate(freq, 2, mixer_flags);
    if(g_bassOutputMixer == 0) {
        ready_since = -1.0;
        debugLog("BASS_Mixer_StreamCreate() failed.\n");
        display_bass_error();
        return false;
    }

    // Disable buffering to lower latency on regular BASS output
    // This has no effect on ASIO/WASAPI since for those the mixer is a decode stream
    BASS_ChannelSetAttribute(g_bassOutputMixer, BASS_ATTRIB_BUFFER, 0.f);

    // Switch to "No sound" device for all future sound processing
    // Only g_bassOutputMixer will be output to the actual device!
    BASS_SetDevice(0);
    return true;
}

bool SoundEngine::initializeOutputDevice(OUTPUT_DEVICE device) {
    debugLog("SoundEngine: initializeOutputDevice( %s ) ...\n", device.name.toUtf8());

    shutdown();

    // We compensate for latency via BASS_ATTRIB_MIXER_LATENCY
    cv_universal_offset_hardcoded.setValue(0.f);

    if(device.driver == OutputDriver::NONE || (device.driver == OutputDriver::BASS && device.id == 0)) {
        ready_since = -1.0;
        m_currentOutputDevice = device;
        cv_snd_output_device.setValue(m_currentOutputDevice.name);
        debugLog("SoundEngine: Output Device = \"%s\"\n", m_currentOutputDevice.name.toUtf8());

        if(osu && osu->m_optionsMenu) {
            osu->m_optionsMenu->updateLayout();
        }

        return true;
    }

    if(device.driver == OutputDriver::BASS) {
        // Normal output: render playback buffer every 5ms (buffer is 30ms large)
        BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 5);
        BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 1);
    } else if(device.driver == OutputDriver::BASS_ASIO || device.driver == OutputDriver::BASS_WASAPI) {
        // ASIO/WASAPI: let driver decide when to render playback buffer
        BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
        BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 0);
    }

    // allow users to override some defaults (but which may cause beatmap desyncs)
    // we only want to set these if their values have been explicitly modified (to avoid sideeffects in the default
    // case, and for my sanity)
    {
        if(cv_snd_updateperiod.getFloat() != cv_snd_updateperiod.getDefaultFloat())
            BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, cv_snd_updateperiod.getInt());

        if(cv_snd_dev_buffer.getFloat() != cv_snd_dev_buffer.getDefaultFloat())
            BASS_SetConfig(BASS_CONFIG_DEV_BUFFER, cv_snd_dev_buffer.getInt());

        if(cv_snd_dev_period.getFloat() != cv_snd_dev_period.getDefaultFloat())
            BASS_SetConfig(BASS_CONFIG_DEV_PERIOD, cv_snd_dev_period.getInt());
    }

    // When the driver is BASS, we can init the mixer immediately
    // On other drivers, we'd rather get the sound card's frequency first
    if(device.driver == OutputDriver::BASS) {
        if(!init_bass_mixer(device)) {
            return false;
        }
    }

#ifdef _WIN32
    if(device.driver == OutputDriver::BASS_ASIO) {
        if(!BASS_ASIO_Init(device.id, 0)) {
            ready_since = -1.0;
            debugLog("BASS_ASIO_Init() failed.\n");
            display_bass_error();
            return false;
        }

        double sample_rate = BASS_ASIO_GetRate();
        if(sample_rate == 0.0) {
            sample_rate = cv_snd_freq.getFloat();
            debugLog("ASIO: BASS_ASIO_GetRate() returned 0, using %f instead!\n", sample_rate);
        } else {
            cv_snd_freq.setValue(sample_rate);
        }
        if(!init_bass_mixer(device)) {
            return false;
        }

        BASS_ASIO_INFO info = {0};
        BASS_ASIO_GetInfo(&info);
        auto bufsize = cv_asio_buffer_size.getInt();
        bufsize = ASIO_clamp(info, bufsize);

        if(osu && osu->m_optionsMenu) {
            auto slider = osu->m_optionsMenu->m_asioBufferSizeSlider;
            slider->setBounds(info.bufmin, info.bufmax);
            slider->setKeyDelta(info.bufgran == -1 ? info.bufmin : info.bufgran);
        }

        if(!BASS_ASIO_ChannelEnableBASS(false, 0, g_bassOutputMixer, true)) {
            ready_since = -1.0;
            debugLog("BASS_ASIO_ChannelEnableBASS() failed.\n");
            display_bass_error();
            return false;
        }

        if(!BASS_ASIO_Start(bufsize, 0)) {
            ready_since = -1.0;
            debugLog("BASS_ASIO_Start() failed.\n");
            display_bass_error();
            return false;
        }

        double wanted_latency = 1000.0 * cv_asio_buffer_size.getFloat() / sample_rate;
        double actual_latency = 1000.0 * (double)BASS_ASIO_GetLatency(false) / sample_rate;
        BASS_ChannelSetAttribute(g_bassOutputMixer, BASS_ATTRIB_MIXER_LATENCY, actual_latency / 1000.0);
        debugLog("ASIO: wanted %f ms, got %f ms latency. Sample rate: %f Hz\n", wanted_latency, actual_latency,
                 sample_rate);
    }

    if(device.driver == OutputDriver::BASS_WASAPI) {
        const float bufferSize =
            std::round(cv_win_snd_wasapi_buffer_size.getFloat() * 1000.0f) / 1000.0f;  // in seconds
        const float updatePeriod =
            std::round(cv_win_snd_wasapi_period_size.getFloat() * 1000.0f) / 1000.0f;  // in seconds

        BASS_WASAPI_DEVICEINFO info;
        if(!BASS_WASAPI_GetDeviceInfo(device.id, &info)) {
            debugLog("WASAPI: Failed to get device info\n");
            return false;
        }
        cv_snd_freq.setValue(info.mixfreq);
        if(!init_bass_mixer(device)) {
            return false;
        }

        // BASS_MIXER_NONSTOP prevents some sound cards from going to sleep when there is no output
        auto flags = BASS_MIXER_NONSTOP;

#ifdef _WIN64
        // BASS_WASAPI_RAW ignores sound "enhancements" that some sound cards offer (adds latency)
        // It is only available on Windows 8.1 or above
        flags |= BASS_WASAPI_RAW;
#endif

        if(cv_win_snd_wasapi_exclusive.getBool()) {
            // BASS_WASAPI_EXCLUSIVE makes neosu have exclusive output to the sound card
            // BASS_WASAPI_AUTOFORMAT chooses the best matching sample format, BASSWASAPI doesn't resample in exclusive
            // mode
            flags |= BASS_WASAPI_EXCLUSIVE | BASS_WASAPI_AUTOFORMAT;
        }

        if(!BASS_WASAPI_Init(device.id, 0, 0, flags, bufferSize, updatePeriod, WASAPIPROC_BASS,
                             (void *)g_bassOutputMixer)) {
            const int errorCode = BASS_ErrorGetCode();
            ready_since = -1.0;
            debugLog("BASS_WASAPI_Init() failed.\n");
            display_bass_error();
            return false;
        }

        if(!BASS_WASAPI_Start()) {
            ready_since = -1.0;
            debugLog("BASS_WASAPI_Start() failed.\n");
            display_bass_error();
            return false;
        }

        BASS_ChannelSetAttribute(g_bassOutputMixer, BASS_ATTRIB_MIXER_LATENCY,
                                 cv_win_snd_wasapi_buffer_size.getFloat());
    }
#endif

    ready_since = engine->getTime();
    m_currentOutputDevice = device;
    cv_snd_output_device.setValue(m_currentOutputDevice.name);
    debugLog("SoundEngine: Output Device = \"%s\"\n", m_currentOutputDevice.name.toUtf8());

    if(osu && osu->m_optionsMenu) {
        osu->m_optionsMenu->updateLayout();
    }

    return true;
}

void SoundEngine::restart() { setOutputDevice(m_currentOutputDevice); }

void SoundEngine::shutdown() {
    loct_abort();

    if(m_currentOutputDevice.driver == OutputDriver::BASS) {
        BASS_SetDevice(m_currentOutputDevice.id);
        BASS_Free();
        BASS_SetDevice(0);
    }

#ifdef _WIN32
    if(m_currentOutputDevice.driver == OutputDriver::BASS_ASIO) {
        BASS_ASIO_Free();
    } else if(m_currentOutputDevice.driver == OutputDriver::BASS_WASAPI) {
        BASS_WASAPI_Free();
    }
#endif

    ready_since = -1.0;
    g_bassOutputMixer = 0;
    BASS_Free();  // free "No sound" device
}

void SoundEngine::update() {}

bool SoundEngine::play(Sound *snd, float pan, float pitch) {
    if(!isReady() || snd == NULL || !snd->isReady()) return false;

    if(!snd->isOverlayable() && snd->isPlaying()) {
        return false;
    }

    if(snd->isOverlayable() && cv_snd_restrict_play_frame.getBool()) {
        if(engine->getTime() <= snd->m_fLastPlayTime) {
            return false;
        }
    }

    HCHANNEL channel = snd->getChannel();
    if(channel == 0) {
        debugLog("SoundEngine::play() failed to get channel, errorcode %i\n", BASS_ErrorGetCode());
        return false;
    }

    pan = clamp<float>(pan, -1.0f, 1.0f);
    pitch = clamp<float>(pitch, 0.0f, 2.0f);
    BASS_ChannelSetAttribute(channel, BASS_ATTRIB_VOL, snd->m_fVolume);
    BASS_ChannelSetAttribute(channel, BASS_ATTRIB_PAN, pan);
    BASS_ChannelSetAttribute(channel, BASS_ATTRIB_NORAMP, snd->isStream() ? 0 : 1);
    if(pitch != 1.0f) {
        const float semitonesShift = lerp<float>(-60.0f, 60.0f, pitch / 2.0f);
        float freq = cv_snd_freq.getFloat();
        BASS_ChannelGetAttribute(channel, BASS_ATTRIB_FREQ, &freq);
        BASS_ChannelSetAttribute(channel, BASS_ATTRIB_FREQ, pow(2.0f, (semitonesShift / 12.0f)) * freq);
    }

    BASS_ChannelFlags(channel, snd->isLooped() ? BASS_SAMPLE_LOOP : 0, BASS_SAMPLE_LOOP);

    if(BASS_Mixer_ChannelGetMixer(channel) != 0) return false;

    auto flags = BASS_MIXER_DOWNMIX | BASS_MIXER_NORAMPIN | BASS_STREAM_AUTOFREE;
    if(!BASS_Mixer_StreamAddChannel(g_bassOutputMixer, channel, flags)) {
        debugLog("BASS_Mixer_StreamAddChannel() failed (%i)!\n", BASS_ErrorGetCode());
        return false;
    }

    // Make sure the mixer is playing! Duh.
    if(m_currentOutputDevice.driver == OutputDriver::BASS) {
        if(BASS_ChannelIsActive(g_bassOutputMixer) != BASS_ACTIVE_PLAYING) {
            if(!BASS_ChannelPlay(g_bassOutputMixer, true)) {
                debugLog("SoundEngine::play() couldn't BASS_ChannelPlay(), errorcode %i\n", BASS_ErrorGetCode());
                return false;
            }
        }
    }

    snd->m_bStarted = true;
    snd->m_fChannelCreationTime = engine->getTime();
    if(snd->m_bPaused) {
        snd->m_bPaused = false;
        snd->m_fLastPlayTime = snd->m_fChannelCreationTime - (((f64)snd->m_paused_position_ms) / 1000.0);
    } else {
        snd->m_fLastPlayTime = snd->m_fChannelCreationTime;
    }

    if(cv_debug.getBool()) {
        debugLog("Playing %s\n", snd->getFilePath().c_str());
    }

    return true;
}

void SoundEngine::pause(Sound *snd) {
    if(!isReady() || snd == NULL || !snd->isReady()) return;
    if(!snd->isStream()) {
        engine->showMessageError("Programmer Error", "Called pause on a sample!");
        return;
    }
    if(!snd->isPlaying()) return;

    auto pan = snd->getPan();
    auto pos = snd->getPositionMS();
    auto loop = snd->isLooped();
    auto speed = snd->getSpeed();

    // Calling BASS_Mixer_ChannelRemove automatically frees the stream due
    // to BASS_STREAM_AUTOFREE. We need to reinitialize it.
    snd->reload();

    snd->setPositionMS(pos);
    snd->setSpeed(speed);
    snd->setPan(pan);
    snd->setLoop(loop);
    snd->m_bPaused = true;
    snd->m_paused_position_ms = pos;
}

void SoundEngine::stop(Sound *snd) {
    if(!isReady() || snd == NULL || !snd->isReady()) return;

    // This will stop all samples, then re-init to be ready for a play()
    snd->reload();
}

bool SoundEngine::isReady() {
    if(ready_since == -1.0) return false;
    return ready_since + (double)cv_snd_ready_delay.getFloat() < engine->getTime();
}

bool SoundEngine::hasExclusiveOutput() { return isASIO() || (isWASAPI() && cv_win_snd_wasapi_exclusive.getBool()); }

void SoundEngine::setOutputDevice(OUTPUT_DEVICE device) {
    bool was_playing = false;
    unsigned long prevMusicPositionMS = 0;
    if(osu->getSelectedBeatmap()->getMusic() != NULL) {
        was_playing = osu->getSelectedBeatmap()->getMusic()->isPlaying();
        prevMusicPositionMS = osu->getSelectedBeatmap()->getMusic()->getPositionMS();
    }

    // TODO: This is blocking main thread, can freeze for a long time on some sound cards
    auto previous = m_currentOutputDevice;
    if(!initializeOutputDevice(device)) {
        if((device.id == previous.id && device.driver == previous.driver) || !initializeOutputDevice(previous)) {
            // We failed to reinitialize the device, don't start an infinite loop, just give up
            m_currentOutputDevice = {
                .id = 0,
                .enabled = true,
                .isDefault = true,
                .name = "No sound",
                .driver = OutputDriver::NONE,
            };
        }
    }

    osu->m_optionsMenu->m_outputDeviceLabel->setText(getOutputDeviceName());
    osu->getSkin()->reloadSounds();
    osu->m_optionsMenu->onOutputDeviceResetUpdate();

    // start playing music again after audio device changed
    if(osu->getSelectedBeatmap()->getMusic() != NULL) {
        if(osu->isInPlayMode()) {
            osu->getSelectedBeatmap()->unloadMusic();
            osu->getSelectedBeatmap()->loadMusic(false);
            osu->getSelectedBeatmap()->getMusic()->setLoop(false);
            osu->getSelectedBeatmap()->getMusic()->setPositionMS(prevMusicPositionMS);
        } else {
            osu->getSelectedBeatmap()->unloadMusic();
            osu->getSelectedBeatmap()->select();
            osu->getSelectedBeatmap()->getMusic()->setPositionMS(prevMusicPositionMS);
        }
    }

    // resume loudness calc
    loct_calc(db->m_loudness_to_calc);

    if(was_playing) {
        osu->music_unpause_scheduled = true;
    }
}

void SoundEngine::setVolume(float volume) {
    if(!isReady()) return;

    m_fVolume = clamp<float>(volume, 0.0f, 1.0f);
    if(m_currentOutputDevice.driver == OutputDriver::BASS_ASIO) {
#ifdef _WIN32
        BASS_ASIO_ChannelSetVolume(false, 0, m_fVolume);
        BASS_ASIO_ChannelSetVolume(false, 1, m_fVolume);
#endif
    } else if(m_currentOutputDevice.driver == OutputDriver::BASS_WASAPI) {
#ifdef _WIN32
        if(hasExclusiveOutput()) {
            // Device volume doesn't seem to work, so we'll use DSP instead
            BASS_ChannelSetAttribute(g_bassOutputMixer, BASS_ATTRIB_VOLDSP, m_fVolume);
        } else {
            BASS_WASAPI_SetVolume(BASS_WASAPI_CURVE_WINDOWS | BASS_WASAPI_VOL_SESSION, m_fVolume);
        }
#endif
    } else {
        // 0 (silent) - 10000 (full).
        BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE, (DWORD)(m_fVolume * 10000));
        BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, (DWORD)(m_fVolume * 10000));
        BASS_SetConfig(BASS_CONFIG_GVOL_MUSIC, (DWORD)(m_fVolume * 10000));
    }
}

void SoundEngine::onFreqChanged(UString oldValue, UString newValue) {
    (void)oldValue;
    (void)newValue;
    if(!isReady()) return;
    restart();
}

std::vector<OUTPUT_DEVICE> SoundEngine::getOutputDevices() {
    std::vector<OUTPUT_DEVICE> outputDevices;

    for(size_t i = 0; i < m_outputDevices.size(); i++) {
        if(m_outputDevices[i].enabled) {
            outputDevices.push_back(m_outputDevices[i]);
        }
    }

    return outputDevices;
}
