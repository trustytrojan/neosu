#include "MapCalcThread.h"

#include <thread>

#include "DatabaseBeatmap.h"

static std::thread thr;
static std::atomic<bool> dead = true;

std::atomic<u32> mct_computed = 0;
std::atomic<u32> mct_total = 0;
std::vector<MapOverrides> mct_results;

static std::vector<BeatmapDifficulty*> maps;

static void run_mct() {
    std::vector<f64> aimStrains;
    std::vector<f64> speedStrains;

    for(auto diff2 : maps) {
        if(dead.load()) return;
        aimStrains.clear();
        speedStrains.clear();

        MapOverrides overrides;

        // NOTE: not thread safe, can crash if loadGameplay() called at the same time
        overrides.map_md5 = diff2->m_sMD5Hash;

        auto c = DatabaseBeatmap::loadPrimitiveObjects(diff2->m_sFilePath, dead);
        overrides.nb_circles = c.numCircles;
        overrides.nb_sliders = c.numSliders;
        overrides.nb_spinners = c.numSpinners;

        pp_info info;
        auto diffres = DatabaseBeatmap::loadDifficultyHitObjects(c, diff2->getAR(), diff2->getCS(), 1.f, false, dead);
        overrides.star_rating = DifficultyCalculator::calculateStarDiffForHitObjects(
            diffres.diffobjects, diff2->getCS(), diff2->getOD(), 1.f, false, false, &info.aim_stars,
            &info.aim_slider_factor, &info.speed_stars, &info.speed_notes, -1, &aimStrains, &speedStrains, dead);

        BPMInfo bpm = getBPM(c.timingpoints);
        overrides.min_bpm = bpm.min;
        overrides.max_bpm = bpm.max;
        overrides.avg_bpm = bpm.most_common;

        mct_results.push_back(overrides);
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
