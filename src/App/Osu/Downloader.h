#pragma once
#include "BanchoProtocol.h"
#include "cbase.h"

class DatabaseBeatmap;

void abort_downloads();

// Downloads `url` and stores downloaded file data into `out`
// When file is fully downloaded, `progress` is 1 and `out` is not NULL
// When download fails, `progress` is -1
void download(const char *url, float *progress, std::vector<u8> &out, int *response_code);

// Downloads and extracts given beatmapset
// When download/extraction fails, `progress` is -1
void download_beatmapset(u32 set_id, float *progress);

// Downloads given beatmap (unless it already exists)
// When download/extraction fails, `progress` is -1
DatabaseBeatmap *download_beatmap(i32 beatmap_id, MD5Hash beatmap_md5, float *progress);
void process_beatmapset_info_response(Packet packet);
