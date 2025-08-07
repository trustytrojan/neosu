#pragma once
// Copyright (c) 2023, kiwec, All rights reserved.

#include <unordered_map>

#include "BanchoProtocol.h"
#include "UString.h"

struct UserInfo {
    i32 user_id = 0;
    bool irc_user = false;

    // Presence (via USER_PRESENCE_REQUEST or USER_PRESENCE_REQUEST_ALL)
    bool has_presence = false;
    UString name;
    u8 utc_offset = 0;
    u8 country = 0;
    u8 privileges = 0;
    float longitude = 0.f;
    float latitude = 0.f;
    i32 global_rank = 0;

    // Stats (via USER_STATS_REQUEST)
    u64 stats_tms = 0;
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

namespace BANCHO::User {

extern std::unordered_map<i32, UserInfo*> online_users;
extern std::vector<i32> friends;
extern std::vector<i32> stats_requests;

void logout_user(i32 user_id);
UserInfo* find_user(const UString& username);
UserInfo* find_user_starting_with(UString prefix, const UString& last_match);
UserInfo* try_get_user_info(i32 user_id, bool wants_presence = false);
UserInfo* get_user_info(i32 user_id, bool wants_presence = false);

void request_presence_batch();
}  // namespace BANCHO::User
