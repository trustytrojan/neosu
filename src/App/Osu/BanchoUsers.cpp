#include "BanchoUsers.h"

std::unordered_map<u32, UserInfo*> online_users;
std::vector<u32> friends;

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
    auto it = std::find(friends.begin(), friends.end(), user_id);
    return it != friends.end();
}
