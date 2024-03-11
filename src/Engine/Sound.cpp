//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		sound wrapper, either streamed or preloaded
//
// $NoKeywords: $snd $os
//===============================================================================//

#include "Sound.h"

#define NOBASSOVERLOADS
#include <bass.h>
#include <bass_fx.h>
#include <bassmix.h>

#include <sstream>

#include "Bancho.h"
#include "ConVar.h"
#include "File.h"
#include "Osu.h"

#include "ConVar.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "SoundEngine.h"

ConVar snd_play_interp_duration(
    "snd_play_interp_duration", 0.75f, FCVAR_NONE,
    "smooth over freshly started channel position jitter with engine time over this duration in seconds");
ConVar snd_play_interp_ratio("snd_play_interp_ratio", 0.50f, FCVAR_NONE,
                             "percentage of snd_play_interp_duration to use 100% engine time over audio time (some "
                             "devices report 0 for very long)");

ConVar snd_wav_file_min_size("snd_wav_file_min_size", 51, FCVAR_NONE,
                             "minimum file size in bytes for WAV files to be considered valid (everything below will "
                             "fail to load), this is a workaround for BASS crashes");

Sound::Sound(std::string filepath, bool stream, bool overlayable, bool threeD, bool loop, bool prescan) : Resource(filepath) {
    m_sample = 0;
    m_stream = 0;
    m_bStream = stream;
    m_bIs3d = threeD;
    m_bIsLooped = loop;
    m_bPrescan = prescan;
    m_bIsOverlayable = overlayable;
    m_fSpeed = 1.0f;
    m_fVolume = 1.0f;
    m_fLastPlayTime = -1.0f;
}

std::vector<HCHANNEL> Sound::getActiveChannels() {
    std::vector<HCHANNEL> channels;

    if(engine->getSound()->isMixing()) {
        if(m_bStream) {
            if(BASS_Mixer_ChannelGetMixer(m_stream) != 0) {
                channels.push_back(m_stream);
            }
        } else {
            for(auto chan : mixer_channels) {
                if(BASS_Mixer_ChannelGetMixer(chan) != 0) {
                    channels.push_back(chan);
                }
            }

            // Only keep channels that are still playing
            mixer_channels = channels;
        }
    } else {
        if(m_bStream) {
            if(BASS_ChannelIsActive(m_stream) == BASS_ACTIVE_PLAYING) {
                channels.push_back(m_stream);
            }
        } else {
            HCHANNEL chans[MAX_OVERLAPPING_SAMPLES] = {0};
            int nb = BASS_SampleGetChannels(m_sample, chans);
            for(int i = 0; i < nb; i++) {
                if(BASS_ChannelIsActive(chans[i]) == BASS_ACTIVE_PLAYING) {
                    channels.push_back(chans[i]);
                }
            }
        }
    }

    return channels;
}

HCHANNEL Sound::getChannel() {
    if(m_bStream) {
        return m_stream;
    } else {
        if(engine->getSound()->isMixing()) {
            // If we want to be able to control samples after playing them, we
            // have to store them here, since bassmix only accepts DECODE streams.
            auto chan = BASS_SampleGetChannel(m_sample, BASS_SAMCHAN_STREAM | BASS_STREAM_DECODE);
            mixer_channels.push_back(chan);
            return chan;
        } else {
            return BASS_SampleGetChannel(m_sample, 0);
        }
    }
}

void Sound::init() {
    if(m_sFilePath.length() < 2 || !m_bAsyncReady) return;

    // HACKHACK: re-set some values to their defaults (only necessary because of the existence of rebuild())
    m_fSpeed = 1.0f;

    // error checking
    if(m_sample == 0 && m_stream == 0) {
        UString msg = "Couldn't load sound \"";
        msg.append(m_sFilePath.c_str());
        msg.append(UString::format("\", stream = %i, errorcode = %i", (int)m_bStream, BASS_ErrorGetCode()));
        msg.append(", file = ");
        msg.append(m_sFilePath.c_str());
        msg.append("\n");
        debugLog(0xffdd3333, "%s", msg.toUtf8());
    } else {
        m_bReady = true;
    }
}

