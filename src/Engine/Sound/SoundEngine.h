#pragma once

#include "Delegate.h"
#include "UString.h"

#define SOUND_ENGINE_TYPE(ClassName, TypeID, ParentClass)               \
    static constexpr TypeId TYPE_ID = TypeID;                           \
    [[nodiscard]] TypeId getTypeId() const override { return TYPE_ID; } \
    [[nodiscard]] bool isTypeOf(TypeId typeId) const override {         \
        return typeId == TYPE_ID || ParentClass::isTypeOf(typeId);      \
    }

class UString;
class Sound;
using SOUNDHANDLE = uint32_t;

class SoundEngine {
    friend class Sound;

   public:
    enum class OutputDriver : uint8_t {
        NONE,
        BASS,         // directsound/wasapi non-exclusive mode/alsa
        BASS_WASAPI,  // exclusive mode
        BASS_ASIO,    // exclusive move
        SOLOUD        // opaque, for now, even though there are multiple possible backends for soloud internally
        // TODO: expose them
    };

   protected:
    struct OUTPUT_DEVICE {
        int id{-1};
        bool enabled{true};
        bool isDefault{false};
        UString name{"Default"};
        OutputDriver driver{OutputDriver::NONE};
    };

   public:
    using TypeId = uint8_t;
    enum SndEngineType : TypeId { BASS, SOLOUD };

    SoundEngine() = default;
    virtual ~SoundEngine() { this->restartCBs = {}; }

    SoundEngine &operator=(const SoundEngine &) = delete;
    SoundEngine &operator=(SoundEngine &&) = delete;

    SoundEngine(const SoundEngine &) = delete;
    SoundEngine(SoundEngine &&) = delete;

    // Factory method to create the appropriate sound engine
    static SoundEngine *createSoundEngine(SndEngineType type = BASS);

    virtual void restart() = 0;
    virtual void shutdown() { ; }
    virtual void update() { ; }

    virtual bool play(Sound *snd, float pan = 0.0f, float pitch = 0.f) = 0;
    virtual void pause(Sound *snd) = 0;
    virtual void stop(Sound *snd) = 0;

    virtual bool isReady() = 0;
    virtual bool hasExclusiveOutput() { return false; }

    virtual void setOutputDevice(const OUTPUT_DEVICE &device) = 0;
    virtual void setVolume(float volume) = 0;

    OUTPUT_DEVICE getDefaultDevice();
    OUTPUT_DEVICE getWantedDevice();
    std::vector<OUTPUT_DEVICE> getOutputDevices();

    virtual void updateOutputDevices(bool printInfo) = 0;
    virtual bool initializeOutputDevice(const OUTPUT_DEVICE &device) = 0;

    virtual void onFreqChanged(float /* oldValue */, float /* newValue */) { ; }
    virtual void onParamChanged(float /* oldValue */, float /* newValue */) { ; }

    using AudioOutputChangedCallback = SA::delegate<void()>;
    inline void setDeviceChangeBeforeCallback(const AudioOutputChangedCallback &callback) {
        this->restartCBs[0] = callback;
    }
    inline void setDeviceChangeAfterCallback(const AudioOutputChangedCallback &callback) {
        this->restartCBs[1] = callback;
    }

    // call this once app init is done, i.e. configs are read, so convar callbacks aren't spuriously fired during init
    virtual void allowInternalCallbacks() { ; }

    [[nodiscard]] inline const UString &getOutputDeviceName() const { return this->currentOutputDevice.name; }
    [[nodiscard]] constexpr auto getOutputDriverType() const { return this->currentOutputDevice.driver; }
    [[nodiscard]] constexpr float getVolume() const { return this->fVolume; }

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
    std::vector<OUTPUT_DEVICE> outputDevices;
    OUTPUT_DEVICE currentOutputDevice;

    float fVolume{1.0f};

    std::array<AudioOutputChangedCallback, 2>
        restartCBs;  // first to exec before restart, second to exec after restart
};
