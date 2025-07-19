#pragma once

#include "Sound.h"
#ifdef MCENGINE_FEATURE_BASS
#include "BassManager.h"

class BassSoundEngine;

class BassSound final : public Sound {
    friend class BassSoundEngine;

   public:
    BassSound(std::string filepath, bool stream, bool overlayable, bool loop)
        : Sound(std::move(filepath), stream, overlayable, loop) {};
    ~BassSound() override { this->destroy(); }

    BassSound &operator=(const BassSound &) = delete;
    BassSound &operator=(BassSound &&) = delete;

    BassSound(const BassSound &) = delete;
    BassSound(BassSound &&) = delete;

    u32 setPosition(f64 percent) override;
    void setPositionMS(unsigned long ms) override;
    void setPositionMS_fast(u32 ms) override;

    void setVolume(float volume) override;
    void setSpeed(float speed) override;
    void setFrequency(float frequency) override;
    void setPan(float pan) override;
    void setLoop(bool loop) override;

    float getPosition() override;
    u32 getPositionMS() override;
    u32 getLengthMS() override;
    float getSpeed() override;
    float getFrequency() override;

    bool isPlaying() override;
    bool isFinished() override;

    void rebuild(std::string newFilePath) override;

    // inspection
    SOUND_TYPE(BassSound, BASS, Sound)

   private:
    void init() override;
    void initAsync() override;
    void destroy() override;

    std::vector<HCHANNEL> getActiveChannels();
    HCHANNEL getChannel();

    f64 fChannelCreationTime{0.0};

    std::vector<HCHANNEL> mixer_channels;
    SOUNDHANDLE stream{0};
    SOUNDHANDLE sample{0};
};

#else
class BassSound : public Sound {};
#endif
