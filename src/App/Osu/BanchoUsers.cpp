#include "BanchoUsers.h"

std::unordered_map<uint32_t, UserInfo*> online_users;

UserInfo* get_user_info(uint32_t user_id, bool fetch) {
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
