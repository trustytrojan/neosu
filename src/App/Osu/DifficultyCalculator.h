#pragma once
#include "cbase.h"

class Beatmap;

class SliderCurve;

class ConVar;

class OsuDifficultyHitObject {
   public:
    enum class TYPE : u8 {
        INVALID = 0,
        CIRCLE,
        SPINNER,
        SLIDER,
    };

    struct SLIDER_SCORING_TIME {
        enum class TYPE {
            TICK,
            REPEAT,
            END,
        };

        TYPE type;
        u32 time;

        u64 sortHack;
    };

    struct SliderScoringTimeComparator {
        bool operator()(const SLIDER_SCORING_TIME &a, const SLIDER_SCORING_TIME &b) const {
            // strict weak ordering!
            if(a.time == b.time)
                return a.sortHack < b.sortHack;
            else
                return a.time < b.time;
        }
    };

   public:
    OsuDifficultyHitObject(TYPE type, Vector2 pos, i32 time);               // circle
    OsuDifficultyHitObject(TYPE type, Vector2 pos, i32 time, i32 endTime);  // spinner
    OsuDifficultyHitObject(TYPE type, Vector2 pos, i32 time, i32 endTime, f32 spanDuration, u8 osuSliderCurveType,
                           std::vector<Vector2> controlPoints, f32 pixelLength,
                           std::vector<SLIDER_SCORING_TIME> scoringTimes, i32 repeats,
                           bool calculateSliderCurveInConstructor);  // slider
    ~OsuDifficultyHitObject();

    OsuDifficultyHitObject(const OsuDifficultyHitObject &) = delete;
    OsuDifficultyHitObject(OsuDifficultyHitObject &&dobj);

    OsuDifficultyHitObject &operator=(OsuDifficultyHitObject &&dobj);

    void updateStackPosition(f32 stackOffset);
    void updateCurveStackPosition(f32 stackOffset);

    // for stacking calculations, always returns the unstacked original position at that point in time
    Vector2 getOriginalRawPosAt(i32 pos);

    f32 getT(i32 pos, bool raw);

    inline i32 getDuration() const { return endTime - time; }

    // circles (base)
    TYPE type;
    Vector2 pos;
    i32 time;

    // spinners + sliders
    i32 endTime;

    // sliders
    f32 spanDuration;  // i.e. sliderTimeWithoutRepeats
    u8 osuSliderCurveType;
    f32 pixelLength;
    std::vector<SLIDER_SCORING_TIME> scoringTimes;
    i32 repeats;

    // custom
    SliderCurve *curve;
    bool scheduledCurveAlloc;
    std::vector<Vector2> scheduledCurveAllocControlPoints;
    f32 scheduledCurveAllocStackOffset;

    i32 stack;
    Vector2 originalPos;

    u64 sortHack;

   private:
    static u64 sortHackCounter;
};

class DifficultyCalculator {
   public:
    static constexpr const i32 PP_ALGORITHM_VERSION = 20220902;

   public:
    // stars, fully static
    static f64 calculateStarDiffForHitObjects(std::vector<OsuDifficultyHitObject> &sortedHitObjects, f32 CS, f32 OD,
                                              f32 speedMultiplier, bool relax, bool touchDevice, f64 *aim,
                                              f64 *aimSliderFactor, f64 *speed, f64 *speedNotes, i32 upToObjectIndex,
                                              std::vector<f64> *outAimStrains, std::vector<f64> *outSpeedStrains);
    static f64 calculateStarDiffForHitObjects(std::vector<OsuDifficultyHitObject> &sortedHitObjects, f32 CS, f32 OD,
                                              f32 speedMultiplier, bool relax, bool touchDevice, f64 *aim,
                                              f64 *aimSliderFactor, f64 *speed, f64 *speedNotes, i32 upToObjectIndex,
                                              std::vector<f64> *outAimStrains, std::vector<f64> *outSpeedStrains,
                                              const std::atomic<bool> &dead);

    // pp, use runtime mods (convenience)
    static f64 calculatePPv2(Beatmap *beatmap, f64 aim, f64 aimSliderFactor, f64 speed, f64 speedNotes,
                             i32 numHitObjects, i32 numCircles, i32 numSliders, i32 numSpinners, i32 maxPossibleCombo,
                             i32 combo = -1, i32 misses = 0, i32 c300 = -1, i32 c100 = 0, i32 c50 = 0);

    // pp, fully static
    static f64 calculatePPv2(i32 modsLegacy, f64 timescale, f64 ar, f64 od, f64 aim, f64 aimSliderFactor, f64 speed,
                             f64 speedNotes, i32 numHitObjects, i32 numCircles, i32 numSliders, i32 numSpinners,
                             i32 maxPossibleCombo, i32 combo, i32 misses, i32 c300, i32 c100, i32 c50);

   private:
    static ConVar *m_osu_slider_scorev2_ref;
};
