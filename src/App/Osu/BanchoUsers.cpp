#include "BanchoUsers.h"

std::unordered_map<u32, UserInfo*> online_users;
std::vector<u32> friends;

UserInfo* find_user(UString username) {
    for(auto pair : online_users) {
        if(pair.second->name == username) {
            return pair.second;
        }
    }

    return NULL;
}

UserInfo* find_user_starting_with(UString prefix, UString last_match) {
    bool matched = last_match.length() == 0;
    for(auto pair : online_users) {
        auto user = pair.second;
        if(!matched) {
            if(user->name == last_match) {
                matched = true;
            }
            continue;
        }

        if(user->name.startsWithIgnoreCase(prefix)) {
            return user;
        }
    }

    if(last_match.length() == 0) {
        return NULL;
    } else {
        return find_user_starting_with(prefix, "");
    }
}

UserInfo* get_user_info(u32 user_id, bool fetch) {
    auto it = online_users.find(user_id);
    if(it != online_users.end()) {
        return it->second;
    }

    UserInfo* info = new UserInfo();
    info->user_id = user_id;
    info->name = UString::format("User #%d", user_id);
    online_users[user_id] = info;

    if(fetch) {
        // NOTE: This isn't implemented, since the server already sends presence
        //       data when we need it.

        // Packet packet;
        // packet.id = USER_PRESENCE_REQUEST;
        // ...
    }

    return info;
}

bool UserInfo::is_friend() {
    auto it = std::find(friends.begin(), friends.end(), this->user_id);
    return it != friends.end();
}
