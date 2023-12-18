#pragma once
#include <stdint.h>

enum WinCondition {
    Score = 0,
    Accuracy = 1,
};

struct Bancho {
    bool is_connected = false;

    uint32_t user_id = 0;

    // Multiplayer rooms
    uint32_t match_id = 0;
    WinCondition win_condition = Score;
};

extern Bancho bancho;
