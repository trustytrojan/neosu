//================ Copyright (c) 2025, WH, All rights reserved. ==================//
//
// Purpose:		SoLoud-specific sound implementation
//
// $NoKeywords: $snd $soloud
//================================================================================//

#include "SoLoudSound.h"

#ifdef MCENGINE_FEATURE_SOLOUD

#include <soloud.h>
#include <soloud_file.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>

#include "SoLoudFX.h"
#include "SoLoudSoundEngine.h"

#include "ConVar.h"
#include "Engine.h"
#include "File.h"
#include "ResourceManager.h"

void SoLoudSound::init() {
    if(this->bIgnored || this->sFilePath.length() < 2 || !(this->bAsyncReady.load())) return;

    if(!this->audioSource)
        debugLogF(0xffdd3333, "Couldn't load sound \"{}\", stream = {}, file = {}\n", this->sFilePath, this->bStream,
                  this->sFilePath);
    else
        this->bReady = true;
}

SoLoudSound::~SoLoudSound() { this->destroy(); }

void SoLoudSound::initAsync() {
    Sound::initAsync();
    if(this->bIgnored) return;

    // clean up any previous instance
    if(this->audioSource) {
        if(this->bStream)
            delete static_cast<SoLoud::SLFXStream *>(this->audioSource);
        else
            delete static_cast<SoLoud::Wav *>(this->audioSource);

        this->audioSource = nullptr;
    }

    // load file into memory first to handle unicode paths properly (windows shenanigans)
    std::vector<char> fileBuffer;
    const char *fileData{nullptr};
    size_t fileSize{0};

    if constexpr(Env::cfg(OS::WINDOWS)) {
        File file(this->sFilePath);

        if(!file.canRead()) {
            debugLogF("Sound Error: Cannot open file {:s}\n", this->sFilePath);
            return;
        }

        fileSize = file.getFileSize();
        if(fileSize == 0) {
            debugLogF("Sound Error: File is empty {:s}\n", this->sFilePath);
            return;
        }

        fileBuffer = file.takeFileBuffer();
        fileData = fileBuffer.data();
        if(!fileData) {
            debugLogF("Sound Error: Failed to read file data {:s}\n", this->sFilePath);
            return;
        }
        // file is closed here
    }

    // create the appropriate audio source based on streaming flag
    SoLoud::result result = SoLoud::SO_NO_ERROR;
    if(this->bStream) {
        // use SLFXStream for streaming audio (music, etc.) includes rate/pitch processing like BASS_FX_TempoCreate
        auto *stream = new SoLoud::SLFXStream(cv::snd_soloud_prefer_ffmpeg.getInt() > 0);

        // use loadToMem for streaming to handle unicode paths on windows
        if constexpr(Env::cfg(OS::WINDOWS))
            result = stream->loadMem(reinterpret_cast<const unsigned char *>(fileData), fileSize, true, false);
        else
            result = stream->load(this->sFilePath.c_str());

        if(result == SoLoud::SO_NO_ERROR) {
            this->audioSource = stream;
            this->fFrequency = stream->mBaseSamplerate;

            this->audioSource->setInaudibleBehavior(
                true, false);  // keep ticking the sound if it goes to 0 volume, and don't kill it

            if(cv::debug_snd.getBool())
                debugLogF(
                    "SoLoudSound: Created SLFXStream for {:s} with speed={:f}, pitch={:f}, looping={:s}, "
                    "decoder={:s}\n",
                    this->sFilePath, this->fSpeed, this->fPitch, this->bIsLooped ? "true" : "false",
                    stream->getDecoder());
        } else {
            delete stream;
            debugLogF("Sound Error: SLFXStream::load() error {} on file {:s}\n", result, this->sFilePath);
            return;
        }
    } else {
        // use Wav for non-streaming audio (hit sounds, effects, etc.)
        auto *wav = new SoLoud::Wav(cv::snd_soloud_prefer_ffmpeg.getInt() > 1);

        if constexpr(Env::cfg(OS::WINDOWS))
            result = wav->loadMem(reinterpret_cast<const unsigned char *>(fileData), fileSize, true, false);
        else
            result = wav->load(this->sFilePath.c_str());

        if(result == SoLoud::SO_NO_ERROR) {
            this->audioSource = wav;
            this->fFrequency = wav->mBaseSamplerate;

            this->audioSource->setInaudibleBehavior(
                true, true);  // keep ticking the sound if it goes to 0 volume, but do kill it if necessary
        } else {
            delete wav;
            debugLogF("Sound Error: SoLoud::Wav::load() error {} on file {:s}\n", result, this->sFilePath);
            return;
        }
    }

    // only play one music track at a time
    this->audioSource->setSingleInstance(this->bStream || !this->bIsOverlayable);
    this->audioSource->setLooping(this->bIsLooped);

    this->bAsyncReady = true;
}

SOUNDHANDLE SoLoudSound::getHandle() { return this->handle; }

