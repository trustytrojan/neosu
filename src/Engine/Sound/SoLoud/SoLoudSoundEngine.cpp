//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		handles sounds using SoLoud library
//
// $NoKeywords: $snd $soloud
//================================================================================//

#include "SoLoudSoundEngine.h"

#ifdef MCENGINE_FEATURE_SOLOUD

#include "SString.h"
#include "SoLoudSound.h"

#include "ConVar.h"
#include "Engine.h"

#include "Environment.h"

#include <utility>
#include <soloud.h>

#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_audio.h>

std::unique_ptr<SoLoud::Soloud> soloud = nullptr;

SoLoudSoundEngine::SoLoudSoundEngine() : SoundEngine() {
    if(!soloud) {
        soloud = std::make_unique<SoLoud::Soloud>();
    }

    cv::snd_freq.setValue(SoLoud::Soloud::AUTO);  // let it be auto-negotiated (the snd_freq callback will adjust if
                                                  // needed, if this is manually set in a config)
    cv::snd_freq.setDefaultFloat(SoLoud::Soloud::AUTO);

    this->iMaxActiveVoices =
        std::clamp<int>(cv::snd_sanity_simultaneous_limit.getInt(), 64,
                        255);  // TODO: lower this minimum (it will crash if more than this many sounds play at once...)

    OUTPUT_DEVICE defaultOutputDevice{};
    cv::snd_output_device.setValue(defaultOutputDevice.name);
    this->outputDevices.push_back(defaultOutputDevice);
    this->currentOutputDevice = defaultOutputDevice;

    this->mSoloudDevices = {};

    this->initializeOutputDevice(defaultOutputDevice);
}

void SoLoudSoundEngine::restart() { this->setOutputDeviceInt(this->getWantedDevice(), true); }

bool SoLoudSoundEngine::play(Sound *snd, float pan, float pitch) {
    if(!this->isReady() || snd == NULL || !snd->isReady()) return false;

    // MC_MESSAGE(
    //     "FIXME: audio pitch may be inaccurate! need to convert caller's pitch properly like old McOsu for SoLoud")

    // @spec: adding 1 here because kiwec changed the calling code in some way that i dont understand yet
    pitch += 1.0f;

    pan = std::clamp<float>(pan, -1.0f, 1.0f);
    pitch = std::clamp<float>(pitch, 0.0f, 2.0f);

    auto *soloudSound = snd->as<SoLoudSound>();
    if(!soloudSound) return false;

    auto handle = soloudSound->getHandle();
    // verify that the sound object's handle isn't stale
    if(handle != 0 && !soloud->isValidVoiceHandle(handle)) {
        handle = 0;
        soloudSound->handle = 0;
    }
    if(handle != 0 && soloud->getPause(handle)) {
        // just unpause if paused
        soloudSound->setPitch(pitch);
        soloudSound->setPan(pan);
        soloud->setPause(handle, false);
        return true;
    }

    return playSound(soloudSound, pan, pitch);
}

