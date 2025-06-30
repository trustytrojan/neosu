#pragma once

#include "Beatmap.h"
#include "Replay.h"

class SimulatedBeatmap : public BeatmapInterface {
   public:
    SimulatedBeatmap(DatabaseBeatmap *diff2, Replay::Mods mods_);
    ~SimulatedBeatmap() override;

    Replay::Mods mods;
    LiveScore live_score;

    void simulate_to(i32 music_pos);

    bool start();
    void update(f64 frame_time);

    // Potentially Visible Set gate time size, for optimizing draw() and update() when iterating over all hitobjects
    long getPVS();

    [[nodiscard]] Vector2 pixels2OsuCoords(Vector2 pixelCoords) const override;  // only used for positional audio atm
    [[nodiscard]] Vector2 osuCoords2Pixels(
        Vector2 coords) const override;  // hitobjects should use this one (includes lots of special behaviour)
    [[nodiscard]] Vector2 osuCoords2RawPixels(
        Vector2 coords) const override;  // raw transform from osu!pixels to absolute screen pixels (without any mods whatsoever)
    [[nodiscard]] Vector2 osuCoords2LegacyPixels(
        Vector2 coords) const override;  // only applies vanilla osu mods and static mods to the coordinates (used for generating
                                // the static slider mesh) centered at (0, 0, 0)

    // cursor
    [[nodiscard]] Vector2 getCursorPos() const override;
    [[nodiscard]] Vector2 getFirstPersonCursorDelta() const;
    [[nodiscard]] inline Vector2 getContinueCursorPoint() const { return this->vContinueCursorPoint; }

    // playfield
    [[nodiscard]] inline Vector2 getPlayfieldSize() const { return this->vPlayfieldSize; }
    [[nodiscard]] inline Vector2 getPlayfieldCenter() const { return this->vPlayfieldCenter; }
    [[nodiscard]] inline float getPlayfieldRotation() const { return this->fPlayfieldRotation; }

    // hitobjects
    [[nodiscard]] inline float getHitcircleXMultiplier() const {
        return this->fXMultiplier;
    }  // multiply osu!pixels with this to get screen pixels

    void fail(bool force_death = false);
    void cancelFailing();
    void resetScore();

    // live statistics
    [[nodiscard]] inline int getNPS() const { return this->iNPS; }
    [[nodiscard]] inline int getND() const { return this->iND; }

    // replay recording
    u8 current_keys = 0;
    u8 last_keys = 0;

    // replay replaying (prerecorded)
    // current_keys, last_keys also reused
    std::vector<LegacyReplay::Frame> spectated_replay;
    Vector2 interpolatedMousePos;
    long current_frame_idx = 0;

    // generic state
    [[nodiscard]] bool isKey1Down() const override;
    [[nodiscard]] bool isKey2Down() const override;
    [[nodiscard]] bool isClickHeld() const override;

    [[nodiscard]] bool isContinueScheduled() const override { return false; }
    [[nodiscard]] bool isPaused() const override { return false; }
    [[nodiscard]] bool isPlaying() const override { return true; }
    [[nodiscard]] Replay::Mods getMods() const override { return this->mods; }
    [[nodiscard]] i32 getModsLegacy() const override { return this->mods.to_legacy(); }
    [[nodiscard]] u32 getScoreV1DifficultyMultiplier() const override;
    [[nodiscard]] f32 getSpeedMultiplier() const override { return this->mods.speed; }
    [[nodiscard]] f32 getRawAR() const override;
    [[nodiscard]] f32 getAR() const override;
    [[nodiscard]] f32 getCS() const override;
    [[nodiscard]] f32 getHP() const override;
    [[nodiscard]] f32 getRawOD() const override;
    [[nodiscard]] f32 getOD() const override;
    [[nodiscard]] f32 getRawApproachTime() const override;
    [[nodiscard]] f32 getApproachTime() const override;
    [[nodiscard]] u32 getLength() const override;
    [[nodiscard]] u32 getLengthPlayable() const override;
    [[nodiscard]] u32 getBreakDurationTotal() const override;
    [[nodiscard]] DatabaseBeatmap::BREAK getBreakForTimeRange(long startMS, long positionMS, long endMS) const;

    // HitObject and other helper functions
    LiveScore::HIT addHitResult(HitObject *hitObject, LiveScore::HIT hit, i32 delta, bool isEndOfCombo = false,
                                        bool ignoreOnHitErrorBar = false, bool hitErrorBarOnly = false,
                                        bool ignoreCombo = false, bool ignoreScore = false, bool ignoreHealth = false) override;
    void addSliderBreak() override;
    void addHealth(f64 percent, bool isFromHitResult);

    void addScorePoints(int points, bool isSpinner = false) override;
    [[nodiscard]] bool isWaiting() const override { return false; }

   protected:
    // database
    DatabaseBeatmap *selectedDifficulty2;

    // sound
    i32 iCurMusicPos = 0;

    // health
    bool bFailed = false;
    f64 fHealth = 1.0;
    f64 fDrainRate = 0.0;

    // breaks
    std::vector<DatabaseBeatmap::BREAK> breaks;
    bool bInBreak = false;
    HitObject *currentHitObject = NULL;
    i32 iNextHitObjectTime = 0;
    i32 iPreviousHitObjectTime = 0;

    // player input
    i32 iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex = 0;
    std::vector<Click> clicks;

    // hitobjects
    std::vector<HitObject *> hitobjects;
    std::vector<HitObject *> hitobjectsSortedByEndTime;

    // statistics
    i32 iNPS;
    i32 iND;
    i32 iCurrentHitObjectIndex;
    i32 iCurrentNumCircles;
    i32 iCurrentNumSliders;
    i32 iCurrentNumSpinners;

   private:
    static inline Vector2 mapNormalizedCoordsOntoUnitCircle(const Vector2 &in) {
        return Vector2(in.x * std::sqrt(1.0f - in.y * in.y / 2.0f), in.y * std::sqrt(1.0f - in.x * in.x / 2.0f));
    }

    static float quadLerp3f(float left, float center, float right, float percent) {
        if(percent >= 0.5f) {
            percent = (percent - 0.5f) / 0.5f;
            percent *= percent;
            return std::lerp<float>(center, right, percent);
        } else {
            percent = percent / 0.5f;
            percent = 1.0f - (1.0f - percent) * (1.0f - percent);
            return std::lerp<float>(left, center, percent);
        }
    }

    void updateAutoCursorPos();
    void updatePlayfieldMetrics();
    void updateHitobjectMetrics();

    void calculateStacks();
    void computeDrainRate();

    // beatmap
    bool bIsSpinnerActive;
    Vector2 vContinueCursorPoint;

    // playfield
    float fPlayfieldRotation;
    Vector2 vPlayfieldCenter;
    Vector2 vPlayfieldSize;

    // hitobject scaling
    float fXMultiplier;

    // auto
    Vector2 vAutoCursorPos;
};
