#pragma once
#include "Replay.h"
#include "score.h"

class HitObject;

class BeatmapInterface {
   public:
    virtual ~BeatmapInterface() = default;

    virtual LiveScore::HIT addHitResult(HitObject *hitObject, LiveScore::HIT hit, i32 delta, bool isEndOfCombo = false,
                                        bool ignoreOnHitErrorBar = false, bool hitErrorBarOnly = false,
                                        bool ignoreCombo = false, bool ignoreScore = false,
                                        bool ignoreHealth = false) = 0;

    [[nodiscard]] virtual u32 getBreakDurationTotal() const = 0;
    [[nodiscard]] virtual u32 getLength() const = 0;
    [[nodiscard]] virtual u32 getLengthPlayable() const = 0;
    [[nodiscard]] virtual bool isContinueScheduled() const = 0;
    [[nodiscard]] virtual bool isPaused() const = 0;
    [[nodiscard]] virtual bool isPlaying() const = 0;
    [[nodiscard]] virtual bool isWaiting() const = 0;
    [[nodiscard]] virtual bool isClickHeld() const = 0;
    [[nodiscard]] virtual bool isKey1Down() const = 0;
    [[nodiscard]] virtual bool isKey2Down() const = 0;

    virtual void addScorePoints(int points, bool isSpinner = false) = 0;
    virtual void addSliderBreak() = 0;
    [[nodiscard]] virtual u32 getScoreV1DifficultyMultiplier() const = 0;

    [[nodiscard]] virtual Replay::Mods getMods() const = 0;
    [[nodiscard]] virtual i32 getModsLegacy() const = 0;
    [[nodiscard]] virtual f32 getSpeedMultiplier() const = 0;
    [[nodiscard]] virtual f32 getRawAR() const = 0;
    [[nodiscard]] virtual f32 getRawOD() const = 0;
    [[nodiscard]] virtual f32 getAR() const = 0;
    [[nodiscard]] virtual f32 getCS() const = 0;
    [[nodiscard]] virtual f32 getHP() const = 0;
    [[nodiscard]] virtual f32 getOD() const = 0;

    [[nodiscard]] virtual f32 getApproachTime() const = 0;
    [[nodiscard]] virtual f32 getRawApproachTime() const = 0;

    [[nodiscard]] virtual Vector2 getCursorPos() const = 0;

    [[nodiscard]] virtual Vector2 pixels2OsuCoords(Vector2 pixelCoords) const = 0;
    [[nodiscard]] virtual Vector2 osuCoords2Pixels(Vector2 coords) const = 0;
    [[nodiscard]] virtual Vector2 osuCoords2RawPixels(Vector2 coords) const = 0;
    [[nodiscard]] virtual Vector2 osuCoords2LegacyPixels(Vector2 coords) const = 0;

    f64 fHpMultiplierComboEnd = 1.0;
    f64 fHpMultiplierNormal = 1.0;
    u32 iMaxPossibleCombo = 0;
    u32 iScoreV2ComboPortionMaximum = 0;

    // It is assumed these values are set correctly
    u32 nb_hitobjects = 0;
    f32 fHitcircleDiameter = 0.f;
    f32 fRawHitcircleDiameter = 0.f;
    f32 fSliderFollowCircleDiameter = 0.f;
    bool bPrevKeyWasKey1 = false;
    bool holding_slider = false;
    bool mod_halfwindow = false;
    bool mod_halfwindow_allow_300s = false;
    bool mod_ming3012 = false;
    bool mod_no100s = false;
    bool mod_no50s = false;

    // Generic behavior below, do not override
    LiveScore::HIT getHitResult(i32 delta);
    f32 getHitWindow300();
    f32 getRawHitWindow300();
    f32 getHitWindow100();
    f32 getHitWindow50();
    f32 getApproachRateForSpeedMultiplier();
    f32 getRawApproachRateForSpeedMultiplier();
    f32 getConstantApproachRateForSpeedMultiplier();
    f32 getOverallDifficultyForSpeedMultiplier();
    f32 getRawOverallDifficultyForSpeedMultiplier();
    f32 getConstantOverallDifficultyForSpeedMultiplier();
};
