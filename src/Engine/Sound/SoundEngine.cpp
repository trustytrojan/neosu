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
#include "ResourceManager.h"
#include "Skin.h"
#include "SongBrowser/LoudnessCalcThread.h"
#include "SongBrowser/SongBrowser.h"
#include "Sound.h"
#include "WinEnvironment.h"

// removed from latest headers. not sure if it's handled at all
#ifndef BASS_CONFIG_MP3_OLDGAPS
#define BASS_CONFIG_MP3_OLDGAPS 68
#endif

#ifdef MCENGINE_PLATFORM_WINDOWS
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
#endif

SoundEngine::SoundEngine() {
    if(!BassManager::init())  // this checks the library versions as well
    {
        engine->showMessageErrorFatal(
            "Fatal Sound Error", UString::fmt("Failed to load BASS feature: {:s} !", BassManager::getFailedLoad()));
        engine->shutdown();
        return;
    }

    BASS_SetConfig(BASS_CONFIG_BUFFER, 100);

    // all beatmaps timed to non-iTunesSMPB + 529 sample deletion offsets on old dlls pre 2015
    BASS_SetConfig(BASS_CONFIG_MP3_OLDGAPS, 1);

    // avoids lag/jitter in BASS_Mixer_ChannelGetPosition() shortly after a BASS_ChannelPlay() after loading/silence
    BASS_SetConfig(BASS_CONFIG_DEV_NONSTOP, 1);

    // if set to 1, increases sample playback latency by 10 ms
    BASS_SetConfig(BASS_CONFIG_VISTA_TRUEPOS, 0);

    this->currentOutputDevice = {
        .id = 0,
        .enabled = true,
        .isDefault = true,
        .name = "No sound",
        .driver = OutputDriver::NONE,
    };
}

OUTPUT_DEVICE SoundEngine::getWantedDevice() {
    auto wanted_name = cv_snd_output_device.getString();
    for(auto device : this->outputDevices) {
        if(device.enabled && device.name.utf8View() == wanted_name) {
            return device;
        }
    }

    debugLogF("Could not find sound device '{:s}', initializing default one instead.\n", wanted_name);
    return this->getDefaultDevice();
}

