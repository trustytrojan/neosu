#pragma once

#include "Beatmap.h"

class SimulatedBeatmap : public BeatmapInterface {
   public:
    SimulatedBeatmap(DatabaseBeatmap *diff2);
    ~SimulatedBeatmap();

    // Important convars
    // TODO @kiwec: set these after initializing object, include in score
    i32 mod_flags = 0;
    float speed = 1.f;
    i32 osu_notelock_type = 2;
    f32 osu_autopilot_lenience = 0.75f;

    // Experimental mods
    // TODO @kiwec: set these after initializing object
    f32 osu_cs_override = -1.f;
    f32 osu_cs_overridenegative = 0.f;
    f32 osu_hp_override = -1.f;
    f32 osu_od_override = -1.f;
    bool osu_od_override_lock = false;
    bool osu_mod_timewarp = false;
    bool osu_mod_timewarp_multiplier = false;
    bool osu_mod_minimize = false;
    bool osu_mod_minimize_multiplier = false;
    bool osu_mod_jigsaw1 = false;
    bool osu_mod_artimewarp = false;
    bool osu_mod_artimewarp_multiplier = false;
    bool osu_mod_arwobble = false;
    bool osu_mod_arwobble_strength = false;
    bool osu_mod_arwobble_interval = false;
    bool osu_mod_fullalternate = false;
    bool osu_mod_wobble = false;
    bool osu_mod_wobble2 = false;
    f32 osu_mod_wobble_strength = 25.f;
    f32 osu_mod_wobble_frequency = 1.f;
    f32 osu_mod_wobble_rotation_speed = 1.f;
    bool osu_mod_jigsaw2 = false;
    f32 osu_mod_jigsaw_followcircle_radius_factor = 0.f;
    bool osu_mod_shirone = false;
    f32 osu_mod_shirone_combo = 20.f;
    // Also set these, inherited from BeatmapInterface
    // bool mod_halfwindow = false;
    // bool mod_halfwindow_allow_300s = false;
    // bool mod_ming3012 = false;
    // bool mod_no100s = false;
    // bool mod_no50s = false;

    LiveScore live_score;

    void simulate_to(i32 music_pos);

    void update();

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

    bool start();
    void fail();
    void cancelFailing();
    void resetScore();

    // live statistics
    inline int getNPS() const { return m_iNPS; }
    inline int getND() const { return m_iND; }
    inline int getHitObjectIndexForCurrentTime() const { return m_iCurrentHitObjectIndex; }
    inline int getNumCirclesForCurrentTime() const { return m_iCurrentNumCircles; }
    inline int getNumSlidersForCurrentTime() const { return m_iCurrentNumSliders; }
    inline int getNumSpinnersForCurrentTime() const { return m_iCurrentNumSpinners; }

    std::vector<f64> m_aimStrains;
    std::vector<f64> m_speedStrains;

    // set to false when using non-vanilla mods (disables score submission)
    bool vanilla = true;

    // replay recording
    u8 current_keys = 0;
    u8 last_keys = 0;

    // replay replaying (prerecorded)
    // current_keys, last_keys also reused
    std::vector<Replay::Frame> spectated_replay;
    Vector2 m_interpolatedMousePos;
    long current_frame_idx = 0;

    virtual u32 getScoreV1DifficultyMultiplier() const;
    virtual f32 getCS() const;
    virtual f32 getHP() const;
    virtual f32 getRawOD() const;
    virtual f32 getOD() const;

    virtual bool isContinueScheduled() const { return false; }
    virtual bool isPaused() const { return false; }

    // generic state
    virtual bool isKey1Down() const;
    virtual bool isKey2Down() const;
    virtual bool isClickHeld() const;

    DatabaseBeatmap::BREAK getBreakForTimeRange(long startMS, long positionMS, long endMS) const;

    // HitObject and other helper functions
    virtual LiveScore::HIT addHitResult(HitObject *hitObject, LiveScore::HIT hit, i32 delta, bool isEndOfCombo = false,
                                        bool ignoreOnHitErrorBar = false, bool hitErrorBarOnly = false,
                                        bool ignoreCombo = false, bool ignoreScore = false, bool ignoreHealth = false);
    virtual void addSliderBreak();
    void addHealth(f64 percent, bool isFromHitResult);

    virtual void addScorePoints(int points, bool isSpinner = false);
    virtual bool isWaiting() const { return false; }

    // live pp/stars
    i32 last_calculated_hitobject = -1;

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
    u64 m_iScoreV2ComboPortionMaximum;

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
    float m_fSliderFollowCircleDiameter;

    // auto
    Vector2 m_vAutoCursorPos;
};