bool SoLoudSoundEngine::playSound(SoLoudSound *soloudSound, float pan, float pitch) {
    if(!soloudSound) return false;

    // check if we should allow playing this frame (for overlayable sounds)
    const bool allowPlayFrame = !soloudSound->isOverlayable() || !cv::snd_restrict_play_frame.getBool() ||
                                engine->getTime() > soloudSound->getLastPlayTime();
    if(!allowPlayFrame) return false;

    auto restorePos = 0.0;  // position in the track to potentially restore to

    // if the sound is already playing and not overlayable, stop it
    if(soloudSound->handle != 0 && !soloudSound->isOverlayable()) {
        restorePos = soloudSound->getStreamPositionInSeconds();
        soloud->stop(soloudSound->handle);
        soloudSound->handle = 0;
    }

    if(cv::debug_snd.getBool()) {
        debugLogF("SoLoudSoundEngine: Playing {:s} (stream={:d}) with speed={:f}, pitch={:f}, volume={:f}\n",
                  soloudSound->sFilePath, soloudSound->bStream ? 1 : 0, soloudSound->fSpeed, pitch,
                  soloudSound->fVolume);
    }

    // play the sound with appropriate method
    unsigned int handle = 0;

    if(soloudSound->bStream) {
        // reset these, because they're "sticky" properties
        soloudSound->setPitch(pitch);
        soloudSound->setPan(pan);

        // streaming audio (music) - play SLFXStream directly (it handles SoundTouch internally)
        // start it at 0 volume and fade it in when we play it (to avoid clicks/pops)
        handle = soloud->play(*soloudSound->audioSource, 0, pan, true /* paused */);
        if(handle)
            soloud->setProtectVoice(
                handle,
                true);  // protect the music channel (don't let it get interrupted when many sounds play back at once)
                        // NOTE: this doesn't seem to work properly, not sure why... need to setMaxActiveVoiceCount
                        // higher than the default 16 as a workaround, otherwise rapidly overlapping samples like from
                        // buzzsliders can cause glitches in music playback
                        // TODO: a better workaround would be to manually prevent samples from playing if
                        // it would lead to getMaxActiveVoiceCount() <= getActiveVoiceCount()

        if(cv::debug_snd.getBool() && handle)
            debugLogF("SoLoudSoundEngine: Playing streaming audio through SLFXStream with speed={:f}, pitch={:f}\n",
                      soloudSound->fSpeed, soloudSound->fPitch);
    } else {
        // non-streaming audio (sound effects) - use direct playback with SoLoud's native speed/pitch control
        handle = this->playDirectSound(soloudSound, pan, pitch, soloudSound->fVolume);
    }

    // finalize playback
    if(handle != 0) {
        // store the handle and mark playback time
        soloudSound->handle = handle;

        if(restorePos != 0.0) soloud->seek(handle, restorePos);  // restore the position to where we were pre-pause

        soloudSound->setLastPlayTime(engine->getTime());

        if(soloudSound->bStream)  // unpause and fade it in if it's a stream (since we started it paused with 0 volume)
        {
            soloud->setPause(handle, false);
            this->setVolumeGradual(handle, soloudSound->fVolume);
        }

        return true;
    }

    if(cv::debug_snd.getBool()) debugLogF("SoLoudSoundEngine: Failed to play sound {:s}\n", soloudSound->sFilePath);

    return false;
}

unsigned int SoLoudSoundEngine::playDirectSound(SoLoudSound *soloudSound, float pan, float pitch, float volume) {
    if(!soloudSound || !soloudSound->audioSource) return 0;

    // calculate final pitch by combining all pitch modifiers
    float finalPitch = pitch;

    // combine with sound's pitch setting
    if(soloudSound->fPitch != 1.0f) finalPitch *= soloudSound->fPitch;

    // if speed compensation is disabled, apply speed as pitch
    if(cv::nightcore_enjoyer.getBool() && soloudSound->fSpeed != 1.0f) finalPitch *= soloudSound->fSpeed;

    // play directly
    unsigned int handle = soloud->play(*soloudSound->audioSource, volume, pan, false /* not paused */);

    if(handle != 0) {
        // set relative play speed (affects both pitch and speed)
        if(finalPitch != 1.0f) soloud->setRelativePlaySpeed(handle, finalPitch);

        if(cv::debug_snd.getBool())
            debugLogF(
                "SoLoudSoundEngine: Playing non-streaming audio with finalPitch={:f} (pitch={:f} * soundPitch={:f} * "
                "speedAsPitch={:f})\n",
                finalPitch, pitch, soloudSound->fPitch,
                (cv::nightcore_enjoyer.getBool() && soloudSound->fSpeed != 1.0f) ? soloudSound->fSpeed : 1.0f);
    }

    return handle;
}

void SoLoudSoundEngine::pause(Sound *snd) {
    if(!this->isReady() || snd == NULL || !snd->isReady()) return;

    auto *soloudSound = snd->as<SoLoudSound>();
    if(!soloudSound || soloudSound->handle == 0) return;

    soloud->setPause(soloudSound->handle, true);
    soloudSound->setLastPlayTime(0.0);
}

void SoLoudSoundEngine::stop(Sound *snd) {
    if(!this->isReady() || snd == NULL || !snd->isReady()) return;

    auto *soloudSound = snd->as<SoLoudSound>();
    if(!soloudSound || soloudSound->handle == 0) return;

    soloudSound->setPosition(0.0);
    soloudSound->setLastPlayTime(0.0);
    soloudSound->setFrequency(0.0);
    soloud->stop(soloudSound->handle);
    soloudSound->handle = 0;
}