OUTPUT_DEVICE SoundEngine::getDefaultDevice() {
    for(auto device : this->outputDevices) {
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
    this->outputDevices.clear();

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
            for(size_t i = 0; i < this->outputDevices.size(); i++) {
                if(this->outputDevices[i].name == soundDevice.name) {
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
        this->outputDevices.push_back(soundDevice);

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
            for(size_t i = 0; i < this->outputDevices.size(); i++) {
                if(this->outputDevices[i].name == soundDevice.name) {
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
        this->outputDevices.push_back(soundDevice);

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
            for(size_t i = 0; i < this->outputDevices.size(); i++) {
                if(this->outputDevices[i].name == soundDevice.name) {
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
        this->outputDevices.push_back(soundDevice);

        debugLog("WASAPI: Device %i = \"%s\", enabled = %i, default = %i\n", d, wasapiDeviceInfo.name, (int)isEnabled,
                 (int)isDefault);
    }
#endif
}

// The BASS mixer is used for every sound driver, but it's useful to be able to
// initialize it later on some drivers where we know the best available frequency.
bool SoundEngine::init_bass_mixer(const OUTPUT_DEVICE& device) {
    auto bass_flags = BASS_DEVICE_STEREO | BASS_DEVICE_FREQ | BASS_DEVICE_NOSPEAKER;
    auto freq = cv_snd_freq.getInt();

    // We initialize a "No sound" device for measuring loudness and mixing sounds,
    // regardless of the device we'll use for actual output.
    if(!BASS_Init(0, freq, bass_flags | BASS_DEVICE_SOFTWARE, NULL, NULL)) {
        auto code = BASS_ErrorGetCode();
        if(code != BASS_ERROR_ALREADY) {
            this->ready_since = -1.0;
            debugLog("BASS_Init(0) failed.\n");
            osu->getNotificationOverlay()->addToast(BassManager::getErrorUString());
            return false;
        }
    }

    if(device.driver == OutputDriver::BASS) {
        if(!BASS_Init(device.id, freq, bass_flags | BASS_DEVICE_SOFTWARE, NULL, NULL)) {
            this->ready_since = -1.0;
            debugLog("BASS_Init(%d) errored out.\n", device.id);
            osu->getNotificationOverlay()->addToast(BassManager::getErrorUString());
            return false;
        }
    }

    auto mixer_flags = BASS_SAMPLE_FLOAT | BASS_MIXER_NONSTOP | BASS_MIXER_RESUME;
    if(device.driver != OutputDriver::BASS) mixer_flags |= BASS_STREAM_DECODE | BASS_MIXER_POSEX;
    this->g_bassOutputMixer = BASS_Mixer_StreamCreate(freq, 2, mixer_flags);
    if(this->g_bassOutputMixer == 0) {
        this->ready_since = -1.0;
        debugLog("BASS_Mixer_StreamCreate() failed.\n");
        osu->getNotificationOverlay()->addToast(BassManager::getErrorUString());
        return false;
    }

    // Disable buffering to lower latency on regular BASS output
    // This has no effect on ASIO/WASAPI since for those the mixer is a decode stream
    BASS_ChannelSetAttribute(this->g_bassOutputMixer, BASS_ATTRIB_BUFFER, 0.f);

    // Switch to "No sound" device for all future sound processing
    // Only g_bassOutputMixer will be output to the actual device!
    BASS_SetDevice(0);
    return true;
}

bool SoundEngine::initializeOutputDevice(const OUTPUT_DEVICE& device) {
    debugLog("SoundEngine: initializeOutputDevice( %s ) ...\n", device.name.toUtf8());

    this->shutdown();

    // We compensate for latency via BASS_ATTRIB_MIXER_LATENCY
    cv_universal_offset_hardcoded.setValue(0.f);

    if(device.driver == OutputDriver::NONE || (device.driver == OutputDriver::BASS && device.id == 0)) {
        this->ready_since = -1.0;
        this->currentOutputDevice = device;
        cv_snd_output_device.setValue(this->currentOutputDevice.name);
        debugLog("SoundEngine: Output Device = \"%s\"\n", this->currentOutputDevice.name.toUtf8());

        if(osu && osu->optionsMenu) {
            osu->optionsMenu->scheduleLayoutUpdate();
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
        if(!this->init_bass_mixer(device)) {
            return false;
        }
    }

#ifdef _WIN32
    if(device.driver == OutputDriver::BASS_ASIO) {
        if(!BASS_ASIO_Init(device.id, 0)) {
            ready_since = -1.0;
            debugLog("BASS_ASIO_Init() failed.\n");
            osu->getNotificationOverlay()->addToast(BassManager::getErrorUString());
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

        BASS_ASIO_INFO info{};
        BASS_ASIO_GetInfo(&info);
        auto bufsize = cv_asio_buffer_size.getInt();
        bufsize = ASIO_clamp(info, bufsize);

        if(osu && osu->optionsMenu) {
            auto slider = osu->optionsMenu->asioBufferSizeSlider;
            slider->setBounds(info.bufmin, info.bufmax);
            slider->setKeyDelta(info.bufgran == -1 ? info.bufmin : info.bufgran);
        }

        if(!BASS_ASIO_ChannelEnableBASS(false, 0, g_bassOutputMixer, true)) {
            ready_since = -1.0;
            debugLog("BASS_ASIO_ChannelEnableBASS() failed.\n");
            osu->getNotificationOverlay()->addToast(BassManager::getErrorUString());
            return false;
        }

        if(!BASS_ASIO_Start(bufsize, 0)) {
            ready_since = -1.0;
            debugLog("BASS_ASIO_Start() failed.\n");
            osu->getNotificationOverlay()->addToast(BassManager::getErrorUString());
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
            osu->getNotificationOverlay()->addToast(BassManager::getErrorUString());
            return false;
        }

        if(!BASS_WASAPI_Start()) {
            ready_since = -1.0;
            debugLog("BASS_WASAPI_Start() failed.\n");
            osu->getNotificationOverlay()->addToast(BassManager::getErrorUString());
            return false;
        }

        BASS_ChannelSetAttribute(g_bassOutputMixer, BASS_ATTRIB_MIXER_LATENCY,
                                 cv_win_snd_wasapi_buffer_size.getFloat());
    }
#endif

    this->ready_since = engine->getTime();
    this->currentOutputDevice = device;
    cv_snd_output_device.setValue(this->currentOutputDevice.name);
    debugLog("SoundEngine: Output Device = \"%s\"\n", this->currentOutputDevice.name.toUtf8());

    if(osu && osu->optionsMenu) {
        osu->optionsMenu->scheduleLayoutUpdate();
    }

    return true;
}

void SoundEngine::restart() { this->setOutputDevice(this->currentOutputDevice); }

void SoundEngine::shutdown() {
    VolNormalization::abort();

    if(this->currentOutputDevice.driver == OutputDriver::BASS) {
        BASS_SetDevice(this->currentOutputDevice.id);
        BASS_Free();
        BASS_SetDevice(0);
    }

#ifdef _WIN32
    if(this->currentOutputDevice.driver == OutputDriver::BASS_ASIO) {
        BASS_ASIO_Free();
    } else if(this->currentOutputDevice.driver == OutputDriver::BASS_WASAPI) {
        BASS_WASAPI_Free();
    }
#endif

    this->ready_since = -1.0;
    this->g_bassOutputMixer = 0;
    BASS_Free();  // free "No sound" device
}

void SoundEngine::update() {}

bool SoundEngine::play(Sound *snd, float pan, float pitch) {
    if(!this->isReady() || snd == NULL || !snd->isReady()) return false;

    if(!snd->isOverlayable() && snd->isPlaying()) {
        return false;
    }

    if(snd->isOverlayable() && cv_snd_restrict_play_frame.getBool()) {
        if(engine->getTime() <= snd->fLastPlayTime) {
            return false;
        }
    }

    HCHANNEL channel = snd->getChannel();
    if(channel == 0) {
        debugLog("SoundEngine::play() failed to get channel, errorcode %i\n", BASS_ErrorGetCode());
        return false;
    }

    pan = std::clamp<float>(pan, -1.0f, 1.0f);
    BASS_ChannelSetAttribute(channel, BASS_ATTRIB_VOL, snd->fVolume);
    BASS_ChannelSetAttribute(channel, BASS_ATTRIB_PAN, pan);
    BASS_ChannelSetAttribute(channel, BASS_ATTRIB_NORAMP, snd->isStream() ? 0 : 1);
    if(pitch != 0.0f) {
        f32 freq = cv_snd_freq.getFloat();
        BASS_ChannelGetAttribute(channel, BASS_ATTRIB_FREQ, &freq);
        BASS_ChannelSetAttribute(channel, BASS_ATTRIB_FREQ, pow(2.0f, pitch) * freq);
    }

    BASS_ChannelFlags(channel, snd->isLooped() ? BASS_SAMPLE_LOOP : 0, BASS_SAMPLE_LOOP);

    if(BASS_Mixer_ChannelGetMixer(channel) != 0) return false;

    auto flags = BASS_MIXER_DOWNMIX | BASS_MIXER_NORAMPIN | BASS_STREAM_AUTOFREE;
    if(!BASS_Mixer_StreamAddChannel(this->g_bassOutputMixer, channel, flags)) {
        debugLog("BASS_Mixer_StreamAddChannel() failed (%i)!\n", BASS_ErrorGetCode());
        return false;
    }

    // Make sure the mixer is playing! Duh.
    if(this->currentOutputDevice.driver == OutputDriver::BASS) {
        if(BASS_ChannelIsActive(this->g_bassOutputMixer) != BASS_ACTIVE_PLAYING) {
            if(!BASS_ChannelPlay(this->g_bassOutputMixer, true)) {
                debugLog("SoundEngine::play() couldn't BASS_ChannelPlay(), errorcode %i\n", BASS_ErrorGetCode());
                return false;
            }
        }
    }

    snd->bStarted = true;
    snd->fChannelCreationTime = engine->getTime();
    if(snd->bPaused) {
        snd->bPaused = false;
        snd->fLastPlayTime = snd->fChannelCreationTime - (((f64)snd->paused_position_ms) / 1000.0);
    } else {
        snd->fLastPlayTime = snd->fChannelCreationTime;
    }

    if(cv_debug.getBool()) {
        debugLog("Playing %s\n", snd->getFilePath().c_str());
    }

    return true;
}

void SoundEngine::pause(Sound *snd) {
    if(!this->isReady() || snd == NULL || !snd->isReady()) return;
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
    resourceManager->reloadResource(snd);

    snd->setPositionMS(pos);
    snd->setSpeed(speed);
    snd->setPan(pan);
    snd->setLoop(loop);
    snd->bPaused = true;
    snd->paused_position_ms = pos;
}

void SoundEngine::stop(Sound *snd) {
    if(!this->isReady() || snd == NULL || !snd->isReady()) return;

    // This will stop all samples, then re-init to be ready for a play()
    resourceManager->reloadResource(snd);
}

bool SoundEngine::isReady() {
    if(this->ready_since == -1.0) return false;
    return this->ready_since + (double)cv_snd_ready_delay.getFloat() < engine->getTime();
}

bool SoundEngine::hasExclusiveOutput() {
    return this->isASIO() || (this->isWASAPI() && cv_win_snd_wasapi_exclusive.getBool());
}

void SoundEngine::setOutputDevice(const OUTPUT_DEVICE& device) {
    bool was_playing = false;
    unsigned long prevMusicPositionMS = 0;
    if(osu->getSelectedBeatmap()->getMusic() != NULL) {
        was_playing = osu->getSelectedBeatmap()->getMusic()->isPlaying();
        prevMusicPositionMS = osu->getSelectedBeatmap()->getMusic()->getPositionMS();
    }

    // TODO: This is blocking main thread, can freeze for a long time on some sound cards
    auto previous = this->currentOutputDevice;
    if(!this->initializeOutputDevice(device)) {
        if((device.id == previous.id && device.driver == previous.driver) || !this->initializeOutputDevice(previous)) {
            // We failed to reinitialize the device, don't start an infinite loop, just give up
            this->currentOutputDevice = {
                .id = 0,
                .enabled = true,
                .isDefault = true,
                .name = "No sound",
                .driver = OutputDriver::NONE,
            };
        }
    }

    osu->optionsMenu->outputDeviceLabel->setText(this->getOutputDeviceName());
    osu->getSkin()->reloadSounds();
    osu->optionsMenu->onOutputDeviceResetUpdate();

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
    VolNormalization::start_calc(db->loudness_to_calc);

    if(was_playing) {
        osu->music_unpause_scheduled = true;
    }
}

void SoundEngine::setVolume(float volume) {
    if(!this->isReady()) return;

    this->fVolume = std::clamp<float>(volume, 0.0f, 1.0f);
    if(this->currentOutputDevice.driver == OutputDriver::BASS_ASIO) {
#ifdef _WIN32
        BASS_ASIO_ChannelSetVolume(false, 0, this->fVolume);
        BASS_ASIO_ChannelSetVolume(false, 1, this->fVolume);
#endif
    } else if(this->currentOutputDevice.driver == OutputDriver::BASS_WASAPI) {
#ifdef _WIN32
        if(hasExclusiveOutput()) {
            // Device volume doesn't seem to work, so we'll use DSP instead
            BASS_ChannelSetAttribute(g_bassOutputMixer, BASS_ATTRIB_VOLDSP, this->fVolume);
        } else {
            BASS_WASAPI_SetVolume(BASS_WASAPI_CURVE_WINDOWS | BASS_WASAPI_VOL_SESSION, this->fVolume);
        }
#endif
    } else {
        // 0 (silent) - 10000 (full).
        BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE, (DWORD)(this->fVolume * 10000));
        BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, (DWORD)(this->fVolume * 10000));
        BASS_SetConfig(BASS_CONFIG_GVOL_MUSIC, (DWORD)(this->fVolume * 10000));
    }
}

void SoundEngine::onFreqChanged(const UString &oldValue, const UString &newValue) {
    (void)oldValue;
    (void)newValue;
    if(!this->isReady()) return;
    this->restart();
}

std::vector<OUTPUT_DEVICE> SoundEngine::getOutputDevices() {
    std::vector<OUTPUT_DEVICE> outputDevices;

    for(size_t i = 0; i < this->outputDevices.size(); i++) {
        if(this->outputDevices[i].enabled) {
            outputDevices.push_back(this->outputDevices[i]);
        }
    }

    return outputDevices;
}
