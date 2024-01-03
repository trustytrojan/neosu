#pragma once
#include "OsuBeatmapStandard.h"
#include "OsuDatabaseBeatmap.h"

enum DownloadStatus {
    DOWNLOADING,
    FAILURE,
    SUCCESS,
};

struct BeatmapDownloadStatus {
    float progress;
    DownloadStatus status;
};

BeatmapDownloadStatus download_mapset(uint32_t set_id);
