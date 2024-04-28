#pragma once
#include "ModFlags.h"
#include "cbase.h"

struct Score;

namespace OsuReplay {
struct Frame {
    long long int cur_music_pos;
    long long int milliseconds_since_last_frame;

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

struct Info {
    uint8_t gamemode;
    uint32_t osu_version;
    UString diff2_md5;
    UString username;
    UString replay_md5;
    int num300s;
    int num100s;
    int num50s;
    int numGekis;
    int numKatus;
    int numMisses;
    int32_t score;
    int comboMax;
    bool perfect;
    int32_t mod_flags;
    UString life_bar_graph;
    int64_t timestamp;
    std::vector<Frame> frames;
};

BEATMAP_VALUES getBeatmapValuesForModsLegacy(int modsLegacy, float legacyAR, float legacyCS, float legacyOD,
                                             float legacyHP);

Info from_bytes(uint8_t* data, int s_data);
std::vector<Frame> get_frames(uint8_t* replay_data, int32_t replay_size);
void compress_frames(const std::vector<Frame>& frames, uint8_t** compressed, size_t* s_compressed);
bool load_from_disk(Score* score);
void load_and_watch(Score score);

}  // namespace OsuReplay
