#include "MapCalcThread.h"

#include <chrono>
#include <thread>
#include <utility>

#include "DatabaseBeatmap.h"
#include "DifficultyCalculator.h"
#include "Osu.h"

// static member definitions
std::unique_ptr<MapCalcThread> MapCalcThread::instance = nullptr;
std::once_flag MapCalcThread::instance_flag;

void MapCalcThread::start_calc_instance(const std::vector<BeatmapDifficulty*>& maps_to_calc) {
    abort();

    if(maps_to_calc.empty()) {
        return;
    }

    this->should_stop = false;
    MapCalcThread::maps_to_process = &maps_to_calc;
    this->computed_count = 0;
    this->total_count = static_cast<u32>(maps_to_calc.size()) + 1;
    this->results.clear();

    this->worker_thread = std::thread(&MapCalcThread::run, this);
}

void MapCalcThread::abort_instance() {
    if(this->should_stop.load()) {
        return;
    }

    this->should_stop = true;

    if(this->worker_thread.joinable()) {
        this->worker_thread.join();
    }

    this->total_count = 0;
    this->computed_count = 0;
}

void MapCalcThread::run() {
    std::vector<f64> aimStrains;
    std::vector<f64> speedStrains;

    for(int i = 0; MapCalcThread::maps_to_process && std::cmp_less(i, MapCalcThread::maps_to_process->size()); i++) {
        // pause handling
        while(osu->should_pause_background_threads.load() && !this->should_stop.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if(this->should_stop.load() || !MapCalcThread::maps_to_process) {
            return;
        }

        aimStrains.clear();
        speedStrains.clear();

        const auto& diff2 = (*MapCalcThread::maps_to_process)[i];

        mct_result result;
        result.diff2 = diff2;

        auto c = DatabaseBeatmap::loadPrimitiveObjects(diff2->sFilePath, this->should_stop);
        result.nb_circles = c.numCircles;
        result.nb_sliders = c.numSliders;
        result.nb_spinners = c.numSpinners;

        pp_info info;
        auto diffres =
            DatabaseBeatmap::loadDifficultyHitObjects(c, diff2->getAR(), diff2->getCS(), 1.f, false, this->should_stop);

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
        result.star_rating =
            static_cast<f32>(DifficultyCalculator::calculateStarDiffForHitObjects(params, this->should_stop));

        BPMInfo bpm = getBPM(c.timingpoints);
        result.min_bpm = static_cast<u32>(bpm.min);
        result.max_bpm = static_cast<u32>(bpm.max);
        result.avg_bpm = static_cast<u32>(bpm.most_common);

        this->results.push_back(result);
        this->computed_count++;
    }

    this->computed_count++;
}

MapCalcThread& MapCalcThread::get_instance() {
    std::call_once(instance_flag, []() { instance = std::make_unique<MapCalcThread>(); });
    return *instance;
}
