#pragma once
#include "cbase.h"

void abort_downloads();

// Downloads `url` and stores downloaded file data into `out`
// When file is fully downloaded, `progress` is 1 and `out` is not NULL
// When download fails, `progress` is -1
void download(const char *url, float *progress, std::vector<uint8_t> &out);

// Downloads and extracts given beatmapset
// When download/extraction fails, `progress` is -1
void download_beatmapset(uint32_t set_id, float *progress);
