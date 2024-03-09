#pragma once
#include <unordered_map>

#include "BanchoProtocol.h"
#include "UString.h"

struct UserInfo {
    uint32_t user_id = 0;
    bool is_friend = false;

    // Presence (via USER_PRESENCE_REQUEST or USER_PRESENCE_REQUEST_ALL)
    UString name;
    uint8_t utc_offset = 0;
    uint8_t country = 0;
    uint8_t privileges = 0;
    float longitude = 0.f;
    float latitude = 0.f;
    int32_t global_rank = 0;

    // Stats (via USER_STATS_REQUEST)
    Action action = UNKNOWN;
    GameMode mode = STANDARD;
    UString info_text = UString("Loading...");
    UString map_md5;
    int32_t map_id = 0;
    uint32_t mods = 0;
    int64_t total_score = 0;
    int64_t ranked_score = 0;
    int32_t plays = 0;
    uint16_t pp = 0.f;
    float accuracy = 0.f;
};

extern std::unordered_map<uint32_t, UserInfo*> online_users;
UserInfo* get_user_info(uint32_t user_id, bool fetch = false);
