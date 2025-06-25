#include "Sound.h"

#include <sstream>

#include "Bancho.h"
#include "ConVar.h"
#include "Engine.h"
#include "File.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "SoundEngine.h"

using namespace std;

Sound::Sound(std::string filepath, bool stream, bool overlayable, bool loop) : Resource(filepath) {
    this->sample = 0;
    this->stream = 0;
    this->bStream = stream;
    this->bIsLooped = loop;
    this->bIsOverlayable = overlayable;
    this->fSpeed = 1.0f;
    this->fVolume = 1.0f;
}

std::vector<HCHANNEL> Sound::getActiveChannels() {
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

HCHANNEL Sound::getChannel() {
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

void Sound::init() {
    if(m_sFilePath.length() < 2 || !m_bAsyncReady) return;

    // HACKHACK: re-set some values to their defaults (only necessary because of the existence of rebuild())
    this->fSpeed = 1.0f;

    // error checking
    if(this->sample == 0 && this->stream == 0) {
        UString msg = "Couldn't load sound \"";
        msg.append(m_sFilePath.c_str());
        msg.append(UString::format("\", stream = %i, errorcode = %i", (int)this->bStream, BASS_ErrorGetCode()));
        msg.append(", file = ");
        msg.append(m_sFilePath.c_str());
        msg.append("\n");
        debugLog(0xffdd3333, "%s", msg.toUtf8());
    } else {
        m_bReady = true;
    }
}

void Sound::initAsync() {
    if(cv_debug_rm.getBool()) debugLog("Resource Manager: Loading %s\n", m_sFilePath.c_str());

    // HACKHACK: workaround for BASS crashes on malformed WAV files
    {
        const int minWavFileSize = cv_snd_wav_file_min_size.getInt();
        if(minWavFileSize > 0) {
            auto fileExtensionLowerCase = UString(env->getFileExtensionFromFilePath(m_sFilePath).c_str());
            fileExtensionLowerCase.lowerCase();
            if(fileExtensionLowerCase == UString("wav")) {
                File wavFile(m_sFilePath);
                if(wavFile.getFileSize() < (size_t)minWavFileSize) {
                    printf("Sound: Ignoring malformed/corrupt WAV file (%i) %s\n", (int)wavFile.getFileSize(),
                           m_sFilePath.c_str());
                    return;
                }
            }
        }
    }

#ifdef _WIN32
    // On Windows, we need to convert the UTF-8 path to UTF-16, or paths with unicode characters will fail to open
    int size = MultiByteToWideChar(CP_UTF8, 0, m_sFilePath.c_str(), m_sFilePath.length(), NULL, 0);
    std::wstring file_path(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, m_sFilePath.c_str(), m_sFilePath.length(), (LPWSTR)file_path.c_str(),
                        file_path.length());
#else
    std::string file_path = m_sFilePath;
#endif

    if(this->bStream) {
        auto flags = BASS_STREAM_DECODE | BASS_SAMPLE_FLOAT | BASS_STREAM_PRESCAN;
        if(cv_snd_async_buffer.getInt() > 0) flags |= BASS_ASYNCFILE;
        if(env->getOS() == Environment::OS::WINDOWS) flags |= BASS_UNICODE;

        this->stream = BASS_StreamCreateFile(false, file_path.c_str(), 0, 0, flags);
        if(!this->stream) {
            debugLog("BASS_StreamCreateFile() returned error %d on file %s\n", BASS_ErrorGetCode(),
                     m_sFilePath.c_str());
            return;
        }

        this->stream = BASS_FX_TempoCreate(this->stream, BASS_FX_FREESOURCE | BASS_STREAM_DECODE);
        if(!this->stream) {
            debugLog("BASS_FX_TempoCreate() returned error %d on file %s\n", BASS_ErrorGetCode(),
                     m_sFilePath.c_str());
            return;
        }

        // Only compute the length once
        i64 length = BASS_ChannelGetLength(this->stream, BASS_POS_BYTE);
        f64 lengthInSeconds = BASS_ChannelBytes2Seconds(this->stream, length);
        f64 lengthInMilliSeconds = lengthInSeconds * 1000.0;
        this->length = (u32)lengthInMilliSeconds;
    } else {
        auto flags = BASS_SAMPLE_FLOAT;
        if(env->getOS() == Environment::OS::WINDOWS) flags |= BASS_UNICODE;

        this->sample = BASS_SampleLoad(false, file_path.c_str(), 0, 0, 1, flags);
        if(!this->sample) {
            auto code = BASS_ErrorGetCode();
            if(code == BASS_ERROR_EMPTY) {
                debugLog("Sound: Ignoring empty file %s\n", m_sFilePath.c_str());
                return;
            } else {
                debugLog("BASS_SampleLoad() returned error %d on file %s\n", code, m_sFilePath.c_str());
                return;
            }
        }

        // Only compute the length once
        i64 length = BASS_ChannelGetLength(this->sample, BASS_POS_BYTE);
        f64 lengthInSeconds = BASS_ChannelBytes2Seconds(this->sample, length);
        f64 lengthInMilliSeconds = lengthInSeconds * 1000.0;
        this->length = (u32)lengthInMilliSeconds;
    }

    m_bAsyncReady = true;
}

void Sound::destroy() {
    if(!m_bReady) return;

    this->bStarted = false;
    m_bReady = false;
    m_bAsyncReady = false;
    this->fLastPlayTime = 0.0;
    this->fChannelCreationTime = 0.0;
    this->bPaused = false;
    this->paused_position_ms = 0;

    if(this->bStream) {
        BASS_Mixer_ChannelRemove(this->stream);
        BASS_ChannelStop(this->stream);
        BASS_StreamFree(this->stream);
        this->stream = 0;
    } else {
        for(auto chan : this->mixer_channels) {
            BASS_Mixer_ChannelRemove(chan);
            BASS_ChannelStop(chan);
            BASS_ChannelFree(chan);
        }
        this->mixer_channels.clear();

        BASS_SampleStop(this->sample);
        BASS_SampleFree(this->sample);
        this->sample = 0;
    }
}

u32 Sound::setPosition(f64 percent) {
    u32 ms = clamp<f64>(percent, 0.0, 1.0) * this->length;
    this->setPositionMS(ms);
    return ms;
}

void Sound::setPositionMS(unsigned long ms) {
    if(!m_bReady || ms > this->getLengthMS()) return;
    if(!this->bStream) {
        engine->showMessageError("Programmer Error", "Called setPositionMS on a sample!");
        return;
    }

    i64 target_pos = BASS_ChannelSeconds2Bytes(this->stream, ms / 1000.0);
    if(target_pos < 0) {
        debugLog("setPositionMS: error %d while calling BASS_ChannelSeconds2Bytes\n", BASS_ErrorGetCode());
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
                if(cv_debug.getBool()) {
                    debugLog("Sound::setPositionMS( %lu ) BASS_ChannelSetPosition() error %i on file %s\n", ms,
                             BASS_ErrorGetCode(), m_sFilePath.c_str());
                }
            }

            this->fLastPlayTime = this->fChannelCreationTime - ((f64)ms / 1000.0);
        } else {
            if(!BASS_ChannelSetPosition(this->stream, target_pos, BASS_POS_BYTE | BASS_POS_DECODETO | BASS_POS_FLUSH)) {
                if(cv_debug.getBool()) {
                    debugLog("Sound::setPositionMS( %lu ) BASS_ChannelSetPosition() error %i on file %s\n", ms,
                             BASS_ErrorGetCode(), m_sFilePath.c_str());
                }
            }
        }
    } else {
        // Unlucky path, we have to reload the stream
        auto pan = this->getPan();
        auto loop = this->isLooped();
        auto speed = this->getSpeed();

        this->reload();

        this->setSpeed(speed);
        this->setPan(pan);
        this->setLoop(loop);
        this->bPaused = true;
        this->paused_position_ms = ms;

        if(!BASS_ChannelSetPosition(this->stream, target_pos, BASS_POS_BYTE | BASS_POS_DECODETO | BASS_POS_FLUSH)) {
            if(cv_debug.getBool()) {
                debugLog("Sound::setPositionMS( %lu ) BASS_ChannelSetPosition() error %i on file %s\n", ms,
                         BASS_ErrorGetCode(), m_sFilePath.c_str());
            }
        }

        if(was_playing) {
            osu->music_unpause_scheduled = true;
        }
    }
}

