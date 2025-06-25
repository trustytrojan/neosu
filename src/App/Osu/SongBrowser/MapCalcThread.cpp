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

    for(const auto & diff2 : maps) {
        while(osu->should_pause_background_threads.load() && !dead.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if(dead.load()) return;
        aimStrains.clear();
        speedStrains.clear();

        mct_result result;
        result.diff2 = diff2;

        auto c = DatabaseBeatmap::loadPrimitiveObjects(diff2->sFilePath, dead);
        result.nb_circles = c.numCircles;
        result.nb_sliders = c.numSliders;
        result.nb_spinners = c.numSpinners;

        pp_info info;
        auto diffres = DatabaseBeatmap::loadDifficultyHitObjects(c, diff2->getAR(), diff2->getCS(), 1.f, false, dead);

        DifficultyCalculator::StarCalcParams params;
        params.sortedHitObjects.swap(diffres.diffobjects);
        params.CS = diff2->getCS();
        params.OD = diff2->getOD();
        params.speedMultiplier = 1.f;
        params.relax = false;
        params.touchDevice = false;
        params.aim = &info.aim_stars;
        params.aimSliderFactor = &info.aim_slider_factor;
        params.difficultAimStrains = &info.difficult_aim_strains;
        params.speed = &info.speed_stars;
        params.speedNotes = &info.speed_notes;
        params.difficultSpeedStrains = &info.difficult_speed_strains;
        params.upToObjectIndex = -1;
        params.outAimStrains = &aimStrains;
        params.outSpeedStrains = &speedStrains;
        result.star_rating = DifficultyCalculator::calculateStarDiffForHitObjects(params, dead);

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
