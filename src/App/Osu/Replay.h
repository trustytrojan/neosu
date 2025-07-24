#pragma once
#include "LegacyReplay.h"

class DatabaseBeatmap;

namespace Replay {

namespace ModFlags {

// Green mods
const u64 NoFail = 1ULL << 0;
const u64 Easy = 1ULL << 1;
const u64 Autopilot = 1ULL << 2;
const u64 Relax = 1ULL << 3;

// Red mods
const u64 Hidden = 1ULL << 4;
const u64 HardRock = 1ULL << 5;
const u64 Flashlight = 1ULL << 6;
const u64 SuddenDeath = 1ULL << 7;
const u64 Perfect = SuddenDeath | (1ULL << 8);
const u64 Nightmare = 1ULL << 9;

// Special mods
const u64 NoPitchCorrection = 1ULL << 10;
const u64 TouchDevice = 1ULL << 11;
const u64 SpunOut = 1ULL << 12;
const u64 ScoreV2 = 1ULL << 13;
const u64 FPoSu = 1ULL << 14;
const u64 Target = 1ULL << 15;

// Experimental mods
const u64 AROverrideLock = 1ULL << 16;
const u64 ODOverrideLock = 1ULL << 17;
const u64 Timewarp = 1ULL << 18;
const u64 ARTimewarp = 1ULL << 19;
const u64 Minimize = 1ULL << 20;
const u64 Jigsaw1 = 1ULL << 21;
const u64 Jigsaw2 = 1ULL << 22;
const u64 Wobble1 = 1ULL << 23;
const u64 Wobble2 = 1ULL << 24;
const u64 ARWobble = 1ULL << 25;
const u64 FullAlternate = 1ULL << 26;
const u64 Shirone = 1ULL << 27;
const u64 Mafham = 1ULL << 28;
const u64 HalfWindow = 1ULL << 29;
const u64 HalfWindowAllow300s = 1ULL << 30;
const u64 Ming3012 = 1ULL << 31;
const u64 No100s = 1ULL << 32;
const u64 No50s = 1ULL << 33;
const u64 MirrorHorizontal = 1ULL << 34;
const u64 MirrorVertical = 1ULL << 35;
const u64 FPoSu_Strafing = 1ULL << 36;
const u64 FadingCursor = 1ULL << 37;
const u64 FPS = 1ULL << 38;
const u64 ReverseSliders = 1ULL << 39;
const u64 Millhioref = 1ULL << 40;
const u64 StrictTracking = 1ULL << 41;
const u64 ApproachDifferent = 1ULL << 42;

// Non-submittable
const u64 Autoplay = 1ULL << 63;

}  // namespace ModFlags

struct Mods {
    u64 flags = 0;

    f32 speed = 1.f;
    i32 notelock_type = 2;
    f32 autopilot_lenience = 0.75f;
    f32 ar_override = -1.f;
    f32 ar_overridenegative = 0.f;
    f32 cs_override = -1.f;
    f32 cs_overridenegative = 0.f;
    f32 hp_override = -1.f;
    f32 od_override = -1.f;
    f32 timewarp_multiplier = 1.5f;
    f32 minimize_multiplier = 0.5f;
    f32 artimewarp_multiplier = 0.5f;
    f32 arwobble_strength = 1.0f;
    f32 arwobble_interval = 7.0f;
    f32 wobble_strength = 25.f;
    f32 wobble_frequency = 1.f;
    f32 wobble_rotation_speed = 1.f;
    f32 jigsaw_followcircle_radius_factor = 0.f;
    f32 shirone_combo = 20.f;

    [[nodiscard]] u32 to_legacy() const;

    // Get AR/OD, ignoring mods which change it over time
    // Used for ppv2 calculations.
    f32 get_naive_ar(DatabaseBeatmap *diff2) const;
    f32 get_naive_od(DatabaseBeatmap *diff2) const;

    static Mods from_cvars();
    static Mods from_legacy(u32 legacy_flags);
    static void use(Mods mods);
};

}  // namespace Replay
