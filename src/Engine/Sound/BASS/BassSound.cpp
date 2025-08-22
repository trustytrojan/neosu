// Copyright (c) 2014, PG, All rights reserved.
#include "BassSound.h"

#ifdef MCENGINE_FEATURE_BASS

#include <utility>

#include "ConVar.h"
#include "Engine.h"
#include "File.h"
#include "Osu.h"
#include "ResourceManager.h"

std::vector<HCHANNEL> BassSound::getActiveChannels() {
    std::vector<HCHANNEL> channels;

    if(this->bStream) {
        if(BASS_Mixer_ChannelGetMixer(this->stream) != 0) {
            channels.push_back(this->stream);
        }
    } else {
        for(auto chan : this->mixer_channels) {
            if(BASS_Mixer_ChannelGetMixer(chan) != 0) {
                channels.push_back(chan);
            }
        }

        // Only keep channels that are still playing
        this->mixer_channels = channels;
    }

    return channels;
}

HCHANNEL BassSound::getChannel() {
    if(this->bStream) {
        return this->stream;
    } else {
        // If we want to be able to control samples after playing them, we
        // have to store them here, since bassmix only accepts DECODE streams.
        auto chan = BASS_SampleGetChannel(this->sample, BASS_SAMCHAN_STREAM | BASS_STREAM_DECODE);
        this->mixer_channels.push_back(chan);
        return chan;
    }
}

void BassSound::init() {
    if(this->bIgnored || this->sFilePath.length() < 2 || !(this->bAsyncReady.load())) return;

    this->bReady = this->bAsyncReady.load();
}

void BassSound::initAsync() {
    Sound::initAsync();
    if(this->bIgnored) return;

    UString file_path{this->sFilePath};

    if(this->bStream) {
        u32 flags = BASS_STREAM_DECODE | BASS_SAMPLE_FLOAT | BASS_STREAM_PRESCAN;
        if(cv::snd_async_buffer.getInt() > 0) flags |= BASS_ASYNCFILE;
        if constexpr(Env::cfg(OS::WINDOWS)) flags |= BASS_UNICODE;

        if(this->bInterrupted.load()) return;
        this->stream = BASS_StreamCreateFile(BASS_FILE_NAME, file_path.plat_str(), 0, 0, flags);
        if(!this->stream) {
            debugLog("BASS_StreamCreateFile() error on file {}: {}\n", this->sFilePath.c_str(),
                     BassManager::getErrorUString());
            return;
        }

        if(this->bInterrupted.load()) return;
        this->stream = BASS_FX_TempoCreate(this->stream, BASS_FX_FREESOURCE | BASS_STREAM_DECODE);
        if(!this->stream) {
            debugLog("BASS_FX_TempoCreate() error on file {}: {}\n", this->sFilePath.c_str(),
                     BassManager::getErrorUString());
            return;
        }

        BASS_ChannelSetAttribute(this->stream, BASS_ATTRIB_TEMPO_OPTION_USE_QUICKALGO, false);
        BASS_ChannelSetAttribute(this->stream, BASS_ATTRIB_TEMPO_OPTION_OVERLAP_MS, 4.0f);
        BASS_ChannelSetAttribute(this->stream, BASS_ATTRIB_TEMPO_OPTION_SEQUENCE_MS, 30.0f);
        BASS_ChannelSetAttribute(this->stream, BASS_ATTRIB_TEMPO_OPTION_OLDPOS, 1);  // use old position calculation

        // Only compute the length once
        if(this->bInterrupted.load()) return;
        i64 length = BASS_ChannelGetLength(this->stream, BASS_POS_BYTE);
        f64 lengthInSeconds = BASS_ChannelBytes2Seconds(this->stream, length);
        f64 lengthInMilliSeconds = lengthInSeconds * 1000.0;
        this->length = (u32)lengthInMilliSeconds;
    } else {
        u32 flags = BASS_SAMPLE_FLOAT;
        if constexpr(Env::cfg(OS::WINDOWS)) flags |= BASS_UNICODE;

        if(this->bInterrupted.load()) return;
        this->sample = BASS_SampleLoad(false, file_path.plat_str(), 0, 0, 1, flags);
        if(!this->sample) {
            auto code = BASS_ErrorGetCode();
            if(code == BASS_ERROR_EMPTY) {
                debugLog("BassSound: Ignoring empty file {}\n", this->sFilePath.c_str());
                return;
            } else {
                debugLog("BASS_SampleLoad() error on file {}: {}\n", this->sFilePath.c_str(),
                         BassManager::getErrorUString(code));
                return;
            }
        }

        // Only compute the length once
        if(this->bInterrupted.load()) return;
        i64 length = BASS_ChannelGetLength(this->sample, BASS_POS_BYTE);
        f64 lengthInSeconds = BASS_ChannelBytes2Seconds(this->sample, length);
        f64 lengthInMilliSeconds = lengthInSeconds * 1000.0;
        this->length = (u32)lengthInMilliSeconds;
    }

    this->fSpeed = 1.0f;
    this->bAsyncReady = true;
}

