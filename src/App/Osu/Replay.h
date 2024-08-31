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

    static Mods from_legacy(i32 legacy_flags) {
        u64 neoflags = 0;
        if(legacy_flags & LegacyFlags::NoFail) neoflags |= ModFlags::NoFail;
        if(legacy_flags & LegacyFlags::Easy) neoflags |= ModFlags::Easy;
        if(legacy_flags & LegacyFlags::TouchDevice) neoflags |= ModFlags::TouchDevice;
        if(legacy_flags & LegacyFlags::Hidden) neoflags |= ModFlags::Hidden;
        if(legacy_flags & LegacyFlags::HardRock) neoflags |= ModFlags::HardRock;
        if(legacy_flags & LegacyFlags::SuddenDeath) neoflags |= ModFlags::SuddenDeath;
        if(legacy_flags & LegacyFlags::Relax) neoflags |= ModFlags::Relax;
        if(legacy_flags & LegacyFlags::Nightcore) neoflags |= ModFlags::NoPitchCorrection;
        if(legacy_flags & LegacyFlags::Flashlight) neoflags |= ModFlags::Flashlight;
        if(legacy_flags & LegacyFlags::SpunOut) neoflags |= ModFlags::SpunOut;
        if(legacy_flags & LegacyFlags::Autopilot) neoflags |= ModFlags::Autopilot;
        if(legacy_flags & LegacyFlags::Perfect) neoflags |= ModFlags::Perfect;
        if(legacy_flags & LegacyFlags::Target) neoflags |= ModFlags::Target;
        if(legacy_flags & LegacyFlags::ScoreV2) neoflags |= ModFlags::ScoreV2;
        if(legacy_flags & LegacyFlags::Nightmare) neoflags |= ModFlags::Nightmare;
        if(legacy_flags & LegacyFlags::FPoSu) neoflags |= ModFlags::FPoSu;
        if(legacy_flags & LegacyFlags::Mirror) {
            // NOTE: We don't know whether the original score was only horizontal, only vertical, or both
            neoflags |= (ModFlags::MirrorHorizontal | ModFlags::MirrorVertical);
        }

        Mods mods;
        mods.flags = neoflags;
        if(legacy_flags & LegacyFlags::DoubleTime)
            mods.speed = 1.5f;
        else if(legacy_flags & LegacyFlags::HalfTime)
            mods.speed = 0.75f;
        return mods;
    }

    i32 to_legacy() const {
        i32 legacy_flags = 0;
        if(speed > 1.f) {
            legacy_flags |= LegacyFlags::DoubleTime;
            if(flags & ModFlags::NoPitchCorrection) legacy_flags |= LegacyFlags::Nightcore;
        } else if(speed < 1.f) {
            legacy_flags |= LegacyFlags::HalfTime;
        }

        if(flags & ModFlags::NoFail) legacy_flags |= LegacyFlags::NoFail;
        if(flags & ModFlags::Easy) legacy_flags |= LegacyFlags::Easy;
        if(flags & ModFlags::TouchDevice) legacy_flags |= LegacyFlags::TouchDevice;
        if(flags & ModFlags::Hidden) legacy_flags |= LegacyFlags::Hidden;
        if(flags & ModFlags::HardRock) legacy_flags |= LegacyFlags::HardRock;
        if(flags & ModFlags::SuddenDeath) legacy_flags |= LegacyFlags::SuddenDeath;
        if(flags & ModFlags::Relax) legacy_flags |= LegacyFlags::Relax;
        if(flags & ModFlags::Flashlight) legacy_flags |= LegacyFlags::Flashlight;
        if(flags & ModFlags::SpunOut) legacy_flags |= LegacyFlags::SpunOut;
        if(flags & ModFlags::Autopilot) legacy_flags |= LegacyFlags::Autopilot;
        if(flags & ModFlags::Perfect) legacy_flags |= LegacyFlags::Perfect;
        if(flags & ModFlags::Target) legacy_flags |= LegacyFlags::Target;
        if(flags & ModFlags::ScoreV2) legacy_flags |= LegacyFlags::ScoreV2;

        // NOTE: Ignoring nightmare, fposu

        return legacy_flags;
    }
};

}  // namespace Replay
