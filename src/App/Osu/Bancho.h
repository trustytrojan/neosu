#pragma once
#include <stdint.h>
#include "BanchoProtocol.h"
#include "UString.h"

class Osu;

struct Bancho {
  Osu *osu = nullptr;
  UString endpoint;
  int32_t user_id = 0;
  UString username;
  Room room;

  // TODO @kiwec: implement score submission
  bool submit_scores = false;

  bool is_online() { return user_id > 0; }
  bool is_in_a_multi_room() { return room.id > 0; }
  bool is_playing_a_multi_map() {
    if(!is_in_a_multi_room()) return false;
    for(int i = 0; i < 16; i++) {
      if(room.slots[i].player_id == user_id) {
        return room.slots[i].is_player_playing();
      }
    }
    return false;
  }
};

struct Channel {
  UString name;
  UString topic;
  uint32_t nb_members;
};

std::string md5(uint8_t *msg, size_t msg_len);

void handle_packet(Packet *packet);

Packet build_login_packet(char *username, char *password);

extern Bancho bancho;
extern std::unordered_map<std::string, Channel*> chat_channels;
