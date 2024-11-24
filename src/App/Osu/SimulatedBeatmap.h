#pragma once

#include "Beatmap.h"
#include "Replay.h"

class SimulatedBeatmap : public BeatmapInterface {
   public:
    SimulatedBeatmap(DatabaseBeatmap *diff2, Replay::Mods mods_);
    ~SimulatedBeatmap();

    Replay::Mods mods;
    LiveScore live_score;

    void simulate_to(i32 music_pos);

    bool start();
    void update(f64 frame_time);

    // Potentially Visible Set gate time size, for optimizing draw() and update() when iterating over all hitobjects
    long getPVS();

    Vector2 pixels2OsuCoords(Vector2 pixelCoords) const;  // only used for positional audio atm
    Vector2 osuCoords2Pixels(
        Vector2 coords) const;  // hitobjects should use this one (includes lots of special behaviour)
    Vector2 osuCoords2RawPixels(
        Vector2 coords) const;  // raw transform from osu!pixels to absolute screen pixels (without any mods whatsoever)
    Vector2 osuCoords2LegacyPixels(
        Vector2 coords) const;  // only applies vanilla osu mods and static mods to the coordinates (used for generating
                                // the static slider mesh) centered at (0, 0, 0)

    // cursor
    virtual Vector2 getCursorPos() const;
    Vector2 getFirstPersonCursorDelta() const;
    inline Vector2 getContinueCursorPoint() const { return m_vContinueCursorPoint; }

    // playfield
    inline Vector2 getPlayfieldSize() const { return m_vPlayfieldSize; }
    inline Vector2 getPlayfieldCenter() const { return m_vPlayfieldCenter; }
    inline float getPlayfieldRotation() const { return m_fPlayfieldRotation; }

    // hitobjects
    inline float getHitcircleXMultiplier() const {
        return m_fXMultiplier;
    }  // multiply osu!pixels with this to get screen pixels

    void fail();
    void cancelFailing();
    void resetScore();

    // live statistics
    inline int getNPS() const { return m_iNPS; }
    inline int getND() const { return m_iND; }

    // replay recording
    u8 current_keys = 0;
    u8 last_keys = 0;

    // replay replaying (prerecorded)
    // current_keys, last_keys also reused
    std::vector<LegacyReplay::Frame> spectated_replay;
    Vector2 m_interpolatedMousePos;
    long current_frame_idx = 0;

    // generic state
    virtual bool isKey1Down() const;
    virtual bool isKey2Down() const;
    virtual bool isClickHeld() const;

    virtual bool isContinueScheduled() const { return false; }
    virtual bool isPaused() const { return false; }
    virtual bool isPlaying() const { return true; }
    virtual Replay::Mods getMods() const { return mods; }
    virtual i32 getModsLegacy() const { return mods.to_legacy(); }
    virtual u32 getScoreV1DifficultyMultiplier() const;
    virtual f32 getSpeedMultiplier() const { return mods.speed; }
    virtual f32 getRawAR() const;
    virtual f32 getAR() const;
    virtual f32 getCS() const;
    virtual f32 getHP() const;
    virtual f32 getRawOD() const;
    virtual f32 getOD() const;
    virtual f32 getRawApproachTime() const;
    virtual f32 getApproachTime() const;
    virtual u32 getLength() const;
    virtual u32 getLengthPlayable() const;
    virtual u32 getBreakDurationTotal() const;
    DatabaseBeatmap::BREAK getBreakForTimeRange(long startMS, long positionMS, long endMS) const;

    // HitObject and other helper functions
    virtual LiveScore::HIT addHitResult(HitObject *hitObject, LiveScore::HIT hit, i32 delta, bool isEndOfCombo = false,
                                        bool ignoreOnHitErrorBar = false, bool hitErrorBarOnly = false,
                                        bool ignoreCombo = false, bool ignoreScore = false, bool ignoreHealth = false);
    virtual void addSliderBreak();
    void addHealth(f64 percent, bool isFromHitResult);

    virtual void addScorePoints(int points, bool isSpinner = false);
    virtual bool isWaiting() const { return false; }

   protected:
    // database
    DatabaseBeatmap *m_selectedDifficulty2;

    // sound
    i32 m_iCurMusicPos = 0;

    // health
    bool m_bFailed = false;
    f64 m_fHealth = 1.0;
    f64 m_fDrainRate = 0.0;

    // breaks
    std::vector<DatabaseBeatmap::BREAK> m_breaks;
    bool m_bInBreak = false;
    HitObject *m_currentHitObject = NULL;
    i32 m_iNextHitObjectTime = 0;
    i32 m_iPreviousHitObjectTime = 0;

    // player input
    i32 m_iAllowAnyNextKeyForFullAlternateUntilHitObjectIndex = 0;
    std::vector<Click> m_clicks;

    // hitobjects
    std::vector<HitObject *> m_hitobjects;
    std::vector<HitObject *> m_hitobjectsSortedByEndTime;

    // statistics
    i32 m_iNPS;
    i32 m_iND;
    i32 m_iCurrentHitObjectIndex;
    i32 m_iCurrentNumCircles;
    i32 m_iCurrentNumSliders;
    i32 m_iCurrentNumSpinners;

   private:
    static inline Vector2 mapNormalizedCoordsOntoUnitCircle(const Vector2 &in) {
        return Vector2(in.x * std::sqrt(1.0f - in.y * in.y / 2.0f), in.y * std::sqrt(1.0f - in.x * in.x / 2.0f));
    }

    static float quadLerp3f(float left, float center, float right, float percent) {
        if(percent >= 0.5f) {
            percent = (percent - 0.5f) / 0.5f;
            percent *= percent;
            return lerp<float>(center, right, percent);
        } else {
            percent = percent / 0.5f;
            percent = 1.0f - (1.0f - percent) * (1.0f - percent);
            return lerp<float>(left, center, percent);
        }
    }

    void updateAutoCursorPos();
    void updatePlayfieldMetrics();
    void updateHitobjectMetrics();

    void calculateStacks();
    void computeDrainRate();

    // beatmap
    bool m_bIsSpinnerActive;
    Vector2 m_vContinueCursorPoint;

    // playfield
    float m_fPlayfieldRotation;
    Vector2 m_vPlayfieldCenter;
    Vector2 m_vPlayfieldSize;

    // hitobject scaling
    float m_fXMultiplier;

    // auto
    Vector2 m_vAutoCursorPos;
};