void Sound::initAsync() {
    if(ResourceManager::debug_rm->getBool()) debugLog("Resource Manager: Loading %s\n", m_sFilePath.c_str());

    // HACKHACK: workaround for BASS crashes on malformed WAV files
    {
        const int minWavFileSize = snd_wav_file_min_size.getInt(); 
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

    if(m_bStream) {
        auto flags = BASS_STREAM_DECODE | BASS_SAMPLE_FLOAT;
        if(m_bPrescan) flags |= BASS_STREAM_PRESCAN;

        m_stream = BASS_StreamCreateFile(false, m_sFilePath.c_str(), 0, 0, flags);
        if(!m_stream) {
            debugLog("BASS_StreamCreateFile() returned error %d on file %s\n", BASS_ErrorGetCode(), m_sFilePath.c_str());
            return;
        }

        auto fx_flags = BASS_FX_FREESOURCE;
        if(engine->getSound()->isMixing()) fx_flags |= BASS_STREAM_DECODE;
        m_stream = BASS_FX_TempoCreate(m_stream, fx_flags);
        if(!m_stream) {
            debugLog("BASS_FX_TempoCreate() returned error %d on file %s\n", BASS_ErrorGetCode(), m_sFilePath.c_str());
            return;
        }
    } else {
        auto flags = BASS_SAMPLE_FLOAT | BASS_SAMPLE_OVER_POS;
        if(m_bIs3d) flags |= BASS_SAMPLE_3D | BASS_SAMPLE_MONO;

        m_sample = BASS_SampleLoad(false, m_sFilePath.c_str(), 0, 0, m_bIsOverlayable ? MAX_OVERLAPPING_SAMPLES : 1, flags);
        if(!m_sample) {
            debugLog("BASS_SampleLoad() returned error %d on file %s\n", BASS_ErrorGetCode(), m_sFilePath.c_str());
            return;
        }
    }

    m_bAsyncReady = true;
}

void Sound::destroy() {
    if(!m_bReady) return;

    m_bReady = false;
    m_fLastPlayTime = 0.0;

    if(m_bStream) {
        if(engine->getSound()->isMixing()) {
            BASS_Mixer_ChannelRemove(m_stream);
        }

        BASS_ChannelStop(m_stream);
        BASS_StreamFree(m_stream);
        m_stream = 0;
    } else {
        BASS_SampleStop(m_sample);
        BASS_SampleFree(m_sample);
        m_sample = 0;
    }
}

void Sound::setPosition(double percent) {
    if(!m_bReady) return;
    if(!m_bStream) {
        engine->showMessageError("Programmer Error", "Called setPosition on a sample!");
        return;
    }

    percent = clamp<double>(percent, 0.0, 1.0);

    const double length = BASS_ChannelGetLength(m_stream, BASS_POS_BYTE);
    const double lengthInSeconds = BASS_ChannelBytes2Seconds(m_stream, length);

    // NOTE: abused for play interp
    if(lengthInSeconds * percent < snd_play_interp_duration.getFloat()) {
        m_fLastPlayTime = engine->getTime() - lengthInSeconds * percent;
    } else {
        m_fLastPlayTime = 0.0;
    }

    if(!BASS_ChannelSetPosition(m_stream, (QWORD)(length*percent), BASS_POS_BYTE)) {
        if(Osu::debug->getBool()) {
            debugLog("Sound::setPosition( %f ) BASS_ChannelSetPosition() error %i on file %s\n", percent,
                     BASS_ErrorGetCode(), m_sFilePath.c_str());
        }
    }
}

void Sound::setPositionMS(unsigned long ms) {
    if(!m_bReady || ms > getLengthMS()) return;
    if(!m_bStream) {
        engine->showMessageError("Programmer Error", "Called setPositionMS on a sample!");
        return;
    }

    const QWORD position = BASS_ChannelSeconds2Bytes(m_stream, ms / 1000.0);

    // NOTE: abused for play interp
    if((double)ms / 1000.0 < snd_play_interp_duration.getFloat())
        m_fLastPlayTime = engine->getTime() - ((double)ms / 1000.0);
    else
        m_fLastPlayTime = 0.0;

    if(!BASS_ChannelSetPosition(m_stream, position, BASS_POS_BYTE)) {
        if(Osu::debug->getBool()) {
            debugLog("Sound::setPositionMS( %lu ) BASS_ChannelSetPosition() error %i on file %s\n", ms, BASS_ErrorGetCode(),
                     m_sFilePath.c_str());
        }
    }
}

void Sound::setVolume(float volume) {
    if(!m_bReady) return;

    m_fVolume = clamp<float>(volume, 0.0f, 1.0f);

    for(auto channel : getActiveChannels()) {
        BASS_ChannelSetAttribute(channel, BASS_ATTRIB_VOL, m_fVolume);
    }
}

void Sound::setSpeed(float speed) {
    if(!m_bReady) return;
    if(!m_bStream) {
        engine->showMessageError("Programmer Error", "Called setSpeed on a sample!");
        return;
    }

    speed = clamp<float>(speed, 0.05f, 50.0f);

    float freq = convar->getConVarByName("snd_freq")->getFloat();
    BASS_ChannelGetAttribute(m_stream, BASS_ATTRIB_FREQ, &freq);

    BASS_ChannelSetAttribute(m_stream, BASS_ATTRIB_TEMPO, 1.0f);
    BASS_ChannelSetAttribute(m_stream, BASS_ATTRIB_TEMPO_FREQ, freq);

    bool nightcoring = bancho.osu->getModNC() || bancho.osu->getModDC();
    if(nightcoring) {
        BASS_ChannelSetAttribute(m_stream, BASS_ATTRIB_TEMPO_FREQ, speed * freq);
    } else {
        BASS_ChannelSetAttribute(m_stream, BASS_ATTRIB_TEMPO, (speed - 1.0f) * 100.0f);
    }

    m_fSpeed = speed;
}

void Sound::setFrequency(float frequency) {
    if(!m_bReady) return;

    frequency = (frequency > 99.0f ? clamp<float>(frequency, 100.0f, 100000.0f) : 0.0f);

    for(auto channel : getActiveChannels()) {
        BASS_ChannelSetAttribute(channel, BASS_ATTRIB_FREQ, frequency);
    }
}

void Sound::setPan(float pan) {
    if(!m_bReady) return;

    m_fPan = clamp<float>(pan, -1.0f, 1.0f);

    for(auto channel : getActiveChannels()) {
        BASS_ChannelSetAttribute(channel, BASS_ATTRIB_PAN, m_fPan);
    }
}

void Sound::setLoop(bool loop) {
    if(!m_bReady) return;
    if(!m_bStream) {
        engine->showMessageError("Programmer Error", "Called setLoop on a sample!");
        return;
    }

    m_bIsLooped = loop;
    BASS_ChannelFlags(m_stream, m_bIsLooped ? BASS_SAMPLE_LOOP : 0, BASS_SAMPLE_LOOP);
}

float Sound::getPosition() {
    if(!m_bReady) return 0.0f;
    if(!m_bStream) {
        engine->showMessageError("Programmer Error", "Called getPosition on a sample!");
        return 0.0f;
    }

    const QWORD lengthBytes = BASS_ChannelGetLength(m_stream, BASS_POS_BYTE);
    const QWORD positionBytes = BASS_ChannelGetPosition(m_stream, BASS_POS_BYTE);
    const float position = (float)((double)(positionBytes) / (double)(lengthBytes));
    return position;
}

unsigned long Sound::getPositionMS() {
    if(!m_bReady) return 0;
    if(!m_bStream) {
        engine->showMessageError("Programmer Error", "Called getPositionMS on a sample!");
        return 0;
    }

    const QWORD position = BASS_ChannelGetPosition(m_stream, BASS_POS_BYTE);
    const double positionInSeconds = BASS_ChannelBytes2Seconds(m_stream, position);
    const double positionInMilliSeconds = positionInSeconds * 1000.0;
    const unsigned long positionMS = static_cast<unsigned long>(positionInMilliSeconds);

    // special case: a freshly started channel position jitters, lerp with engine time over a set duration to smooth
    // things over
    const double interpDuration = snd_play_interp_duration.getFloat();
    const unsigned long interpDurationMS = interpDuration * 1000;
    if(interpDuration > 0.0 && positionMS < interpDurationMS) {
        const float speedMultiplier = getSpeed();
        const double delta = (engine->getTime() - m_fLastPlayTime) * speedMultiplier;
        if(m_fLastPlayTime > 0.0 && delta < interpDuration && isPlaying()) {
            const double lerpPercent = clamp<double>(((delta / interpDuration) - snd_play_interp_ratio.getFloat()) /
                                                         (1.0 - snd_play_interp_ratio.getFloat()),
                                                     0.0, 1.0);
            return static_cast<unsigned long>(lerp<double>(delta * 1000.0, (double)positionMS, lerpPercent));
        }
    }

    return positionMS;
}

unsigned long Sound::getLengthMS() {
    if(!m_bReady) return 0;
    if(!m_bStream) {
        engine->showMessageError("Programmer Error", "Called getLengthMS on a sample!");
        return 0;
    }

    const QWORD length = BASS_ChannelGetLength(m_stream, BASS_POS_BYTE);
    const double lengthInSeconds = BASS_ChannelBytes2Seconds(m_stream, length);
    const double lengthInMilliSeconds = lengthInSeconds * 1000.0;
    return static_cast<unsigned long>(lengthInMilliSeconds);
}

float Sound::getSpeed() {
    return m_fSpeed;
}

float Sound::getFrequency() {
    auto default_freq = convar->getConVarByName("snd_freq")->getFloat();
    if(!m_bReady) return default_freq;
    if(!m_bStream) {
        engine->showMessageError("Programmer Error", "Called getFrequency on a sample!");
        return default_freq;
    }

    float frequency = default_freq;
    BASS_ChannelGetAttribute(m_stream, BASS_ATTRIB_FREQ, &frequency);
    return frequency;
}

bool Sound::isPlaying() {
    if(!m_bReady) return false;

    auto channels = getActiveChannels();
    return !channels.empty();
}

bool Sound::isFinished() {
    if(!m_bReady) return false;

    if(m_bStream) {
        return BASS_ChannelIsActive(m_stream) == BASS_ACTIVE_STOPPED;
    } else {
        return getActiveChannels().empty();
    }
}

void Sound::rebuild(std::string newFilePath) {
    m_sFilePath = newFilePath;

    reload();
}