void SoLoudSoundEngine::setOutputDeviceByName(const UString &desiredDeviceName) {
    for(const auto &device : this->outputDevices) {
        if(device.name == desiredDeviceName) {
            this->setOutputDeviceInt(device);
            return;
        }
    }

    debugLogF("couldn't find output device \"{:s}\"!\n", desiredDeviceName.toUtf8());
    this->initializeOutputDevice(this->getDefaultDevice());  // initialize default
}

void SoLoudSoundEngine::setOutputDevice(const SoundEngine::OUTPUT_DEVICE &device) {
    this->setOutputDeviceInt(device, false);
}

bool SoLoudSoundEngine::setOutputDeviceInt(const SoundEngine::OUTPUT_DEVICE &desiredDevice, bool force) {
    if(force || !this->bReady) {
        // TODO: This is blocking main thread, can freeze for a long time on some sound cards
        auto previous = this->currentOutputDevice;
        if(!this->initializeOutputDevice(desiredDevice)) {
            if((desiredDevice.id == previous.id && desiredDevice.driver == previous.driver) ||
               !this->initializeOutputDevice(previous)) {
                // We failed to reinitialize the device, don't start an infinite loop, just give up
                this->currentOutputDevice = {};
                return false;
            }
        }
        return true;
    }
    for(const auto &device : this->outputDevices) {
        if(device.name == desiredDevice.name) {
            if(device.id != this->currentOutputDevice.id) {
                auto previous = this->currentOutputDevice;
                if(!this->initializeOutputDevice(desiredDevice)) this->initializeOutputDevice(previous);
            } else {
                debugLogF("\"{:s}\" already is the current device.\n", desiredDevice.name);
                return false;
            }

            return true;
        }
    }

    debugLogF("couldn't find output device \"{:s}\"!\n", desiredDevice.name);
    return false;
}

// delay setting these until after everything is fully init, so we don't restart multiple times while reading config
void SoLoudSoundEngine::allowInternalCallbacks() {
    // convar callbacks
    cv::snd_freq.setCallback(SA::MakeDelegate<&SoLoudSoundEngine::restart>(this));
    cv::snd_restart.setCallback(SA::MakeDelegate<&SoLoudSoundEngine::restart>(this));
    cv::snd_soloud_backend.setCallback(SA::MakeDelegate<&SoLoudSoundEngine::restart>(this));
    cv::snd_sanity_simultaneous_limit.setCallback(SA::MakeDelegate<&SoLoudSoundEngine::onMaxActiveChange>(this));
    cv::snd_output_device.setCallback(SA::MakeDelegate<&SoLoudSoundEngine::setOutputDeviceByName>(this));

    bool doRestart = (cv::snd_freq.getDefaultFloat() != cv::snd_freq.getFloat()) ||
                     (cv::snd_restart.getDefaultFloat() != cv::snd_restart.getFloat()) ||
                     (cv::snd_soloud_backend.getDefaultString() != cv::snd_soloud_backend.getString());

    if(doRestart) {
        this->restart();
    }

    bool doMaxActive =
        cv::snd_sanity_simultaneous_limit.getDefaultFloat() != cv::snd_sanity_simultaneous_limit.getFloat();
    if(doMaxActive) {
        this->onMaxActiveChange(cv::snd_sanity_simultaneous_limit.getFloat());
    }

    // if we restarted already, then we already set the output device to the desired one
    bool doChangeOutput = !doRestart && (cv::snd_output_device.getDefaultString() != cv::snd_output_device.getString());
    if(doChangeOutput) {
        this->setOutputDeviceByName(UString{cv::snd_output_device.getString()});
    }
}

SoLoudSoundEngine::~SoLoudSoundEngine() {
    if(soloud && this->isReady()) {
        soloud->deinit();
    }
    soloud.reset();
}

void SoLoudSoundEngine::setVolume(float volume) {
    if(!this->isReady()) return;

    this->fMasterVolume = std::clamp<float>(volume, 0.0f, 1.0f);

    // if (cv::debug_snd.getBool())
    // 	debugLogF("setting global volume to {:f}\n", fVolume);
    soloud->setGlobalVolume(this->fMasterVolume);
}