// Inaccurate but fast seeking, to use at song select
void Sound::setPositionMS_fast(u32 ms) {
    if(!m_bReady || ms > this->getLengthMS()) return;
    if(!this->bStream) {
        engine->showMessageError("Programmer Error", "Called setPositionMS_fast on a sample!");
        return;
    }

    i64 target_pos = BASS_ChannelSeconds2Bytes(this->stream, ms / 1000.0);
    if(target_pos < 0) {
        debugLog("setPositionMS_fast: error %d while calling BASS_ChannelSeconds2Bytes\n", BASS_ErrorGetCode());
        return;
    }

    if(this->isPlaying()) {
        if(!BASS_Mixer_ChannelSetPosition(this->stream, target_pos, BASS_POS_BYTE | BASS_POS_MIXER_RESET)) {
            if(cv_debug.getBool()) {
                debugLog("Sound::setPositionMS_fast( %lu ) BASS_ChannelSetPosition() error %i on file %s\n", ms,
                         BASS_ErrorGetCode(), m_sFilePath.c_str());
            }
        }

        this->fLastPlayTime = this->fChannelCreationTime - ((f64)ms / 1000.0);
    } else {
        if(!BASS_ChannelSetPosition(this->stream, target_pos, BASS_POS_BYTE | BASS_POS_FLUSH)) {
            if(cv_debug.getBool()) {
                debugLog("Sound::setPositionMS( %lu ) BASS_ChannelSetPosition() error %i on file %s\n", ms,
                         BASS_ErrorGetCode(), m_sFilePath.c_str());
            }
        }
    }
}

