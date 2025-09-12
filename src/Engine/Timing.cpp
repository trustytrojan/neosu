// Copyright (c) 2025, WH, All rights reserved.
#include "Timing.h"

#ifdef MCENGINE_PLATFORM_WINDOWS
#include "WinDebloatDefs.h"
#include <libloaderapi.h>
#include <winnt.h>
#include <profileapi.h>
#include "dynutils.h"
#endif

namespace Timing::detail {
#ifdef MCENGINE_PLATFORM_WINDOWS
namespace {  // static namespace
class SleepHandler final {
    NOCOPY_NOMOVE(SleepHandler)
   public:
    SleepHandler() {
        // initialize performance counter frequency
        QueryPerformanceFrequency(&m_perfFreq);

        auto *ntdll_handle{reinterpret_cast<dynutils::lib_obj *>(GetModuleHandle(TEXT("ntdll.dll")))};
        if(ntdll_handle) {
            m_pNtDelayExec = dynutils::load_func<NtDelayExecution_t>(ntdll_handle, "NtDelayExecution");
            m_pNtQueryTimerRes = dynutils::load_func<NtQueryTimerResolution_t>(ntdll_handle, "NtQueryTimerResolution");
            m_pNtSetTimerRes = dynutils::load_func<NtSetTimerResolution_t>(ntdll_handle, "NtSetTimerResolution");
        }

        if(!!m_pNtDelayExec && !!m_pNtQueryTimerRes && !!m_pNtSetTimerRes) {
            ULONG maxRes, minRes, currentRes;
            if(m_pNtQueryTimerRes(&maxRes, &minRes, &currentRes) >= 0) {
                m_originalRes = currentRes;

                ULONG actualRes;
                if(m_pNtSetTimerRes(minRes, TRUE, &actualRes) >= 0 && actualRes <= 10000) {
                    // only enable if we achieved <= 1ms resolution (10000 * 100ns units)
                    m_bUseNTDelayExec = true;
                    m_actualDelayAmount = actualRes * 100ULL;
                    measureActualDelay(); // get accurate NtDelayExecution time
                }
            }
        }
    }

    ~SleepHandler() {
        // clear timer res we set
        if(m_originalRes != 0 && !!m_pNtSetTimerRes) {
            ULONG actualRes;
            m_pNtSetTimerRes(m_originalRes, TRUE, &actualRes);
        }
    }

    forceinline void imprecise(uint64_t ns) {
        if(!m_bUseNTDelayExec) {
            SDL_DelayNS(ns);
            return;
        }

        m_pNtDelayExec(0, nsToDelayInterval(ns));
    }

    forceinline void precise(uint64_t ns) {
        if(!m_bUseNTDelayExec) {
            SDL_DelayPrecise(ns);
            return;
        }

        // use NtDelayExecution for bulk delay, busy-wait for remainder
        uint64_t targetTime = getHighResTime() + ns;
        if(ns > m_actualDelayAmount) {
            uint64_t bulkDelay = ns - m_actualDelayAmount;
            m_pNtDelayExec(0, nsToDelayInterval(bulkDelay));
        }

        // busy-wait for remaining time
        while(getHighResTime() < targetTime) {
            YieldProcessor();
        }
    }

   private:
    void measureActualDelay() {
        // measure the minimum time NtDelayExecution actually sleeps for
        constexpr int numSamples = 3; // 3 samples per delay amount
        constexpr const auto testDelays = std::array{50000ULL, 100000ULL, 250000ULL, 500000ULL};  // 0.05ms to 0.25ms

        uint64_t minActualSleep = UINT64_MAX;

        for(const auto testDelayNs : testDelays) {
            uint64_t totalActual = 0;
            int validSamples = 0;

            for(int i = 0; i < numSamples; ++i) {
                uint64_t startTime = getHighResTime();
                m_pNtDelayExec(0, nsToDelayInterval(testDelayNs));
                uint64_t endTime = getHighResTime();

                uint64_t actualDelay = endTime - startTime;
                if(actualDelay >= testDelayNs / 2 && actualDelay < testDelayNs * 4) {  // sanity check
                    totalActual += actualDelay;
                    validSamples++;
                }
            }

            if(validSamples > 0) {
                uint64_t avgActual = totalActual / validSamples;
                if(avgActual < minActualSleep) {
                    minActualSleep = avgActual;
                }
            }
        }

        if(minActualSleep != UINT64_MAX) {
            m_actualDelayAmount = minActualSleep;
            // cap at 2ms
            if(m_actualDelayAmount > 2000000) {
                m_actualDelayAmount = 2000000;
            }
        }
        // else if we failed, m_actualDelayAmount will == the timer resolution
    }

    forceinline uint64_t getHighResTime() {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        // convert to nanoseconds
        return static_cast<uint64_t>((counter.QuadPart * 1000000000ULL) / m_perfFreq.QuadPart);
    }

    static PLARGE_INTEGER nsToDelayInterval(uint64_t ns) {
        static int64_t sleep_ticks{0};
        sleep_ticks = static_cast<int64_t>(ns);
        sleep_ticks /= 100;
        sleep_ticks *= -1;
        return reinterpret_cast<PLARGE_INTEGER>(&sleep_ticks);
    }

    using NtDelayExecution_t = LONG NTAPI(BOOLEAN Alertable, PLARGE_INTEGER DelayInterval);
    using NtQueryTimerResolution_t = LONG NTAPI(PULONG MaximumTime, PULONG MinimumTime, PULONG CurrentTime);
    using NtSetTimerResolution_t = LONG NTAPI(ULONG DesiredTime, BOOLEAN SetResolution, PULONG ActualTime);

    NtDelayExecution_t *m_pNtDelayExec{nullptr};
    NtQueryTimerResolution_t *m_pNtQueryTimerRes{nullptr};
    NtSetTimerResolution_t *m_pNtSetTimerRes{nullptr};

    LARGE_INTEGER m_perfFreq{};
    uint64_t m_actualDelayAmount{0};  // minimum time NtDelayExecution actually sleeps for in nanoseconds
    uint64_t m_originalRes{0};
    bool m_bUseNTDelayExec{false};
};

// so we can run ctor/dtors on program init/shutdown
static SleepHandler sleeper{};

}  // namespace

void sleep_ns_internal(uint64_t ns) noexcept { sleeper.imprecise(ns); }
void sleep_ns_precise_internal(uint64_t ns) noexcept { sleeper.precise(ns); }

#else  // SDL (generic)
void sleep_ns_internal(uint64_t ns) noexcept { SDL_DelayNS(ns); }
void sleep_ns_precise_internal(uint64_t ns) noexcept { SDL_DelayPrecise(ns); }
#endif
}  // namespace Timing::detail
