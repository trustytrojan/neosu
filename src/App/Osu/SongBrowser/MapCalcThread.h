#pragma once
#include <atomic>
#include <mutex>

#include "Database.h"

class DatabaseBeatmap;

struct mct_result {
    DatabaseBeatmap* diff2 = NULL;
    u32 nb_circles = 0;
    u32 nb_sliders = 0;
    u32 nb_spinners = 0;
    f32 star_rating = 0.f;
    u32 min_bpm = 0;
    u32 max_bpm = 0;
    u32 avg_bpm = 0;
};

extern std::atomic<u32> mct_computed;
extern std::atomic<u32> mct_total;

extern std::mutex mct_results_mtx;
extern std::vector<mct_result> mct_results;

void mct_calc(std::vector<BeatmapDifficulty*> maps_to_calc);
void mct_abort();
