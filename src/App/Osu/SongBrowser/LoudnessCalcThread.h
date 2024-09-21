#pragma once
#include <atomic>
#include <mutex>

#include "Database.h"

class DatabaseBeatmap;

u32 loct_computed();
u32 loct_total();
void loct_calc(std::vector<BeatmapDifficulty*> maps_to_calc);
void loct_abort();
