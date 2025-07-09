#include "BanchoUsers.h"

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "Chat.h"
#include "Engine.h"
#include "Osu.h"
#include "SpectatorScreen.h"

std::unordered_map<u32, UserInfo*> online_users;
std::vector<u32> friends;

std::vector<UserInfo*> presence_requests;
std::vector<u32> stats_requests;

void request_presence(UserInfo* info) {
    if(info->has_presence) return;

    for(auto req : presence_requests) {
        if(req->user_id == info->user_id) return;
    }

    presence_requests.push_back(info);
}

void request_presence_batch() {
    std::vector<u32> actual_requests;
    for(auto req : presence_requests) {
        if(!req->has_presence) {
            actual_requests.push_back(req->user_id);
        }
    }

    presence_requests.clear();
    if(actual_requests.empty()) return;

    Packet packet;
    packet.id = USER_PRESENCE_REQUEST;
    write<u16>(&packet, actual_requests.size());
    for(auto user_id : actual_requests) {
        write<u32>(&packet, user_id);
    }
    send_packet(packet);
}

void logout_user(u32 user_id) {
    for(auto it = online_users.begin(); it != online_users.end(); it++) {
        if(it->first == user_id) {
            debugLog("%s has disconnected.\n", it->second->name.toUtf8());
            if(it->first == bancho.spectated_player_id) {
                stop_spectating();
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

UserInfo* try_get_user_info(u32 user_id, bool wants_presence) {
    auto it = online_users.find(user_id);
    if(it != online_users.end()) {
        if(wants_presence) {
            request_presence(it->second);
        }

        return it->second;
    }

    return NULL;
}

UserInfo* get_user_info(u32 user_id, bool wants_presence) {
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

bool UserInfo::is_friend() {
    auto it = std::find(friends.begin(), friends.end(), this->user_id);
    return it != friends.end();
}
