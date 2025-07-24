#pragma once

#include "SoundEngine.h"
#ifdef MCENGINE_FEATURE_BASS

#include "BassSound.h"

class Sound;

class BassSoundEngine final : public SoundEngine {
    friend class BassSound;

   public:
    BassSoundEngine();
    ~BassSoundEngine() override;

    BassSoundEngine &operator=(const BassSoundEngine &) = delete;
    BassSoundEngine &operator=(BassSoundEngine &&) = delete;

    BassSoundEngine(const BassSoundEngine &) = delete;
    BassSoundEngine(BassSoundEngine &&) = delete;

    void restart() override;
    void shutdown() override;

    bool play(Sound *snd, float pan = 0.0f, float pitch = 0.f) override;
    void pause(Sound *snd) override;
    void stop(Sound *snd) override;

    bool isReady() override;
    bool hasExclusiveOutput() override;

    void setOutputDevice(const OUTPUT_DEVICE &device) override;
    void setVolume(float volume) override;

    void updateOutputDevices(bool printInfo) override;
    bool initializeOutputDevice(const OUTPUT_DEVICE &device) override;

    void onFreqChanged(float oldValue, float newValue) override;
    void onParamChanged(float oldValue, float newValue) override;

    SOUND_ENGINE_TYPE(BassSoundEngine, BASS, SoundEngine)
   private:
    bool isASIO() { return this->currentOutputDevice.driver == OutputDriver::BASS_ASIO; }
    bool isWASAPI() { return this->currentOutputDevice.driver == OutputDriver::BASS_WASAPI; }
    bool init_bass_mixer(const OUTPUT_DEVICE &device);

    void shutdown_lite(bool force = false); // don't free "No Sound" for changing output device

    void free_if_init(OUTPUT_DEVICE &device);

    OUTPUT_DEVICE no_sound_device{
        .id = 0,
        .isInit = false,
        .enabled = true,
        .isDefault = true,
        .name = "No sound",
        .driver = OutputDriver::NONE,
    };

    double ready_since{-1.0};
    SOUNDHANDLE g_bassOutputMixer = 0;
};

#ifdef MCENGINE_PLATFORM_WINDOWS
DWORD ASIO_clamp(BASS_ASIO_INFO info, DWORD buflen);
#endif

// convenience conversion macro to get the sound handle, extra args are any extra conditions to check for besides general state validity
// just minor boilerplate reduction
#define GETHANDLE(T, accessor, ...) \
	[&]() -> std::pair<T *, SOUNDHANDLE> { \
		SOUNDHANDLE retHandle = 0; \
		T *retSound = nullptr; \
		if (this->isReady() && snd && snd->isReady() __VA_OPT__(&&(__VA_ARGS__)) && (retSound = snd->as<T>())) \
			retHandle = retSound->accessor; \
		return {retSound, retHandle}; \
	}()

#else
class BassSoundEngine : public SoundEngine {};
#endif
