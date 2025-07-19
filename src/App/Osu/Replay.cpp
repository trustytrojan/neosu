#include "Replay.h"

#include "Beatmap.h"
#include "CBaseUICheckbox.h"
#include "CBaseUISlider.h"
#include "DatabaseBeatmap.h"
#include "GameRules.h"
#include "Osu.h"

i32 Replay::Mods::to_legacy() const {
    i32 legacy_flags = 0;
    if(this->speed > 1.f) {
        legacy_flags |= LegacyFlags::DoubleTime;
        if(this->flags & ModFlags::NoPitchCorrection) legacy_flags |= LegacyFlags::Nightcore;
    } else if(this->speed < 1.f) {
        legacy_flags |= LegacyFlags::HalfTime;
    }

    if(this->flags & ModFlags::NoFail) legacy_flags |= LegacyFlags::NoFail;
    if(this->flags & ModFlags::Easy) legacy_flags |= LegacyFlags::Easy;
    if(this->flags & ModFlags::TouchDevice) legacy_flags |= LegacyFlags::TouchDevice;
    if(this->flags & ModFlags::Hidden) legacy_flags |= LegacyFlags::Hidden;
    if(this->flags & ModFlags::HardRock) legacy_flags |= LegacyFlags::HardRock;
    if(this->flags & ModFlags::SuddenDeath) legacy_flags |= LegacyFlags::SuddenDeath;
    if(this->flags & ModFlags::Relax) legacy_flags |= LegacyFlags::Relax;
    if(this->flags & ModFlags::Flashlight) legacy_flags |= LegacyFlags::Flashlight;
    if(this->flags & ModFlags::SpunOut) legacy_flags |= LegacyFlags::SpunOut;
    if(this->flags & ModFlags::Autopilot) legacy_flags |= LegacyFlags::Autopilot;
    if(this->flags & ModFlags::Perfect) legacy_flags |= LegacyFlags::Perfect;
    if(this->flags & ModFlags::Target) legacy_flags |= LegacyFlags::Target;
    if(this->flags & ModFlags::ScoreV2) legacy_flags |= LegacyFlags::ScoreV2;
    if(this->flags & ModFlags::Autoplay) {
        legacy_flags &= ~(LegacyFlags::Relax | LegacyFlags::Autopilot);
        legacy_flags |= LegacyFlags::Autoplay;
    }

    // NOTE: Ignoring nightmare, fposu

    return legacy_flags;
}

f32 Replay::Mods::get_naive_ar(DatabaseBeatmap *diff2) const {
    float ARdifficultyMultiplier = 1.0f;
    if((this->flags & Replay::ModFlags::HardRock)) ARdifficultyMultiplier = 1.4f;
    if((this->flags & Replay::ModFlags::Easy)) ARdifficultyMultiplier = 0.5f;

    f32 AR = std::clamp<f32>(diff2->getAR() * ARdifficultyMultiplier, 0.0f, 10.0f);
    if(this->ar_override >= 0.0f) AR = this->ar_override;
    if(this->ar_overridenegative < 0.0f) AR = this->ar_overridenegative;

    if(this->flags & Replay::ModFlags::AROverrideLock) {
        AR = GameRules::getRawConstantApproachRateForSpeedMultiplier(GameRules::getRawApproachTime(AR), this->speed);
    }

    return AR;
}

f32 Replay::Mods::get_naive_od(DatabaseBeatmap *diff2) const {
    float ODdifficultyMultiplier = 1.0f;
    if((this->flags & Replay::ModFlags::HardRock)) ODdifficultyMultiplier = 1.4f;
    if((this->flags & Replay::ModFlags::Easy)) ODdifficultyMultiplier = 0.5f;

    f32 OD = std::clamp<f32>(diff2->getOD() * ODdifficultyMultiplier, 0.0f, 10.0f);
    if(this->od_override >= 0.0f) OD = this->od_override;

    if(this->flags & Replay::ModFlags::ODOverrideLock) {
        OD = GameRules::getRawConstantOverallDifficultyForSpeedMultiplier(GameRules::getRawHitWindow300(OD),
                                                                          this->speed);
    }

    return OD;
}

