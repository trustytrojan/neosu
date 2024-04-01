#pragma once
#include "ModFlags.h"
#include "cbase.h"

namespace OsuReplay {
struct Frame {
    int64_t cur_music_pos;
    int64_t milliseconds_since_last_frame;

    float x;  // 0 - 512
    float y;  // 0 - 384

    uint8_t key_flags;
};

enum KeyFlags {
    M1 = 1,
    M2 = 2,
    K1 = 4,
    K2 = 8,
    Smoke = 16,
};

struct BEATMAP_VALUES {
    float AR;
    float CS;
    float OD;
    float HP;

    float speedMultiplier;

    float difficultyMultiplier;
    float csDifficultyMultiplier;
};

BEATMAP_VALUES getBeatmapValuesForModsLegacy(int modsLegacy, float legacyAR, float legacyCS, float legacyOD,
                                             float legacyHP);

}  // namespace OsuReplay
