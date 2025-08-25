#pragma once

#include "Sound.h"
#ifdef MCENGINE_FEATURE_BASS
#include "BassManager.h"

#include <vector>
class BassSoundEngine;

struct BassHandle {
    HCHANNEL channel;

    // Volume of the channel *before* applying BassSound->fVolume
    f32 base_volume;
};

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

    void setPositionMS(u32 ms) override;

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

    std::vector<BassHandle> getActiveHandles();
    BassHandle getNewHandle(f32 volume);

    std::vector<BassHandle> mixer_handles;
    SOUNDHANDLE stream{0};
    SOUNDHANDLE sample{0};
};

#else
class BassSound : public Sound {};
#endif
