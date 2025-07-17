#pragma once

#include "UString.h"
#include "BassManager.h"

#define SOUND_ENGINE_TYPE(ClassName, TypeID, ParentClass)               \
    static constexpr TypeId TYPE_ID = TypeID;                           \
    [[nodiscard]] TypeId getTypeId() const override { return TYPE_ID; } \
    [[nodiscard]] bool isTypeOf(TypeId typeId) const override {         \
        return typeId == TYPE_ID || ParentClass::isTypeOf(typeId);      \
    }

enum class OutputDriver : uint8_t {
    NONE,
    BASS,         // directsound/wasapi non-exclusive mode/alsa
    BASS_WASAPI,  // exclusive mode
    BASS_ASIO,    // exclusive move
};

struct OUTPUT_DEVICE {
    int id;
    bool enabled;
    bool isDefault;
    UString name;
    OutputDriver driver;
};

class Sound;

class SoundEngine {
    using SOUNDHANDLE = unsigned long;

   public:
    using TypeId = uint8_t;
    enum SndEngineType : TypeId { BASS, SOLOUD };

    SoundEngine();
    ~SoundEngine();
    void restart();
    void shutdown();

    void update();

    bool play(Sound *snd, float pan = 0.0f, float pitch = 0.f);
    void pause(Sound *snd);
    void stop(Sound *snd);

    bool isReady();
    bool isASIO() { return this->currentOutputDevice.driver == OutputDriver::BASS_ASIO; }
    bool isWASAPI() { return this->currentOutputDevice.driver == OutputDriver::BASS_WASAPI; }
    bool hasExclusiveOutput();

    void setOutputDevice(const OUTPUT_DEVICE &device);
    void setVolume(float volume);

    OUTPUT_DEVICE getDefaultDevice();
    OUTPUT_DEVICE getWantedDevice();
    std::vector<OUTPUT_DEVICE> getOutputDevices();

    [[nodiscard]] inline const UString &getOutputDeviceName() const { return this->currentOutputDevice.name; }
    [[nodiscard]] inline float getVolume() const { return this->fVolume; }

    void updateOutputDevices(bool printInfo);
    bool initializeOutputDevice(const OUTPUT_DEVICE &device);
    bool init_bass_mixer(const OUTPUT_DEVICE &device);

    SOUNDHANDLE g_bassOutputMixer = 0;
    void onFreqChanged(float oldValue, float newValue);
    void onParamChanged(float oldValue, float newValue);

    // type inspection
    //[[nodiscard]] virtual TypeId getTypeId() const = 0;
    //[[nodiscard]] virtual bool isTypeOf(TypeId /*type_id*/) const { return false; }
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
    // temp until SoLoud is added
    [[nodiscard]] TypeId getTypeId() const { return BASS; }
    //SOUND_ENGINE_TYPE(SoundEngine, BASS, SoundEngine)
   private:
    std::vector<OUTPUT_DEVICE> outputDevices;

    OUTPUT_DEVICE currentOutputDevice;

    double ready_since = -1.0;
    float fVolume = 1.0f;
};

#ifdef MCENGINE_PLATFORM_WINDOWS
DWORD ASIO_clamp(BASS_ASIO_INFO info, DWORD buflen);
#endif
