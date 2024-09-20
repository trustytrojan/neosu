#pragma once
#include "BanchoProtocol.h"
#include "DatabaseBeatmap.h"

struct OnlineMapInfo {
    long ranked_status;
    bool server_has_osz2;
    u32 beatmap_id;
    u32 beatmap_set_id;
    i32 online_offset;
    int nb_scores;
};

void process_leaderboard_response(Packet response);

void fetch_online_scores(DatabaseBeatmap *beatmap);
