#pragma once
#include <unordered_map>

#include "BanchoProtocol.h"
#include "UString.h"

struct UserInfo {
    u32 user_id = 0;
    bool irc_user = false;

    bool has_presence = false;
    bool has_stats = false;

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
    MD5Hash map_md5;
    i32 map_id = 0;
    u32 mods = 0;
    i64 total_score = 0;
    i64 ranked_score = 0;
    i32 plays = 0;
    u16 pp = 0.f;
    float accuracy = 0.f;

    // Received when spectating
    LiveReplayBundle::Action spec_action = LiveReplayBundle::Action::NONE;

    bool is_friend();
};

extern std::unordered_map<u32, UserInfo*> online_users;
extern std::vector<u32> friends;
extern std::vector<u32> stats_requests;

void logout_user(u32 user_id);
UserInfo* find_user(UString username);
UserInfo* find_user_starting_with(UString prefix, UString last_match);
UserInfo* try_get_user_info(u32 user_id, bool wants_presence = false);
UserInfo* get_user_info(u32 user_id, bool wants_presence = false);

void request_presence_batch();
