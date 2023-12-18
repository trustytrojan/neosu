#pragma once
#include <stdint.h>

struct Bancho {
    bool is_connected = false;

    uint32_t user_id = 0;
};

extern Bancho bancho;