void BassSound::destroy() {
    if(!this->bAsyncReady) {
        this->interruptLoad();
    }

    if(this->sample != 0) {
        BASS_SampleStop(this->sample);
        BASS_SampleFree(this->sample);
        this->sample = 0;
    }

    if(this->stream != 0) {
        BASS_Mixer_ChannelRemove(this->stream);
        BASS_ChannelStop(this->stream);
        BASS_StreamFree(this->stream);
        this->stream = 0;
    }

    for(auto chan : this->mixer_channels) {
        BASS_Mixer_ChannelRemove(chan);
        BASS_ChannelStop(chan);
        BASS_ChannelFree(chan);
    }
    this->mixer_channels.clear();

    this->bStarted = false;
    this->bReady = false;
    this->bAsyncReady = false;
    this->fLastPlayTime = 0.0;
    this->fChannelCreationTime = 0.0;
    this->bPaused = false;
    this->paused_position_ms = 0;
    this->bIgnored = false;
}

u32 BassSound::setPosition(f64 percent) {
    u32 ms = std::clamp<f64>(percent, 0.0, 1.0) * this->length;
    this->setPositionMS(ms);
    return ms;
}

void BassSound::setPositionMS(unsigned long ms) {
    if(!this->bReady || ms > this->getLengthMS()) return;
    if(!this->bStream) {
        engine->showMessageError("Programmer Error", "Called setPositionMS on a sample!");
        return;
    }

    i64 target_pos = BASS_ChannelSeconds2Bytes(this->stream, ms / 1000.0);
    if(target_pos < 0) {
        debugLog("BASS_ChannelSeconds2Bytes( stream , {} ) error: {}\n", ms / 1000.0, BassManager::getErrorUString());
        return;
    }

    // Naively setting position breaks with the current BASS version (& addons).
    //
    // BASS_STREAM_PRESCAN no longer seems to work, so our only recourse is to use the BASS_POS_DECODETO
    // flag which renders the whole audio stream up until the requested seek point.
    //
    // The downside of BASS_POS_DECODETO is that it can only seek forward... furthermore, we can't
    // just seek to 0 before seeking forward again, since BASS_Mixer_ChannelGetPosition breaks
    // in that case. So, our only recourse is to just reload the whole fucking stream just for seeking.

    bool was_playing = this->isPlaying();
    auto pos = this->getPositionMS();
    if(pos <= ms) {
        // Lucky path, we can just seek forward and be done
        if(this->isPlaying()) {
            if(!BASS_Mixer_ChannelSetPosition(this->stream, target_pos,
                                              BASS_POS_BYTE | BASS_POS_DECODETO | BASS_POS_MIXER_RESET)) {
                if(cv::debug_snd.getBool()) {
                    debugLog("setPositionMS( {} ) BASS_ChannelSetPosition() error on file {}: {}\n", ms,
                             this->sFilePath.c_str(), BassManager::getErrorUString(BASS_ErrorGetCode()));
                }
            }
            this->fLastPlayTime = this->fChannelCreationTime - ((f64)ms / 1000.0);
        } else {
            if(!BASS_ChannelSetPosition(this->stream, target_pos, BASS_POS_BYTE | BASS_POS_DECODETO | BASS_POS_FLUSH)) {
                if(cv::debug_snd.getBool()) {
                    debugLog("setPositionMS( {} ) BASS_ChannelSetPosition() error on file {}: {}\n", ms,
                             this->sFilePath.c_str(), BassManager::getErrorUString(BASS_ErrorGetCode()));
                }
            }
        }
    } else {
        // Unlucky path, we have to reload the stream
        auto pan = this->getPan();
        auto loop = this->isLooped();
        auto speed = this->getSpeed();

        resourceManager->reloadResource(this);

        this->setSpeed(speed);
        this->setPan(pan);
        this->setLoop(loop);
        this->bPaused = true;
        this->paused_position_ms = ms;

        if(!BASS_ChannelSetPosition(this->stream, target_pos, BASS_POS_BYTE | BASS_POS_DECODETO | BASS_POS_FLUSH)) {
            if(cv::debug_snd.getBool()) {
                debugLog("setPositionMS( {} ) BASS_ChannelSetPosition() error on file {}: {}\n", ms,
                         this->sFilePath.c_str(), BassManager::getErrorUString());
            }
        }

        if(was_playing) {
            osu->music_unpause_scheduled = true;
        }
    }

    // reset interpolation state after seeking
    this->interpolator.reset(static_cast<f64>(ms), Timing::getTimeReal(), this->getSpeed());
}

