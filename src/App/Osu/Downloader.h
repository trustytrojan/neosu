#pragma once
#include "cbase.h"

void abort_downloads();

// Downloads `url` and stores downloaded file data into `out`
// When file is fully downloaded, `progress` is 1 and `out` is not NULL
// When download fails, `progress` is -1
void download(const char *url, float *progress, std::vector<u8> &out, int *response_code);

// Downloads and extracts given beatmapset
// When download/extraction fails, `progress` is -1
void download_beatmapset(u32 set_id, float *progress);
