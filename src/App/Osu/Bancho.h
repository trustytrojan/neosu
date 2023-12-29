#pragma once
#include <stdint.h>
#include "BanchoProtocol.h"
#include "UString.h"

class Osu;

struct Bancho {
  Osu *osu = nullptr;
  uint32_t user_id = 0;
  UString username;

  // Multiplayer rooms
  uint32_t match_id = 0;
  WinCondition win_condition = SCOREV1;
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