void SoLoudSound::destroy() {
    if(!this->bReady) return;

    this->bReady = false;

    // stop the sound if it's playing
    if(this->handle != 0) {
        soloud->stop(this->handle);
        this->handle = 0;
    }

    // clean up audio source
    if(this->audioSource) {
        if(this->bStream)
            delete static_cast<SoLoud::SLFXStream *>(this->audioSource);
        else
            delete static_cast<SoLoud::Wav *>(this->audioSource);

        this->audioSource = nullptr;
    }

    // need to reset this because the soloud handle has been destroyed
    this->bIsLoopingActuallySet = false;
    this->fFrequency = 44100.0f;
    this->fPitch = 1.0f;
    this->fSpeed = 1.0f;
    this->fPan = 0.0f;
    this->fVolume = 1.0f;
    this->fLastPlayTime = 0.0f;
    this->bIgnored = false;
}

u32 SoLoudSound::setPosition(f64 percent) {
    if(!this->bReady || !this->audioSource || !this->handle) return 0;

    percent = std::clamp<double>(percent, 0.0, 1.0);

    // calculate position based on the ORIGINAL timeline
    const double streamLengthInSeconds = getSourceLengthInSeconds();
    double positionInSeconds = streamLengthInSeconds * percent;

    if(cv::debug_snd.getBool())
        debugLogF("seeking to {:.2f} percent (position: {}ms, length: {}ms)\n", percent,
                  static_cast<unsigned long>(positionInSeconds * 1000),
                  static_cast<unsigned long>(streamLengthInSeconds * 1000));

    // seek
    soloud->seek(this->handle, positionInSeconds);

    // reset position interp vars
    this->interpolator.reset(getStreamPositionInSeconds() / 1000.0, Timing::getTimeReal(), getSpeed());
    return static_cast<u32>(positionInSeconds * 1000);
}

void SoLoudSound::setPositionMS(unsigned long ms) {
    if(!this->bReady || !this->audioSource || !this->handle) return;

    auto msD = static_cast<double>(ms);

    auto streamLengthMS = static_cast<double>(getLengthMS());
    if(msD > streamLengthMS) return;

    double positionInSeconds = msD / 1000.0;

    if(cv::debug_snd.getBool()) debugLogF("seeking to {:g}ms (length: {:g}ms)\n", msD, streamLengthMS);

    // seek
    soloud->seek(this->handle, positionInSeconds);

    // reset position interp vars
    this->interpolator.reset(getStreamPositionInSeconds() / 1000.0, Timing::getTimeReal(), getSpeed());
}

void SoLoudSound::setVolume(float volume) {
    if(!this->bReady) return;

    this->fVolume = std::clamp<float>(volume, 0.0f, 1.0f);

    if(!this->handle) return;

    // apply to active voice if not overlayable
    if(!this->bIsOverlayable) soloud->setVolume(this->handle, this->fVolume);
}

void SoLoudSound::setSpeed(float speed) {
    if(!this->bReady || !this->audioSource || !this->handle) return;

    // sample speed could be supported, but there is nothing using it right now so i will only bother when the time
    // comes
    if(!this->bStream) {
        debugLogF("Programmer Error: tried to setSpeed on a sample!\n");
        return;
    }

    speed = std::clamp<float>(speed, 0.05f, 50.0f);

    if(this->fSpeed != speed) {
        float previousSpeed = this->fSpeed;
        this->fSpeed = speed;

        auto *filteredStream = static_cast<SoLoud::SLFXStream *>(this->audioSource);

        if(cv::nightcore_enjoyer.getBool()) {
            this->setPitch(1.0f);  // make sure the filter pitch is reset
            filteredStream->setSpeedFactor(1.0f);
            // then directly set the relative play speed
            soloud->setRelativePlaySpeed(this->handle, speed);
        } else {
            soloud->setRelativePlaySpeed(this->handle, 1.0f);
            // update the SLFXStream parameters
            filteredStream->setSpeedFactor(this->fSpeed);
        }

        if(cv::debug_snd.getBool())
            debugLogF("SoLoudSound: Speed change {:s}: {:f}->{:f} (nightcore_enjoyer={})\n", this->sFilePath,
                      previousSpeed, this->fSpeed, cv::nightcore_enjoyer.getBool());
    }
}

void SoLoudSound::setPitch(float pitch) {
    if(!this->bReady || !this->audioSource) return;

    // sample pitch could be supported, but there is nothing using it right now so i will only bother when the time
    // comes
    if(!this->bStream) {
        debugLogF("Programmer Error: tried to this->setPitch on a sample!\n");
        return;
    }

    pitch = std::clamp<float>(pitch, 0.0f, 2.0f);

    if(this->fPitch != pitch) {
        float previousPitch = this->fPitch;
        this->fPitch = pitch;

        // simply update the SLFXStream parameters
        auto *stream = static_cast<SoLoud::SLFXStream *>(this->audioSource);
        stream->setPitchFactor(this->fPitch);

        if(cv::debug_snd.getBool())
            debugLogF("SoLoudSound: Pitch change {:s}: {:f}->{:f} (stream, updated live)\n", this->sFilePath,
                      previousPitch, this->fPitch);
    }
}

