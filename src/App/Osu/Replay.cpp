#include "Replay.h"

#include "Beatmap.h"
#include "Osu.h"

i32 Replay::Mods::to_legacy() const {
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
    if(flags & ModFlags::Autoplay) {
        legacy_flags &= ~(LegacyFlags::Relax | LegacyFlags::Autopilot);
        legacy_flags |= LegacyFlags::Autoplay;
    }

    // NOTE: Ignoring nightmare, fposu

    return legacy_flags;
}

Replay::Mods Replay::Mods::from_cvars() {
    Replay::Mods mods;

    if(cv_mod_nofail.getBool()) mods.flags |= Replay::ModFlags::NoFail;
    if(cv_mod_easy.getBool()) mods.flags |= Replay::ModFlags::Easy;
    if(cv_mod_autopilot.getBool()) mods.flags |= Replay::ModFlags::Autopilot;
    if(cv_mod_relax.getBool()) mods.flags |= Replay::ModFlags::Relax;
    if(cv_mod_hidden.getBool()) mods.flags |= Replay::ModFlags::Hidden;
    if(cv_mod_hardrock.getBool()) mods.flags |= Replay::ModFlags::HardRock;
    if(cv_mod_flashlight.getBool()) mods.flags |= Replay::ModFlags::Flashlight;
    if(cv_mod_suddendeath.getBool()) mods.flags |= Replay::ModFlags::SuddenDeath;
    if(cv_mod_perfect.getBool()) mods.flags |= Replay::ModFlags::Perfect;
    if(cv_mod_nightmare.getBool()) mods.flags |= Replay::ModFlags::Nightmare;
    if(cv_nightcore_enjoyer.getBool()) mods.flags |= Replay::ModFlags::NoPitchCorrection;
    if(cv_mod_touchdevice.getBool()) mods.flags |= Replay::ModFlags::TouchDevice;
    if(cv_mod_spunout.getBool()) mods.flags |= Replay::ModFlags::SpunOut;
    if(cv_mod_scorev2.getBool()) mods.flags |= Replay::ModFlags::ScoreV2;
    if(cv_mod_fposu.getBool()) mods.flags |= Replay::ModFlags::FPoSu;
    if(cv_mod_target.getBool()) mods.flags |= Replay::ModFlags::Target;
    if(cv_ar_override_lock.getBool()) mods.flags |= Replay::ModFlags::AROverrideLock;
    if(cv_od_override_lock.getBool()) mods.flags |= Replay::ModFlags::ODOverrideLock;
    if(cv_mod_timewarp.getBool()) mods.flags |= Replay::ModFlags::Timewarp;
    if(cv_mod_artimewarp.getBool()) mods.flags |= Replay::ModFlags::ARTimewarp;
    if(cv_mod_minimize.getBool()) mods.flags |= Replay::ModFlags::Minimize;
    if(cv_mod_jigsaw1.getBool()) mods.flags |= Replay::ModFlags::Jigsaw1;
    if(cv_mod_jigsaw2.getBool()) mods.flags |= Replay::ModFlags::Jigsaw2;
    if(cv_mod_wobble.getBool()) mods.flags |= Replay::ModFlags::Wobble1;
    if(cv_mod_wobble2.getBool()) mods.flags |= Replay::ModFlags::Wobble2;
    if(cv_mod_arwobble.getBool()) mods.flags |= Replay::ModFlags::ARWobble;
    if(cv_mod_fullalternate.getBool()) mods.flags |= Replay::ModFlags::FullAlternate;
    if(cv_mod_shirone.getBool()) mods.flags |= Replay::ModFlags::Shirone;
    if(cv_mod_mafham.getBool()) mods.flags |= Replay::ModFlags::Mafham;
    if(cv_mod_halfwindow.getBool()) mods.flags |= Replay::ModFlags::HalfWindow;
    if(cv_mod_halfwindow_allow_300s.getBool()) mods.flags |= Replay::ModFlags::HalfWindowAllow300s;
    if(cv_mod_ming3012.getBool()) mods.flags |= Replay::ModFlags::Ming3012;
    if(cv_mod_no100s.getBool()) mods.flags |= Replay::ModFlags::No100s;
    if(cv_mod_no50s.getBool()) mods.flags |= Replay::ModFlags::No50s;
    if(cv_mod_autoplay.getBool()) {
        mods.flags &= ~(Replay::ModFlags::Relax | Replay::ModFlags::Autopilot);
        mods.flags |= Replay::ModFlags::Autoplay;
    }

    auto beatmap = osu->getSelectedBeatmap();
    if(beatmap != NULL) {
        mods.speed = beatmap->getSpeedMultiplier();
    }

    mods.notelock_type = cv_notelock_type.getInt();
    mods.autopilot_lenience = cv_autopilot_lenience.getFloat();
    mods.ar_override = cv_ar_override.getFloat();
    mods.ar_overridenegative = cv_ar_overridenegative.getFloat();
    mods.cs_override = cv_cs_override.getFloat();
    mods.cs_overridenegative = cv_cs_overridenegative.getFloat();
    mods.hp_override = cv_hp_override.getFloat();
    mods.od_override = cv_od_override.getFloat();
    mods.timewarp_multiplier = cv_mod_timewarp_multiplier.getFloat();
    mods.minimize_multiplier = cv_mod_minimize_multiplier.getFloat();
    mods.artimewarp_multiplier = cv_mod_artimewarp_multiplier.getFloat();
    mods.arwobble_strength = cv_mod_arwobble_strength.getFloat();
    mods.arwobble_interval = cv_mod_arwobble_interval.getFloat();
    mods.wobble_strength = cv_mod_wobble_strength.getFloat();
    mods.wobble_rotation_speed = cv_mod_wobble_rotation_speed.getFloat();
    mods.jigsaw_followcircle_radius_factor = cv_mod_jigsaw_followcircle_radius_factor.getFloat();
    mods.shirone_combo = cv_mod_shirone_combo.getFloat();

    return mods;
}

Replay::Mods Replay::Mods::from_legacy(i32 legacy_flags) {
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
    if(legacy_flags & LegacyFlags::Autoplay) {
        neoflags &= ~(Replay::ModFlags::Relax | Replay::ModFlags::Autopilot);
        neoflags |= Replay::ModFlags::Autoplay;
    }

    Mods mods;
    mods.flags = neoflags;
    if(legacy_flags & LegacyFlags::DoubleTime)
        mods.speed = 1.5f;
    else if(legacy_flags & LegacyFlags::HalfTime)
        mods.speed = 0.75f;
    return mods;
}
