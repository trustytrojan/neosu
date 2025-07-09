#include "LoudnessCalcThread.h"

#include <chrono>
#include <utility>

#include "DatabaseBeatmap.h"
#include "Engine.h"
#include "Sound.h"
#include "SoundEngine.h"

struct LoudnessCalcThread {
    std::thread thr;
    std::atomic<bool> dead = true;
    std::vector<BeatmapDifficulty*> maps;

    std::atomic<u32> nb_computed = 0;
    std::atomic<u32> nb_total = 0;

   public:
    LoudnessCalcThread(std::vector<BeatmapDifficulty*> maps_to_calc) {
        this->dead = false;
        this->maps = std::move(maps_to_calc);
        this->nb_total = this->maps.size() + 1;
        this->thr = std::thread(&LoudnessCalcThread::run, this);
    }

    ~LoudnessCalcThread() {
        this->dead = true;
        this->thr.join();
    }

   private:
    void run() {
        std::string last_song = "";
        f32 last_loudness = 0.f;
        f32 buf[44100];

        BASS_SetDevice(0);
        BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 0);

        for(auto diff2 : this->maps) {
            while(osu->should_pause_background_threads.load() && !this->dead.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if(this->dead.load()) return;
            if(diff2->loudness.load() != 0.f) continue;

            auto song = diff2->getFullSoundFilePath();
            if(song == last_song) {
                diff2->loudness = last_loudness;
                this->nb_computed++;
                continue;
            }

            auto decoder = BASS_StreamCreateFile(false, song.c_str(), 0, 0, BASS_STREAM_DECODE | BASS_SAMPLE_MONO);
            if(!decoder) {
                auto err_str = BassManager::getErrorUString();
                debugLog("BASS_StreamCreateFile(%s): %s\n", song.c_str(), err_str.toUtf8());
                this->nb_computed++;
                continue;
            }

            auto loudness = BASS_Loudness_Start(decoder, BASS_LOUDNESS_INTEGRATED, 0);
            if(!loudness) {
                auto err_str = BassManager::getErrorUString();
                debugLog("BASS_Loudness_Start(): %s\n", err_str.toUtf8());
                BASS_ChannelFree(decoder);
                this->nb_computed++;
                continue;
            }

            // Did you know?
            // If we do while(BASS_ChannelGetData(decoder, buf, sizeof(buf) >= 0), we get an infinite loop!
            // Thanks, Microsoft!
            int c;
            do {
                c = BASS_ChannelGetData(decoder, buf, sizeof(buf));
            } while(c >= 0);

            BASS_ChannelFree(decoder);

            f32 integrated_loudness = 0.f;
            BASS_Loudness_GetLevel(loudness, BASS_LOUDNESS_INTEGRATED, &integrated_loudness);
            BASS_Loudness_Stop(loudness);
            if(integrated_loudness == -HUGE_VAL) {
                debugLog("No loudness information available for '%s' (silent song?)\n", song.c_str());
            } else {
                diff2->loudness = integrated_loudness;
                diff2->update_overrides();
                last_loudness = integrated_loudness;
                last_song = song;
            }

            this->nb_computed++;
        }

        this->nb_computed++;
    }
};

static std::vector<LoudnessCalcThread*> threads;

u32 loct_computed() {
    u32 x = 0;
    for(auto thr : threads) {
        x += thr->nb_computed.load();
    }
    return x;
}

u32 loct_total() {
    u32 x = 0;
    for(auto thr : threads) {
        x += thr->nb_total.load();
    }
    return x;
}

void loct_calc(std::vector<BeatmapDifficulty*> maps_to_calc) {
    loct_abort();
    if(maps_to_calc.empty()) return;
    if(!cv_normalize_loudness.getBool()) return;

    i32 nb_threads = cv_loudness_calc_threads.getInt();
    if(nb_threads <= 0) {
        // dividing by 2 still burns cpu if hyperthreading is enabled, let's keep it at a sane amount of threads
        nb_threads = std::max(std::thread::hardware_concurrency() / 4, 1u);
    }
    if(maps_to_calc.size() < nb_threads) nb_threads = maps_to_calc.size();
    int chunk_size = maps_to_calc.size() / nb_threads;
    int remainder = maps_to_calc.size() % nb_threads;

    auto it = maps_to_calc.begin();
    for(int i = 0; i < nb_threads; i++) {
        int cur_chunk_size = chunk_size + (i < remainder ? 1 : 0);

        auto chunk = std::vector<BeatmapDifficulty*>(it, it + cur_chunk_size);
        it += cur_chunk_size;

        auto lct = new LoudnessCalcThread(chunk);
        threads.push_back(lct);
    }
}

void loct_abort() {
    for(auto thr : threads) {
        delete thr;
    }
    threads.clear();
}
