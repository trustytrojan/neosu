#include "MapCalcThread.h"

#include <chrono>
#include <thread>

#include "DatabaseBeatmap.h"

static std::thread thr;
static std::atomic<bool> dead = true;

std::atomic<u32> mct_computed = 0;
std::atomic<u32> mct_total = 0;
std::vector<mct_result> mct_results;

static std::vector<BeatmapDifficulty*> maps;

static void run_mct() {
    std::vector<f64> aimStrains;
    std::vector<f64> speedStrains;

    for(auto diff2 : maps) {
        while(osu->should_pause_background_threads.load() && !dead.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if(dead.load()) return;
        aimStrains.clear();
        speedStrains.clear();

        mct_result result;
        result.diff2 = diff2;

        auto c = DatabaseBeatmap::loadPrimitiveObjects(diff2->m_sFilePath, dead);
        result.nb_circles = c.numCircles;
        result.nb_sliders = c.numSliders;
        result.nb_spinners = c.numSpinners;

        pp_info info;
        auto diffres = DatabaseBeatmap::loadDifficultyHitObjects(c, diff2->getAR(), diff2->getCS(), 1.f, false, dead);
        result.star_rating = DifficultyCalculator::calculateStarDiffForHitObjects(
            diffres.diffobjects, diff2->getCS(), diff2->getOD(), 1.f, false, false, &info.aim_stars,
            &info.aim_slider_factor, &info.speed_stars, &info.speed_notes, -1, &aimStrains, &speedStrains, dead);

        BPMInfo bpm = getBPM(c.timingpoints);
        result.min_bpm = bpm.min;
        result.max_bpm = bpm.max;
        result.avg_bpm = bpm.most_common;

        mct_results.push_back(result);
        mct_computed++;
    }

    mct_computed++;
}

void mct_calc(std::vector<BeatmapDifficulty*> maps_to_calc) {
    mct_abort();
    if(maps_to_calc.empty()) return;

    dead = false;
    maps = maps_to_calc;
    mct_computed = 0;
    mct_total = maps.size() + 1;
    mct_results.clear();
    thr = std::thread(run_mct);
}

void mct_abort() {
    if(dead.load()) return;

    dead = true;
    thr.join();

    mct_total = 0;
    mct_computed = 0;
}