void SoLoudSoundEngine::setVolumeGradual(unsigned int handle, float targetVol, float fadeTimeMs) {
    if(!this->isReady() || handle == 0 || !soloud->isValidVoiceHandle(handle)) return;

    soloud->setVolume(handle, 0.0f);

    if(cv::debug_snd.getBool()) debugLogF("fading in to {:.2f}\n", targetVol);

    soloud->fadeVolume(handle, targetVol, fadeTimeMs / 1000.0f);
}

void SoLoudSoundEngine::updateOutputDevices(bool printInfo) {
    if(!this->isReady())  // soloud needs to be initialized first
        return;

    using namespace SoLoud;

    // reset these, because if the backend changed, it might enumerate devices differently
    this->mSoloudDevices.clear();
    this->outputDevices.erase(this->outputDevices.begin() + 1,
                              this->outputDevices.end());  // keep default device (elem 0)

    debugLogF("SoundEngine: Using SoLoud backend: {:s}\n", cv::snd_soloud_backend.getString());
    DeviceInfo currentDevice{};

    result res = soloud->getCurrentDevice(&currentDevice);
    if(res == SO_NO_ERROR) {
        this->mSoloudDevices[-1] = {currentDevice};
        debugLogF("SoundEngine: Current device: {:s} (Default: {:s})\n", currentDevice.name,
                  currentDevice.isDefault ? "Yes" : "No");
    } else {
        this->mSoloudDevices[-1] = {
            .name = "Unavailable", .identifier = "", .isDefault = true, .nativeDeviceInfo = nullptr};
    }

    DeviceInfo *devices{};
    unsigned int deviceCount = 0;
    res = soloud->enumerateDevices(&devices, &deviceCount);

    if(res == SO_NO_ERROR) {
        for(int d = 0; std::cmp_less(d, deviceCount); d++) {
            if(printInfo)
                debugLogF("SoundEngine: Device {}: {:s} (Default: {:s})\n", d, devices[d].name,
                          devices[d].isDefault ? "Yes" : "No");

            UString originalDeviceName{&devices[d].name[0]};
            OUTPUT_DEVICE soundDevice;
            soundDevice.id = d;
            soundDevice.name = originalDeviceName;
            soundDevice.enabled = true;
            soundDevice.isDefault = devices[d].isDefault;
            soundDevice.driver = OutputDriver::SOLOUD;

            // avoid duplicate names
            int duplicateNameCounter = 2;
            while(true) {
                bool foundDuplicateName = false;
                for(const auto &existingDevice : this->outputDevices) {
                    if(existingDevice.name == soundDevice.name) {
                        foundDuplicateName = true;
                        soundDevice.name = originalDeviceName;
                        soundDevice.name.append(UString::format(" (%i)", duplicateNameCounter));
                        duplicateNameCounter++;
                        break;
                    }
                }

                if(!foundDuplicateName) break;
            }
            if(cv::debug_snd.getBool()) {
                debugLogF("added device id {} name {} iteration (d) {}\n", soundDevice.id, soundDevice.name, d);
            }
            this->mSoloudDevices[d] = {devices[d]};
            this->outputDevices.push_back(soundDevice);
        }
    }
}

