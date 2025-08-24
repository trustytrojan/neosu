#pragma once
// Copyright (c) 2014, PG, All rights reserved.

#include <memory>
#include "Resource.h"
#include "PlaybackInterpolator.h"

#define SOUND_TYPE(ClassName, TypeID, ParentClass)                      \
    static constexpr TypeId TYPE_ID = TypeID;                           \
    [[nodiscard]] TypeId getTypeId() const override { return TYPE_ID; } \
    [[nodiscard]] bool isTypeOf(TypeId typeId) const override {         \
        return typeId == TYPE_ID || ParentClass::isTypeOf(typeId);      \
    }

class SoundEngine;

using SOUNDHANDLE = uint32_t;

class Sound : public Resource {
    friend class SoundEngine;

   public:
    using TypeId = uint8_t;
    enum SndType : TypeId { BASS, SOLOUD };

   public:
    Sound(std::string filepath, bool stream, bool overlayable, bool loop)
        : Resource(std::move(filepath)), bStream(stream), bIsLooped(loop), bIsOverlayable(overlayable) {}

    // Factory method to create the appropriate sound object
    static Sound *createSound(std::string filepath, bool stream, bool overlayable, bool loop);

    virtual void setPositionMS(u32 ms) = 0;

    virtual void setVolume(float volume) = 0;
    virtual void setSpeed(float speed) = 0;
    virtual void setPitch(float pitch) { this->fPitch = pitch; }
    virtual void setFrequency(float frequency) = 0;
    virtual void setPan(float pan) = 0;
    virtual void setLoop(bool loop) = 0;

    virtual float getPosition() = 0;
    virtual u32 getPositionMS() = 0;
    virtual u32 getLengthMS() = 0;
    virtual float getFrequency() = 0;
    virtual float getPan() { return this->fPan; }
    virtual float getSpeed() { return this->fSpeed; }
    virtual float getPitch() { return this->fPitch; }

    virtual bool isPlaying() = 0;
    virtual bool isFinished() = 0;

    virtual void rebuild(std::string newFilePath) = 0;

    [[nodiscard]] constexpr bool isStream() const { return this->bStream; }
    [[nodiscard]] constexpr bool isLooped() const { return this->bIsLooped; }
    [[nodiscard]] constexpr bool isOverlayable() const { return this->bIsOverlayable; }

    // type inspection
    [[nodiscard]] Type getResType() const final { return SOUND; }

    Sound *asSound() final { return this; }
    [[nodiscard]] const Sound *asSound() const final { return this; }

    // type inspection
    [[nodiscard]] virtual TypeId getTypeId() const = 0;
    [[nodiscard]] virtual bool isTypeOf(TypeId /*type_id*/) const { return false; }
    template <typename T>
    [[nodiscard]] bool isType() const {
        return isTypeOf(T::TYPE_ID);
    }
    template <typename T>
    T *as() {
        return isType<T>() ? static_cast<T *>(this) : nullptr;
    }
    template <typename T>
    const T *as() const {
        return isType<T>() ? static_cast<const T *>(this) : nullptr;
    }

   protected:
    void init() override = 0;
    void initAsync() override;
    void destroy() override = 0;

    bool bStream;
    bool bIsLooped;
    bool bIsOverlayable;

    bool bIgnored{false};  // early check for audio file validity
    bool bStarted{false};
    bool bPaused{false};

    float fPan{0.0f};
    float fSpeed{1.0f};
    float fPitch{1.0f};
    float fVolume{1.0f};
    u32 paused_position_ms{0};
    u32 length{0};

    PlaybackInterpolator interpolator;

   private:
    static bool isValidAudioFile(const std::string &filePath, const std::string &fileExt);
};
