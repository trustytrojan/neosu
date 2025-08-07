#pragma once
// Copyright (c) 2024, kiwec, All rights reserved.

#include <atomic>
#include <mutex>

#include "Database.h"

class DatabaseBeatmap;

extern std::atomic<u32> sct_computed;
extern std::atomic<u32> sct_total;

void sct_calc(const std::vector<FinishedScore>& scores_to_calc);
void sct_abort();
