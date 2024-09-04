#pragma once
#include <atomic>
#include <mutex>

#include "Database.h"

extern std::atomic<u32> mct_computed;
extern std::atomic<u32> mct_total;

extern std::mutex mct_results_mtx;
extern std::vector<MapOverrides> mct_results;

void mct_calc(std::vector<BeatmapDifficulty*> maps_to_calc);
void mct_abort();
