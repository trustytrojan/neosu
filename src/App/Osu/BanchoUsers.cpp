// Copyright (c) 2023, kiwec, All rights reserved.
#include "BanchoUsers.h"

#include <algorithm>

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "Chat.h"
#include "Engine.h"
#include "NotificationOverlay.h"
#include "Osu.h"
#include "SpectatorScreen.h"

namespace BANCHO::User {
namespace {  // static namespace
std::vector<UserInfo*> presence_requests;

void request_presence(UserInfo* info) {
    if(info->has_presence) return;

    for(auto req : presence_requests) {
        if(req->user_id == info->user_id) return;
    }

    presence_requests.push_back(info);
}
}  // namespace

std::unordered_map<i32, UserInfo*> online_users;
std::vector<i32> friends;
std::vector<i32> stats_requests;

void request_presence_batch() {
    std::vector<i32> actual_requests;
    for(auto req : presence_requests) {
        if(!req->has_presence) {
            actual_requests.push_back(req->user_id);
        }
    }

    presence_requests.clear();
    if(actual_requests.empty()) return;

    Packet packet;
    packet.id = USER_PRESENCE_REQUEST;
    BANCHO::Proto::write<u16>(&packet, actual_requests.size());
    for(auto user_id : actual_requests) {
        BANCHO::Proto::write<i32>(&packet, user_id);
    }
    BANCHO::Net::send_packet(packet);
}

void logout_user(i32 user_id) {
    for(auto it = online_users.begin(); it != online_users.end(); it++) {
        if(it->first == user_id) {
            debugLog("{:s} has disconnected.\n", it->second->name.toUtf8());
            if(it->first == bancho->spectated_player_id) {
                stop_spectating();
            }

            if(it->second->is_friend() && cv::notify_friend_status_change.getBool()) {
                auto text = UString::format("%s is now offline", it->second->name.toUtf8());
                osu->notificationOverlay->addToast(text, CHAT_TOAST);
            }

            delete it->second;
            online_users.erase(it);

            osu->chat->updateUserList();

            return;
        }
    }
}

UserInfo* find_user(const UString& username) {
    for(auto pair : online_users) {
        if(pair.second->name == username) {
            return pair.second;
        }
    }

    return NULL;
}

UserInfo* find_user_starting_with(UString prefix, const UString& last_match) {
    bool matched = last_match.length() == 0;
    for(auto pair : online_users) {
        auto user = pair.second;
        if(!matched) {
            if(user->name == last_match) {
                matched = true;
            }
            continue;
        }
        UString lowerName{user->name};
        lowerName.lowerCase();
        prefix.lowerCase();
        if(lowerName.startsWith(prefix)) {
            return user;
        }
    }

    if(last_match.length() == 0) {
        return NULL;
    } else {
        return find_user_starting_with(prefix, "");
    }
}

UserInfo* try_get_user_info(i32 user_id, bool wants_presence) {
    auto it = online_users.find(user_id);
    if(it != online_users.end()) {
        if(wants_presence) {
            request_presence(it->second);
        }

        return it->second;
    }

    return NULL;
}

UserInfo* get_user_info(i32 user_id, bool wants_presence) {
    auto info = try_get_user_info(user_id, wants_presence);
    if(!info) {
        info = new UserInfo();
        info->user_id = user_id;
        info->name = UString::format("User #%d", user_id);
        online_users[user_id] = info;
        osu->chat->updateUserList();

        if(wants_presence) {
            request_presence(info);
        }
    }

    return info;
}

}  // namespace BANCHO::User

using namespace BANCHO::User;

bool UserInfo::is_friend() {
    auto it = std::ranges::find(friends, this->user_id);
    return it != friends.end();
}
