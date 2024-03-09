#pragma once
#include <stdint.h>

#include "BanchoProtocol.h"
#include "UString.h"

class Osu;

struct Bancho {
    UString mcosu_version;

    Osu *osu = nullptr;
    UString endpoint;
    int32_t user_id = 0;
    UString username;
    MD5Hash pw_md5;
    Room room;

    bool prefer_daycore = false;

    bool submit_scores = false;
    UString user_agent;
    UString client_hashes;
    UString disk_uuid;
    UString install_id;

    bool is_online() { return user_id > 0; }

    // Room ID can be 0 on private servers! So we check if the room has players instead.
    bool is_in_a_multi_room() { return room.nb_players > 0; }

    bool match_started = false;
    bool is_playing_a_multi_map() { return match_started; }

    Slot last_scores[16];
};

struct Channel {
    UString name;
    UString topic;
    uint32_t nb_members;
};

MD5Hash md5(uint8_t *msg, size_t msg_len);

void handle_packet(Packet *packet);

Packet build_login_packet();

extern Bancho bancho;
extern std::unordered_map<std::string, Channel *> chat_channels;
