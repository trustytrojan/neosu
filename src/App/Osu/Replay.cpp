#include "Replay.h"

#include "Beatmap.h"
#include "CBaseUICheckbox.h"
#include "CBaseUISlider.h"
#include "DatabaseBeatmap.h"
#include "GameRules.h"
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

f32 Replay::Mods::get_naive_ar(DatabaseBeatmap *diff2) const {
    float ARdifficultyMultiplier = 1.0f;
    if((flags & Replay::ModFlags::HardRock)) ARdifficultyMultiplier = 1.4f;
    if((flags & Replay::ModFlags::Easy)) ARdifficultyMultiplier = 0.5f;

    f32 AR = clamp<f32>(diff2->getAR() * ARdifficultyMultiplier, 0.0f, 10.0f);
    if(ar_override >= 0.0f) AR = ar_override;
    if(ar_overridenegative < 0.0f) AR = ar_overridenegative;

    if(flags & Replay::ModFlags::AROverrideLock) {
        AR = GameRules::getRawConstantApproachRateForSpeedMultiplier(GameRules::getRawApproachTime(AR), speed);
    }

    return AR;
}

f32 Replay::Mods::get_naive_od(DatabaseBeatmap *diff2) const {
    float ODdifficultyMultiplier = 1.0f;
    if((flags & Replay::ModFlags::HardRock)) ODdifficultyMultiplier = 1.4f;
    if((flags & Replay::ModFlags::Easy)) ODdifficultyMultiplier = 0.5f;

    f32 OD = clamp<f32>(diff2->getOD() * ODdifficultyMultiplier, 0.0f, 10.0f);
    if(od_override >= 0.0f) OD = od_override;

    if(flags & Replay::ModFlags::ODOverrideLock) {
        OD = GameRules::getRawConstantOverallDifficultyForSpeedMultiplier(GameRules::getRawHitWindow300(OD), speed);
    }

    return OD;
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

void Replay::Mods::use(Mods mods) {
    // Reset mod selector buttons and sliders
    auto mod_selector = osu->getModSelector();
    mod_selector->resetMods();

    // Set cvars
    cv_mod_nofail.setValue(mods.flags & Replay::ModFlags::NoFail);
    cv_mod_easy.setValue(mods.flags & Replay::ModFlags::Easy);
    cv_mod_hidden.setValue(mods.flags & Replay::ModFlags::Hidden);
    cv_mod_hardrock.setValue(mods.flags & Replay::ModFlags::HardRock);
    cv_mod_flashlight.setValue(mods.flags & Replay::ModFlags::Flashlight);
    cv_mod_suddendeath.setValue(mods.flags & Replay::ModFlags::SuddenDeath);
    cv_mod_perfect.setValue(mods.flags & Replay::ModFlags::Perfect);
    cv_mod_nightmare.setValue(mods.flags & Replay::ModFlags::Nightmare);
    cv_nightcore_enjoyer.setValue(mods.flags & Replay::ModFlags::NoPitchCorrection);
    cv_mod_touchdevice.setValue(mods.flags & Replay::ModFlags::TouchDevice);
    cv_mod_spunout.setValue(mods.flags & Replay::ModFlags::SpunOut);
    cv_mod_scorev2.setValue(mods.flags & Replay::ModFlags::ScoreV2);
    cv_mod_fposu.setValue(mods.flags & Replay::ModFlags::FPoSu);
    cv_mod_target.setValue(mods.flags & Replay::ModFlags::Target);
    cv_ar_override_lock.setValue(mods.flags & Replay::ModFlags::AROverrideLock);
    cv_od_override_lock.setValue(mods.flags & Replay::ModFlags::ODOverrideLock);
    cv_mod_timewarp.setValue(mods.flags & Replay::ModFlags::Timewarp);
    cv_mod_artimewarp.setValue(mods.flags & Replay::ModFlags::ARTimewarp);
    cv_mod_minimize.setValue(mods.flags & Replay::ModFlags::Minimize);
    cv_mod_jigsaw1.setValue(mods.flags & Replay::ModFlags::Jigsaw1);
    cv_mod_jigsaw2.setValue(mods.flags & Replay::ModFlags::Jigsaw2);
    cv_mod_wobble.setValue(mods.flags & Replay::ModFlags::Wobble1);
    cv_mod_wobble2.setValue(mods.flags & Replay::ModFlags::Wobble2);
    cv_mod_arwobble.setValue(mods.flags & Replay::ModFlags::ARWobble);
    cv_mod_fullalternate.setValue(mods.flags & Replay::ModFlags::FullAlternate);
    cv_mod_shirone.setValue(mods.flags & Replay::ModFlags::Shirone);
    cv_mod_mafham.setValue(mods.flags & Replay::ModFlags::Mafham);
    cv_mod_halfwindow.setValue(mods.flags & Replay::ModFlags::HalfWindow);
    cv_mod_halfwindow_allow_300s.setValue(mods.flags & Replay::ModFlags::HalfWindowAllow300s);
    cv_mod_ming3012.setValue(mods.flags & Replay::ModFlags::Ming3012);
    cv_mod_no100s.setValue(mods.flags & Replay::ModFlags::No100s);
    cv_mod_no50s.setValue(mods.flags & Replay::ModFlags::No50s);
    cv_notelock_type.setValue(mods.notelock_type);
    cv_autopilot_lenience.setValue(mods.autopilot_lenience);
    cv_mod_timewarp_multiplier.setValue(mods.timewarp_multiplier);
    cv_mod_minimize_multiplier.setValue(mods.minimize_multiplier);
    cv_mod_artimewarp_multiplier.setValue(mods.artimewarp_multiplier);
    cv_mod_arwobble_strength.setValue(mods.arwobble_strength);
    cv_mod_arwobble_interval.setValue(mods.arwobble_interval);
    cv_mod_wobble_strength.setValue(mods.wobble_strength);
    cv_mod_wobble_rotation_speed.setValue(mods.wobble_rotation_speed);
    cv_mod_jigsaw_followcircle_radius_factor.setValue(mods.jigsaw_followcircle_radius_factor);
    cv_mod_shirone_combo.setValue(mods.shirone_combo);
    cv_ar_override.setValue(mods.ar_override);
    cv_ar_overridenegative.setValue(mods.ar_overridenegative);
    cv_cs_override.setValue(mods.cs_override);
    cv_cs_overridenegative.setValue(mods.cs_overridenegative);
    cv_hp_override.setValue(mods.hp_override);
    cv_od_override.setValue(mods.od_override);
    if(mods.flags & Replay::ModFlags::Autoplay) {
        cv_mod_autoplay.setValue(true);
        cv_mod_autopilot.setValue(false);
        cv_mod_relax.setValue(false);
    } else {
        cv_mod_autoplay.setValue(false);
        cv_mod_autopilot.setValue(mods.flags & Replay::ModFlags::Autopilot);
        cv_mod_relax.setValue(mods.flags & Replay::ModFlags::Relax);
    }

    f32 speed_override = mods.speed == 1.f ? -1.f : mods.speed;
    cv_speed_override.setValue(speed_override);

    // Update mod selector UI
    mod_selector->enableModsFromFlags(mods.to_legacy());
    cv_speed_override.setValue(speed_override);  // enableModsFromFlags() edits cv_speed_override
    mod_selector->m_ARLock->setChecked(mods.flags & Replay::ModFlags::AROverrideLock);
    mod_selector->m_ODLock->setChecked(mods.flags & Replay::ModFlags::AROverrideLock);
    mod_selector->m_speedSlider->setValue(mods.speed, false, false);
    mod_selector->m_CSSlider->setValue(mods.cs_override, false, false);
    mod_selector->m_ARSlider->setValue(mods.ar_override, false, false);
    mod_selector->m_ODSlider->setValue(mods.od_override, false, false);
    mod_selector->m_HPSlider->setValue(mods.hp_override, false, false);
    mod_selector->updateOverrideSliderLabels();
    mod_selector->updateExperimentalButtons();

    osu->updateMods();
}
