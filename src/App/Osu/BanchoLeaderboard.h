#pragma once
#include "BanchoProtocol.h"
#include "OsuDatabaseBeatmap.h"

struct OnlineMapInfo {
  long ranked_status;
  bool server_has_osz2;
  uint32_t beatmap_id;
  uint32_t beatmap_set_id;
  int nb_scores;
};

void process_leaderboard_response(Packet response);

void fetch_online_scores(OsuDatabaseBeatmap *beatmap);