// Inaccurate but fast seeking, to use at song select
void BassSound::setPositionMS_fast(u32 ms) {
    if(!this->bReady || ms > this->getLengthMS()) return;
    if(!this->bStream) {
        engine->showMessageError("Programmer Error", "Called setPositionMS_fast on a sample!");
        return;
    }

    i64 target_pos = BASS_ChannelSeconds2Bytes(this->stream, ms / 1000.0);
    if(target_pos < 0) {
        debugLog("BASS_ChannelSeconds2Bytes( stream , {} ) error on file {}: {}\n", ms / 1000.0,
                 this->sFilePath.c_str(), BassManager::getErrorUString());
        return;
    }

    if(this->isPlaying()) {
        if(!BASS_Mixer_ChannelSetPosition(this->stream, target_pos, BASS_POS_BYTE | BASS_POS_MIXER_RESET)) {
            if(cv::debug_snd.getBool()) {
                debugLog("BASS_Mixer_ChannelSetPosition( stream , {} ) error on file {}: {}\n", ms,
                         this->sFilePath.c_str(), BassManager::getErrorUString());
            }
        }
        this->fLastPlayTime = this->fChannelCreationTime - ((f64)ms / 1000.0);
    } else {
        if(!BASS_ChannelSetPosition(this->stream, target_pos, BASS_POS_BYTE | BASS_POS_FLUSH)) {
            if(cv::debug_snd.getBool()) {
                debugLog("BASS_ChannelSetPosition( stream , {} ) error on file {}: {}\n", ms, this->sFilePath.c_str(),
                         BassManager::getErrorUString());
            }
        }
    }

    // reset interpolation state after seeking
    this->interpolator.reset(static_cast<f64>(ms), Timing::getTimeReal(), this->getSpeed());
}

void BassSound::setVolume(float volume) {
    if(!this->bReady) return;

    this->fVolume = std::clamp<float>(volume, 0.0f, 2.0f);

    // TODO @kiwec: add a setting to NOT change the volume of existing sounds
    //              because hitsounds can have different volume...
    for(auto channel : this->getActiveChannels()) {
        BASS_ChannelSetAttribute(channel, BASS_ATTRIB_VOL, this->fVolume);
    }
}

void BassSound::setSpeed(float speed) {
    if(!this->bReady) return;
    if(!this->bStream) {
        engine->showMessageError("Programmer Error", "Called setSpeed on a sample!");
        return;
    }

    speed = std::clamp<float>(speed, 0.05f, 50.0f);

    float freq = cv::snd_freq.getFloat();
    BASS_ChannelGetAttribute(this->stream, BASS_ATTRIB_FREQ, &freq);

    BASS_ChannelSetAttribute(this->stream, BASS_ATTRIB_TEMPO, 1.0f);
    BASS_ChannelSetAttribute(this->stream, BASS_ATTRIB_TEMPO_FREQ, freq);

    if(cv::nightcore_enjoyer.getBool()) {
        BASS_ChannelSetAttribute(this->stream, BASS_ATTRIB_TEMPO_FREQ, speed * freq);
    } else {
        BASS_ChannelSetAttribute(this->stream, BASS_ATTRIB_TEMPO, (speed - 1.0f) * 100.0f);
    }

    this->fSpeed = speed;
}

