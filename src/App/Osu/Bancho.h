#pragma once
#include <stdint.h>

#include "BanchoProtocol.h"
#include "Osu.h"


enum WinCondition {
    Score = 0,
    Accuracy = 1,
};

struct Bancho {
    Osu* osu = nullptr;
    uint32_t user_id = 0;

    // Multiplayer rooms
    uint32_t match_id = 0;
    WinCondition win_condition = Score;
};

// Returns 0 if there was an error while sending the packet
int send_packet(Packet *packet);

Packet build_login_packet(char *username, char *password);

extern Bancho bancho;
