#pragma once
// Copyright (c) 2023, kiwec, All rights reserved.

#include "BanchoProtocol.h"
#include "DatabaseBeatmap.h"

namespace BANCHO::Leaderboard {
struct OnlineMapInfo {
    long ranked_status;
    bool server_has_osz2;
    u32 beatmap_id;
    u32 beatmap_set_id;
    i32 online_offset;
    int nb_scores;
};

void process_leaderboard_response(Packet response);

void fetch_online_scores(std::shared_ptr<DatabaseBeatmap> beatmap);
}  // namespace BANCHO::Leaderboard
