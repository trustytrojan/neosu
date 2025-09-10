#pragma once
// Copyright (c) 2024, kiwec, All rights reserved.

#include "types.h"
#include "MD5Hash.h"

#include <atomic>
#include <unordered_map>

class DatabaseBeatmap;
struct FinishedScore;

extern std::atomic<u32> sct_computed;
extern std::atomic<u32> sct_total;

void sct_calc(std::unordered_map<MD5Hash, std::vector<FinishedScore>> scores_to_maybe_calc);
void sct_abort();