void BassSound::setFrequency(float frequency) {
    if(!this->bReady) return;

    frequency = (frequency > 99.0f ? std::clamp<float>(frequency, 100.0f, 100000.0f) : 0.0f);

    for(auto channel : this->getActiveChannels()) {
        BASS_ChannelSetAttribute(channel, BASS_ATTRIB_FREQ, frequency);
    }
}

void BassSound::setPan(float pan) {
    if(!this->bReady) return;

    this->fPan = std::clamp<float>(pan, -1.0f, 1.0f);

    for(auto channel : this->getActiveChannels()) {
        BASS_ChannelSetAttribute(channel, BASS_ATTRIB_PAN, this->fPan);
    }
}

void BassSound::setLoop(bool loop) {
    if(!this->bReady) return;
    if(!this->bStream) {
        engine->showMessageError("Programmer Error", "Called setLoop on a sample!");
        return;
    }

    this->bIsLooped = loop;
    BASS_ChannelFlags(this->stream, this->bIsLooped ? BASS_SAMPLE_LOOP : 0, BASS_SAMPLE_LOOP);
}

float BassSound::getPosition() {
    if(!this->bReady) return 0.f;
    if(!this->bStream) {
        engine->showMessageError("Programmer Error", "Called getPosition on a sample!");
        return 0.f;
    }
    if(this->bPaused) {
        return (f64)this->paused_position_ms / (f64)this->length;
    }

    i64 lengthBytes = BASS_ChannelGetLength(this->stream, BASS_POS_BYTE);
    if(lengthBytes < 0) {
        // The stream ended and got freed by BASS_STREAM_AUTOFREE -> invalid handle!
        return 1.f;
    }

    i64 positionBytes = 0;
    if(this->isPlaying()) {
        positionBytes = BASS_Mixer_ChannelGetPosition(this->stream, BASS_POS_BYTE);
    } else {
        positionBytes = BASS_ChannelGetPosition(this->stream, BASS_POS_BYTE);
    }

    const auto position = (float)((double)(positionBytes) / (double)(lengthBytes));
    return position;
}

u32 BassSound::getPositionMS() {
    if(!this->bReady) return 0;
    if(!this->bStream) {
        engine->showMessageError("Programmer Error", "Called getPositionMS on a sample!");
        return 0;
    }
    if(this->bPaused) {
        this->interpolator.reset(this->paused_position_ms, Timing::getTimeReal(), this->getSpeed());
        return this->paused_position_ms;
    }

    i64 positionBytes = 0;
    if(this->isPlaying()) {
        positionBytes = BASS_Mixer_ChannelGetPosition(this->stream, BASS_POS_BYTE);
    } else {
        positionBytes = BASS_ChannelGetPosition(this->stream, BASS_POS_BYTE);
    }
    if(positionBytes < 0) {
        // The stream ended and got freed by BASS_STREAM_AUTOFREE -> invalid handle!
        this->interpolator.reset(this->length, Timing::getTimeReal(), this->getSpeed());
        return this->length;
    }

    f64 positionInSeconds = BASS_ChannelBytes2Seconds(this->stream, positionBytes);
    f64 rawPositionMS = positionInSeconds * 1000.0;

    // get interpolated position
    u32 interpolatedPositionMS =
        this->interpolator.update(rawPositionMS, Timing::getTimeReal(), this->getSpeed(), this->isLooped(),
                                  static_cast<u64>(this->length), this->isPlaying());

    return interpolatedPositionMS;
}

u32 BassSound::getLengthMS() {
    if(!this->bReady) return 0;
    return this->length;
}

float BassSound::getSpeed() { return this->fSpeed; }

float BassSound::getFrequency() {
    auto default_freq = cv::snd_freq.getFloat();
    if(!this->bReady) return default_freq;
    if(!this->bStream) {
        engine->showMessageError("Programmer Error", "Called getFrequency on a sample!");
        return default_freq;
    }

    float frequency = default_freq;
    BASS_ChannelGetAttribute(this->stream, BASS_ATTRIB_FREQ, &frequency);
    return frequency;
}

bool BassSound::isPlaying() {
    return this->bReady && this->bStarted && !this->bPaused && !this->getActiveChannels().empty();
}

bool BassSound::isFinished() { return this->bReady && this->bStarted && !this->isPlaying(); }

void BassSound::rebuild(std::string newFilePath) {
    this->sFilePath = std::move(newFilePath);
    resourceManager->reloadResource(this);
}

#endif