void Sound::setVolume(float volume) {
    if(!m_bReady) return;

    this->fVolume = clamp<float>(volume, 0.0f, 2.0f);

    for(auto channel : this->getActiveChannels()) {
        BASS_ChannelSetAttribute(channel, BASS_ATTRIB_VOL, this->fVolume);
    }
}

void Sound::setSpeed(float speed) {
    if(!m_bReady) return;
    if(!this->bStream) {
        engine->showMessageError("Programmer Error", "Called setSpeed on a sample!");
        return;
    }

    speed = clamp<float>(speed, 0.05f, 50.0f);

    float freq = cv_snd_freq.getFloat();
    BASS_ChannelGetAttribute(this->stream, BASS_ATTRIB_FREQ, &freq);

    BASS_ChannelSetAttribute(this->stream, BASS_ATTRIB_TEMPO, 1.0f);
    BASS_ChannelSetAttribute(this->stream, BASS_ATTRIB_TEMPO_FREQ, freq);

    if(cv_nightcore_enjoyer.getBool()) {
        BASS_ChannelSetAttribute(this->stream, BASS_ATTRIB_TEMPO_FREQ, speed * freq);
    } else {
        BASS_ChannelSetAttribute(this->stream, BASS_ATTRIB_TEMPO, (speed - 1.0f) * 100.0f);
    }

    this->fSpeed = speed;
}

void Sound::setFrequency(float frequency) {
    if(!m_bReady) return;

    frequency = (frequency > 99.0f ? clamp<float>(frequency, 100.0f, 100000.0f) : 0.0f);

    for(auto channel : this->getActiveChannels()) {
        BASS_ChannelSetAttribute(channel, BASS_ATTRIB_FREQ, frequency);
    }
}

void Sound::setPan(float pan) {
    if(!m_bReady) return;

    this->fPan = clamp<float>(pan, -1.0f, 1.0f);

    for(auto channel : this->getActiveChannels()) {
        BASS_ChannelSetAttribute(channel, BASS_ATTRIB_PAN, this->fPan);
    }
}

void Sound::setLoop(bool loop) {
    if(!m_bReady) return;
    if(!this->bStream) {
        engine->showMessageError("Programmer Error", "Called setLoop on a sample!");
        return;
    }

    this->bIsLooped = loop;
    BASS_ChannelFlags(this->stream, this->bIsLooped ? BASS_SAMPLE_LOOP : 0, BASS_SAMPLE_LOOP);
}

float Sound::getPosition() {
    if(!m_bReady) return 0.f;
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

    const float position = (float)((double)(positionBytes) / (double)(lengthBytes));
    return position;
}

u32 Sound::getPositionMS() {
    if(!m_bReady) return 0;
    if(!this->bStream) {
        engine->showMessageError("Programmer Error", "Called getPositionMS on a sample!");
        return 0;
    }
    if(this->bPaused) {
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
        return this->length;
    }

    f64 positionInSeconds = BASS_ChannelBytes2Seconds(this->stream, positionBytes);
    f64 positionInMilliSeconds = positionInSeconds * 1000.0;
    u32 positionMS = (u32)positionInMilliSeconds;
    if(!this->isPlaying()) {
        return positionMS;
    }

    // special case: a freshly started channel position jitters, lerp with engine time over a set duration to smooth
    // things over
    f64 interpDuration = cv_snd_play_interp_duration.getFloat();
    if(interpDuration <= 0.0) return positionMS;

    f64 channel_age = engine->getTime() - this->fChannelCreationTime;
    if(channel_age >= interpDuration) return positionMS;

    f64 speedMultiplier = this->getSpeed();
    f64 delta = channel_age * speedMultiplier;
    f64 interp_ratio = cv_snd_play_interp_ratio.getFloat();
    if(delta < interpDuration) {
        delta = (engine->getTime() - this->fLastPlayTime) * speedMultiplier;
        f64 lerpPercent = clamp<f64>(((delta / interpDuration) - interp_ratio) / (1.0 - interp_ratio), 0.0, 1.0);
        positionMS = (u32)lerp<f64>(delta * 1000.0, (f64)positionMS, lerpPercent);
    }

    return positionMS;
}

u32 Sound::getLengthMS() {
    if(!m_bReady) return 0;
    return this->length;
}

float Sound::getSpeed() { return this->fSpeed; }

float Sound::getFrequency() {
    auto default_freq = cv_snd_freq.getFloat();
    if(!m_bReady) return default_freq;
    if(!this->bStream) {
        engine->showMessageError("Programmer Error", "Called getFrequency on a sample!");
        return default_freq;
    }

    float frequency = default_freq;
    BASS_ChannelGetAttribute(this->stream, BASS_ATTRIB_FREQ, &frequency);
    return frequency;
}

bool Sound::isPlaying() {
    return m_bReady && this->bStarted && !this->bPaused && !this->getActiveChannels().empty();
}

bool Sound::isFinished() { return m_bReady && this->bStarted && !this->isPlaying(); }

void Sound::rebuild(std::string newFilePath) {
    m_sFilePath = newFilePath;
    this->reload();
}