void SoLoudSound::setFrequency(float frequency) {
    if(!this->bReady || !this->audioSource) return;

    frequency = (frequency > 99.0f ? std::clamp<float>(frequency, 100.0f, 100000.0f) : 0.0f);

    if(this->fFrequency != frequency) {
        if(frequency > 0) {
            if(this->bStream) {
                float pitchRatio = frequency / this->fFrequency;

                // apply the frequency change through pitch
                // this isn't the only or even a good way, but it does the trick
                this->setPitch(this->fPitch * pitchRatio);
            } else if(this->handle) {
                soloud->setSamplerate(this->handle, frequency);
            }
            this->fFrequency = frequency;
        } else  // 0 means reset to default
        {
            if(this->bStream)
                this->setPitch(1.0f);
            else if(this->handle)
                soloud->setSamplerate(this->handle, frequency);
            this->fFrequency = this->audioSource->mBaseSamplerate;
        }
    }
}

void SoLoudSound::setPan(float pan) {
    if(!this->bReady || !this->handle) return;

    pan = std::clamp<float>(pan, -1.0f, 1.0f);

    // apply to the active voice
    soloud->setPan(this->handle, pan);
}

void SoLoudSound::setLoop(bool loop) {
    if(!this->bReady || !this->audioSource || this->bIsLoopingActuallySet == loop) return;

    this->bIsLooped = loop;

    if(cv::debug_snd.getBool()) debugLogF("setLoop {}\n", loop);

    // apply to the source
    this->audioSource->setLooping(loop);

    // apply to the active voice
    if(this->handle != 0) {
        soloud->setLooping(this->handle, loop);
        this->bIsLoopingActuallySet = loop;
    }
}

void SoLoudSound::setOverlayable(bool overlayable) {
    this->bIsOverlayable = overlayable;
    if(this->audioSource) this->audioSource->setSingleInstance(!overlayable);
}

float SoLoudSound::getPosition() {
    if(!this->bReady || !this->audioSource || !this->handle) return 0.0f;

    double streamLengthInSeconds = getSourceLengthInSeconds();
    if(streamLengthInSeconds <= 0.0) return 0.0f;

    double streamPositionInSeconds = getStreamPositionInSeconds();

    return std::clamp<float>(streamPositionInSeconds / streamLengthInSeconds, 0.0f, 1.0f);
}

// slightly tweaked interp algo from the SDL_mixer version, to smooth out position updates
u32 SoLoudSound::getPositionMS() {
    if(!this->bReady || !this->audioSource || !this->handle) return 0;

    return this->interpolator.update(getStreamPositionInSeconds() * 1000.0, Timing::getTimeReal(), getSpeed(),
                                     isLooped(), getLengthMS(), isPlaying());
}

u32 SoLoudSound::getLengthMS() {
    if(!this->bReady || !this->audioSource) return 0;

    const double lengthInMilliSeconds = getSourceLengthInSeconds() * 1000.0;
    // if (cv::debug_snd.getBool())
    // 	debugLogF("lengthMS for {:s}: {:g}\n", this->sFilePath, lengthInMilliSeconds);
    return static_cast<u32>(lengthInMilliSeconds);
}

float SoLoudSound::getSpeed() {
    if(!this->bReady) return 1.0f;

    return this->fSpeed;
}

float SoLoudSound::getPitch() {
    if(!this->bReady) return 1.0f;

    return this->fPitch;
}

bool SoLoudSound::isPlaying() {
    if(!this->bReady || !this->handle) return false;

    // a sound is playing if the handle is valid and the sound isn't paused
    if(!soloud->isValidVoiceHandle(this->handle)) {
        this->handle = 0;
    }

    return (this->handle != 0) && !soloud->getPause(this->handle);
}

bool SoLoudSound::isFinished() {
    if(!this->bReady) return false;

    if(this->handle == 0) return true;

    // a sound is finished if the handle is no longer valid
    if(!soloud->isValidVoiceHandle(this->handle)) {
        this->handle = 0;
    }

    return this->handle == 0;
}

void SoLoudSound::rebuild(std::string newFilePath) {
    this->sFilePath = std::move(newFilePath);
    resourceManager->reloadResource(this);
}

// soloud-specific accessors

double SoLoudSound::getStreamPositionInSeconds() const {
    if(!this->audioSource) return this->interpolator.getLastInterpolatedPositionMS() / 1000.0;
    if(this->bStream)
        return static_cast<SoLoud::SLFXStream *>(this->audioSource)->getRealStreamPosition();
    else
        return this->handle ? soloud->getStreamPosition(this->handle)
                            : this->interpolator.getLastInterpolatedPositionMS() / 1000.0;
}

double SoLoudSound::getSourceLengthInSeconds() const {
    if(!this->audioSource) return 0.0;
    if(this->bStream)
        return static_cast<SoLoud::SLFXStream *>(this->audioSource)->getLength();
    else
        return static_cast<SoLoud::Wav *>(this->audioSource)->getLength();
}

#endif  // MCENGINE_FEATURE_SOLOUD
