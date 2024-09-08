#pragma once
#include "LegacyReplay.h"

namespace Replay {

enum ModFlags {
    // Green mods
    NoFail = 1 << 0,
    Easy = 1 << 1,
    Autopilot = 1 << 2,
    Relax = 1 << 3,

    // Red mods
    Hidden = 1 << 4,
    HardRock = 1 << 5,
    Flashlight = 1 << 6,
    SuddenDeath = 1 << 7,
    Perfect = SuddenDeath | (1 << 8),
    Nightmare = 1 << 9,

    // Special mods
    NoPitchCorrection = 1 << 10,
    TouchDevice = 1 << 11,
    SpunOut = 1 << 12,
    ScoreV2 = 1 << 13,
    FPoSu = 1 << 14,
    Target = 1 << 15,

    // Experimental mods
    AROverrideLock = 1 << 16,
    ODOverrideLock = 1 << 17,
    Timewarp = 1 << 18,
    ARTimewarp = 1 << 19,
    Minimize = 1 << 20,
    Jigsaw1 = 1 << 21,
    Jigsaw2 = 1 << 22,
    Wobble1 = 1 << 23,
    Wobble2 = 1 << 24,
    ARWobble = 1 << 25,
    FullAlternate = 1 << 26,
    Shirone = 1 << 27,
    Mafham = 1 << 28,
    HalfWindow = 1 << 29,
    HalfWindowAllow300s = 1 << 30,
    Ming3012 = 1 << 31,
    No100s = 1 << 32,
    No50s = 1 << 33,
    MirrorHorizontal = 1 << 34,
    MirrorVertical = 1 << 35,
    FPoSu_Strafing = 1 << 36,
    FadingCursor = 1 << 37,
    FPS = 1 << 38,
    ReverseSliders = 1 << 39,
    Millhioref = 1 << 40,
    StrictTracking = 1 << 41,
    ApproachDifferent = 1 << 42,

    // Non-submittable
    Autoplay = 1 << 63,
};

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

    i32 to_legacy() const;

    static Mods from_cvars();
    static Mods from_legacy(i32 legacy_flags);
};

}  // namespace Replay