bool SoLoudSoundEngine::initializeOutputDevice(const OUTPUT_DEVICE &device) {
    // run callbacks pt. 1
    if(this->restartCBs[0] != nullptr) this->restartCBs[0]();
    debugLogF("id {} name {}\n", device.id, device.name);

    // cleanup potential previous device
    if(this->isReady()) {
        soloud->deinit();
        this->bReady = false;
    }

    // basic flags
    // roundoff clipping alters/"damages" the waveform, but it sounds weird without it
    auto flags = SoLoud::Soloud::CLIP_ROUNDOFF; /* | SoLoud::Soloud::NO_FPU_REGISTER_CHANGE; */

    auto backend = SoLoud::Soloud::MINIAUDIO;
    const auto &userBackend = cv::snd_soloud_backend.getString();
    if((userBackend != cv::snd_soloud_backend.getDefaultString())) {
        if(SString::contains_ncase(userBackend, "sdl"))
            backend = SoLoud::Soloud::SDL3;
        else
            backend = SoLoud::Soloud::MINIAUDIO;
    }

    unsigned int sampleRate = (cv::snd_freq.getVal<unsigned int>() == static_cast<unsigned int>(cv::snd_freq.getDefaultFloat())
                                   ? (unsigned int)SoLoud::Soloud::AUTO
                                   : cv::snd_freq.getVal<unsigned int>());
    if(sampleRate < 22500 || sampleRate > 192000) sampleRate = SoLoud::Soloud::AUTO;

    unsigned int bufferSize =
        (cv::snd_soloud_buffer.getVal<unsigned int>() == static_cast<unsigned int>(cv::snd_soloud_buffer.getDefaultFloat())
             ? (unsigned int)SoLoud::Soloud::AUTO
             : cv::snd_soloud_buffer.getVal<unsigned int>());
    if(bufferSize > 2048) bufferSize = SoLoud::Soloud::AUTO;

    // use stereo output
    constexpr unsigned int channels = 2;

    // setup some SDL hints in case the SDL backend is used
    SDL_SetHintWithPriority(SDL_HINT_AUDIO_DEVICE_STREAM_NAME, PACKAGE_NAME, SDL_HINT_OVERRIDE);
    SDL_SetHintWithPriority(SDL_HINT_AUDIO_DEVICE_STREAM_ROLE, "game", SDL_HINT_OVERRIDE);

    // initialize a new soloud instance
    SoLoud::result result = soloud->init(flags, backend, sampleRate, bufferSize, channels);

    if(result != SoLoud::SO_NO_ERROR) {
        this->bReady = false;
        engine->showMessageError("Sound Error", UString::format("SoLoud::Soloud::init() failed (%i)!", result));
        return false;
    }

    this->bReady = true;

    // populate devices array and set the desired output
    this->updateOutputDevices(true);
    if(device.id != this->currentOutputDevice.id && this->outputDevices.size() > 1) {
        if(soloud->setDevice(&this->mSoloudDevices[device.id].identifier[0]) == SoLoud::SO_NO_ERROR) {
            this->currentOutputDevice = device;
            this->currentOutputDevice.name = {
                &this->mSoloudDevices[device.id].name[0]};  // TODO: is this redundant? idk, need to go over the device
                                                            // selection rebase from mcosu-ng
        } else {
            this->currentOutputDevice = this->outputDevices[0];
        }

        cv::snd_output_device.setValue(this->currentOutputDevice.name, false);
    }

    // it's 0.95 by default, for some reason
    soloud->setPostClipScaler(1.0f);

    cv::snd_freq.setValue(soloud->getBackendSamplerate(),
                          false);  // set the cvar to match the actual output sample rate (without running callbacks)
    cv::snd_soloud_backend.setValue(soloud->getBackendString(), false);  // ditto
    if(cv::snd_soloud_buffer.getFloat() != cv::snd_soloud_buffer.getDefaultFloat())
        cv::snd_soloud_buffer.setValue(soloud->getBackendBufferSize(),
                                       false);  // ditto (but only if explicitly non-default was requested already)

    this->onMaxActiveChange(cv::snd_sanity_simultaneous_limit.getFloat());

    debugLogF(
        "SoundEngine: Initialized SoLoud with output device = \"{:s}\" flags: 0x{:x}, backend: {:s}, sampleRate: {}, "
        "bufferSize: {}, channels: {}, "
        "maxActiveVoiceCount: {}\n",
        this->currentOutputDevice.name.toUtf8(), static_cast<unsigned int>(flags), soloud->getBackendString(),
        soloud->getBackendSamplerate(), soloud->getBackendBufferSize(), soloud->getBackendChannels(),
        this->iMaxActiveVoices);

    // init global volume
    this->setVolume(this->fMasterVolume);

    // run callbacks pt. 2
    if(this->restartCBs[1] != nullptr) {
        this->restartCBs[1]();
    }
    return true;
}

void SoLoudSoundEngine::onMaxActiveChange(float newMax) {
    const auto desired = std::clamp<unsigned int>(static_cast<unsigned int>(newMax), 64, 255);
    if(std::cmp_not_equal(soloud->getMaxActiveVoiceCount(), desired)) {
        SoLoud::result res = soloud->setMaxActiveVoiceCount(desired);
        if(res != SoLoud::SO_NO_ERROR) debugLogF("SoundEngine WARNING: failed to setMaxActiveVoiceCount ({})\n", res);
    }
    this->iMaxActiveVoices = static_cast<int>(soloud->getMaxActiveVoiceCount());
    cv::snd_sanity_simultaneous_limit.setValue(this->iMaxActiveVoices, false);  // no infinite callback loop
}

#endif  // MCENGINE_FEATURE_SOLOUD
