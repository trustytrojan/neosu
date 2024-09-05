#include "LoudnessCalcThread.h"

#include "DatabaseBeatmap.h"
#include "Engine.h"
#include "Sound.h"
#include "SoundEngine.h"

std::atomic<u32> loct_computed = 0;
std::atomic<u32> loct_total = 0;

static std::thread thr;
static std::atomic<bool> dead = true;
static std::vector<BeatmapDifficulty*> maps;

static void run_loct() {
    debugLog("Started loudness calculation thread\n");
    std::string last_song = "";
    f32 last_loudness = 0.f;
    f32 buf[44100];

    BASS_SetDevice(0);
    BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 0);

    // XXX: This seems really slow. Need to find out how to get BASS to do more work.

    for(auto diff2 : maps) {
        if(dead.load()) return;
        if(diff2->loudness.load() != 0.f) continue;

        auto song = diff2->getFullSoundFilePath();
        if(song == last_song) {
            diff2->loudness = last_loudness;
            loct_computed++;
            continue;
        }

        auto decoder = BASS_StreamCreateFile(false, song.c_str(), 0, 0, BASS_STREAM_DECODE | BASS_SAMPLE_MONO);
        if(!decoder) {
            debugLog("BASS_StreamCreateFile() returned error %d on file %s\n", BASS_ErrorGetCode(), song.c_str());
            loct_computed++;
            continue;
        }

        auto loudness = BASS_Loudness_Start(decoder, BASS_LOUDNESS_INTEGRATED, 0);
        if(!loudness) {
            debugLog("BASS_Loudness_Start() returned error %d\n", BASS_ErrorGetCode());
            BASS_ChannelFree(decoder);
            loct_computed++;
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

        loct_computed++;
    }

    loct_computed++;
}

void loct_calc(std::vector<BeatmapDifficulty*> maps_to_calc) {
    loct_abort();
    if(maps_to_calc.empty()) return;
    if(!cv_normalize_loudness.getBool()) return;

    dead = false;
    maps = maps_to_calc;
    loct_computed = 0;
    loct_total = maps.size() + 1;
    thr = std::thread(run_loct);
}

void loct_abort() {
    if(dead.load()) return;

    dead = true;
    thr.join();

    loct_total = 0;
    loct_computed = 0;
}