Replay::Mods Replay::Mods::from_cvars() {
    Replay::Mods mods;

    if(cv::mod_nofail.getBool()) mods.flags |= Replay::ModFlags::NoFail;
    if(cv::mod_easy.getBool()) mods.flags |= Replay::ModFlags::Easy;
    if(cv::mod_autopilot.getBool()) mods.flags |= Replay::ModFlags::Autopilot;
    if(cv::mod_relax.getBool()) mods.flags |= Replay::ModFlags::Relax;
    if(cv::mod_hidden.getBool()) mods.flags |= Replay::ModFlags::Hidden;
    if(cv::mod_hardrock.getBool()) mods.flags |= Replay::ModFlags::HardRock;
    if(cv::mod_flashlight.getBool()) mods.flags |= Replay::ModFlags::Flashlight;
    if(cv::mod_suddendeath.getBool()) mods.flags |= Replay::ModFlags::SuddenDeath;
    if(cv::mod_perfect.getBool()) mods.flags |= Replay::ModFlags::Perfect;
    if(cv::mod_nightmare.getBool()) mods.flags |= Replay::ModFlags::Nightmare;
    if(cv::nightcore_enjoyer.getBool()) mods.flags |= Replay::ModFlags::NoPitchCorrection;
    if(cv::mod_touchdevice.getBool()) mods.flags |= Replay::ModFlags::TouchDevice;
    if(cv::mod_spunout.getBool()) mods.flags |= Replay::ModFlags::SpunOut;
    if(cv::mod_scorev2.getBool()) mods.flags |= Replay::ModFlags::ScoreV2;
    if(cv::mod_fposu.getBool()) mods.flags |= Replay::ModFlags::FPoSu;
    if(cv::mod_target.getBool()) mods.flags |= Replay::ModFlags::Target;
    if(cv::ar_override_lock.getBool()) mods.flags |= Replay::ModFlags::AROverrideLock;
    if(cv::od_override_lock.getBool()) mods.flags |= Replay::ModFlags::ODOverrideLock;
    if(cv::mod_timewarp.getBool()) mods.flags |= Replay::ModFlags::Timewarp;
    if(cv::mod_artimewarp.getBool()) mods.flags |= Replay::ModFlags::ARTimewarp;
    if(cv::mod_minimize.getBool()) mods.flags |= Replay::ModFlags::Minimize;
    if(cv::mod_jigsaw1.getBool()) mods.flags |= Replay::ModFlags::Jigsaw1;
    if(cv::mod_jigsaw2.getBool()) mods.flags |= Replay::ModFlags::Jigsaw2;
    if(cv::mod_wobble.getBool()) mods.flags |= Replay::ModFlags::Wobble1;
    if(cv::mod_wobble2.getBool()) mods.flags |= Replay::ModFlags::Wobble2;
    if(cv::mod_arwobble.getBool()) mods.flags |= Replay::ModFlags::ARWobble;
    if(cv::mod_fullalternate.getBool()) mods.flags |= Replay::ModFlags::FullAlternate;
    if(cv::mod_shirone.getBool()) mods.flags |= Replay::ModFlags::Shirone;
    if(cv::mod_mafham.getBool()) mods.flags |= Replay::ModFlags::Mafham;
    if(cv::mod_halfwindow.getBool()) mods.flags |= Replay::ModFlags::HalfWindow;
    if(cv::mod_halfwindow_allow_300s.getBool()) mods.flags |= Replay::ModFlags::HalfWindowAllow300s;
    if(cv::mod_ming3012.getBool()) mods.flags |= Replay::ModFlags::Ming3012;
    if(cv::mod_no100s.getBool()) mods.flags |= Replay::ModFlags::No100s;
    if(cv::mod_no50s.getBool()) mods.flags |= Replay::ModFlags::No50s;
    if(cv::mod_autoplay.getBool()) {
        mods.flags &= ~(Replay::ModFlags::Relax | Replay::ModFlags::Autopilot);
        mods.flags |= Replay::ModFlags::Autoplay;
    }

    auto beatmap = osu->getSelectedBeatmap();
    if(beatmap != NULL) {
        mods.speed = beatmap->getSpeedMultiplier();
    }

    mods.notelock_type = cv::notelock_type.getInt();
    mods.autopilot_lenience = cv::autopilot_lenience.getFloat();
    mods.ar_override = cv::ar_override.getFloat();
    mods.ar_overridenegative = cv::ar_overridenegative.getFloat();
    mods.cs_override = cv::cs_override.getFloat();
    mods.cs_overridenegative = cv::cs_overridenegative.getFloat();
    mods.hp_override = cv::hp_override.getFloat();
    mods.od_override = cv::od_override.getFloat();
    mods.timewarp_multiplier = cv::mod_timewarp_multiplier.getFloat();
    mods.minimize_multiplier = cv::mod_minimize_multiplier.getFloat();
    mods.artimewarp_multiplier = cv::mod_artimewarp_multiplier.getFloat();
    mods.arwobble_strength = cv::mod_arwobble_strength.getFloat();
    mods.arwobble_interval = cv::mod_arwobble_interval.getFloat();
    mods.wobble_strength = cv::mod_wobble_strength.getFloat();
    mods.wobble_rotation_speed = cv::mod_wobble_rotation_speed.getFloat();
    mods.jigsaw_followcircle_radius_factor = cv::mod_jigsaw_followcircle_radius_factor.getFloat();
    mods.shirone_combo = cv::mod_shirone_combo.getFloat();

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
    cv::mod_nofail.setValue<bool>(!!(mods.flags & Replay::ModFlags::NoFail));
    cv::mod_easy.setValue<bool>(!!(mods.flags & Replay::ModFlags::Easy));
    cv::mod_hidden.setValue<bool>(!!(mods.flags & Replay::ModFlags::Hidden));
    cv::mod_hardrock.setValue<bool>(!!(mods.flags & Replay::ModFlags::HardRock));
    cv::mod_flashlight.setValue<bool>(!!(mods.flags & Replay::ModFlags::Flashlight));
    cv::mod_suddendeath.setValue<bool>(!!(mods.flags & Replay::ModFlags::SuddenDeath));
    cv::mod_perfect.setValue<bool>(!!(mods.flags & Replay::ModFlags::Perfect));
    cv::mod_nightmare.setValue<bool>(!!(mods.flags & Replay::ModFlags::Nightmare));
    cv::nightcore_enjoyer.setValue<bool>(!!(mods.flags & Replay::ModFlags::NoPitchCorrection));
    cv::mod_touchdevice.setValue<bool>(!!(mods.flags & Replay::ModFlags::TouchDevice));
    cv::mod_spunout.setValue<bool>(!!(mods.flags & Replay::ModFlags::SpunOut));
    cv::mod_scorev2.setValue<bool>(!!(mods.flags & Replay::ModFlags::ScoreV2));
    cv::mod_fposu.setValue<bool>(!!(mods.flags & Replay::ModFlags::FPoSu));
    cv::mod_target.setValue<bool>(!!(mods.flags & Replay::ModFlags::Target));
    cv::ar_override_lock.setValue<bool>(!!(mods.flags & Replay::ModFlags::AROverrideLock));
    cv::od_override_lock.setValue<bool>(!!(mods.flags & Replay::ModFlags::ODOverrideLock));
    cv::mod_timewarp.setValue<bool>(!!(mods.flags & Replay::ModFlags::Timewarp));
    cv::mod_artimewarp.setValue<bool>(!!(mods.flags & Replay::ModFlags::ARTimewarp));
    cv::mod_minimize.setValue<bool>(!!(mods.flags & Replay::ModFlags::Minimize));
    cv::mod_jigsaw1.setValue<bool>(!!(mods.flags & Replay::ModFlags::Jigsaw1));
    cv::mod_jigsaw2.setValue<bool>(!!(mods.flags & Replay::ModFlags::Jigsaw2));
    cv::mod_wobble.setValue<bool>(!!(mods.flags & Replay::ModFlags::Wobble1));
    cv::mod_wobble2.setValue<bool>(!!(mods.flags & Replay::ModFlags::Wobble2));
    cv::mod_arwobble.setValue<bool>(!!(mods.flags & Replay::ModFlags::ARWobble));
    cv::mod_fullalternate.setValue<bool>(!!(mods.flags & Replay::ModFlags::FullAlternate));
    cv::mod_shirone.setValue<bool>(!!(mods.flags & Replay::ModFlags::Shirone));
    cv::mod_mafham.setValue<bool>(!!(mods.flags & Replay::ModFlags::Mafham));
    cv::mod_halfwindow.setValue<bool>(!!(mods.flags & Replay::ModFlags::HalfWindow));
    cv::mod_halfwindow_allow_300s.setValue<bool>(!!(mods.flags & Replay::ModFlags::HalfWindowAllow300s));
    cv::mod_ming3012.setValue<bool>(!!(mods.flags & Replay::ModFlags::Ming3012));
    cv::mod_no100s.setValue<bool>(!!(mods.flags & Replay::ModFlags::No100s));
    cv::mod_no50s.setValue<bool>(!!(mods.flags & Replay::ModFlags::No50s));
    cv::notelock_type.setValue(mods.notelock_type);
    cv::autopilot_lenience.setValue(mods.autopilot_lenience);
    cv::mod_timewarp_multiplier.setValue(mods.timewarp_multiplier);
    cv::mod_minimize_multiplier.setValue(mods.minimize_multiplier);
    cv::mod_artimewarp_multiplier.setValue(mods.artimewarp_multiplier);
    cv::mod_arwobble_strength.setValue(mods.arwobble_strength);
    cv::mod_arwobble_interval.setValue(mods.arwobble_interval);
    cv::mod_wobble_strength.setValue(mods.wobble_strength);
    cv::mod_wobble_rotation_speed.setValue(mods.wobble_rotation_speed);
    cv::mod_jigsaw_followcircle_radius_factor.setValue(mods.jigsaw_followcircle_radius_factor);
    cv::mod_shirone_combo.setValue(mods.shirone_combo);
    cv::ar_override.setValue(mods.ar_override);
    cv::ar_overridenegative.setValue(mods.ar_overridenegative);
    cv::cs_override.setValue(mods.cs_override);
    cv::cs_overridenegative.setValue(mods.cs_overridenegative);
    cv::hp_override.setValue(mods.hp_override);
    cv::od_override.setValue(mods.od_override);
    if(mods.flags & Replay::ModFlags::Autoplay) {
        cv::mod_autoplay.setValue(true);
        cv::mod_autopilot.setValue(false);
        cv::mod_relax.setValue(false);
    } else {
        cv::mod_autoplay.setValue(false);
        cv::mod_autopilot.setValue<bool>(!!(mods.flags & Replay::ModFlags::Autopilot));
        cv::mod_relax.setValue<bool>(!!(mods.flags & Replay::ModFlags::Relax));
    }

    f32 speed_override = mods.speed == 1.f ? -1.f : mods.speed;
    cv::speed_override.setValue(speed_override);

    // Update mod selector UI
    mod_selector->enableModsFromFlags(mods.to_legacy());
    cv::speed_override.setValue(speed_override);  // enableModsFromFlags() edits cv::speed_override
    mod_selector->ARLock->setChecked(!!(mods.flags & Replay::ModFlags::AROverrideLock));
    mod_selector->ODLock->setChecked(!!(mods.flags & Replay::ModFlags::AROverrideLock));
    mod_selector->speedSlider->setValue(mods.speed, false, false);
    mod_selector->CSSlider->setValue(mods.cs_override, false, false);
    mod_selector->ARSlider->setValue(mods.ar_override, false, false);
    mod_selector->ODSlider->setValue(mods.od_override, false, false);
    mod_selector->HPSlider->setValue(mods.hp_override, false, false);
    mod_selector->updateOverrideSliderLabels();
    mod_selector->updateExperimentalButtons();

    osu->updateMods();
}
