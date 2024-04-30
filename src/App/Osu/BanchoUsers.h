#pragma once
#include <unordered_map>

#include "BanchoProtocol.h"
#include "UString.h"

struct UserInfo {
    u32 user_id = 0;

    // Presence (via USER_PRESENCE_REQUEST or USER_PRESENCE_REQUEST_ALL)
    UString name;
    u8 utc_offset = 0;
    u8 country = 0;
    u8 privileges = 0;
    float longitude = 0.f;
    float latitude = 0.f;
    i32 global_rank = 0;

    // Stats (via USER_STATS_REQUEST)
    Action action = UNKNOWN;
    GameMode mode = STANDARD;
    UString info_text = UString("Loading...");
    UString map_md5;
    i32 map_id = 0;
    u32 mods = 0;
    i64 total_score = 0;
    i64 ranked_score = 0;
    i32 plays = 0;
    u16 pp = 0.f;
    float accuracy = 0.f;

    bool is_friend();
};

extern std::unordered_map<u32, UserInfo*> online_users;
extern std::vector<u32> friends;

UserInfo* get_user_info(u32 user_id, bool fetch = false);
