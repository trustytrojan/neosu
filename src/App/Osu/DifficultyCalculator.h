#pragma once
#include "cbase.h"

class Beatmap;

class SliderCurve;

class ConVar;

struct pp_info {
    f64 total_stars = 0.0;
    f64 aim_stars = 0.0;
    f64 aim_slider_factor = 0.0;
    f64 speed_stars = 0.0;
    f64 speed_notes = 0.0;
    f64 pp = -1.0;
};

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
    };

    struct SliderScoringTimeComparator {
        bool operator()(const SLIDER_SCORING_TIME &a, const SLIDER_SCORING_TIME &b) const {
            if(a.time != b.time) return a.time < b.time;
            return &a < &b;
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
};

class DifficultyCalculator {
   public:
    static constexpr const i32 PP_ALGORITHM_VERSION = 20220902;

    class Skills {
       public:
        static constexpr const int NUM_SKILLS = 3;

       public:
        enum class Skill {
            SPEED,
            AIM_SLIDERS,
            AIM_NO_SLIDERS,
        };

        static int skillToIndex(const Skill skill) {
            switch(skill) {
                case Skill::SPEED:
                    return 0;
                case Skill::AIM_SLIDERS:
                    return 1;
                case Skill::AIM_NO_SLIDERS:
                    return 2;
            }

            return 0;
        }
    };

    // see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Speed.cs
    // see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Aim.cs

    // how much strains decay per interval (if the previous interval's peak strains after applying decay are
    // still higher than the current one's, they will be used as the peak strains).
    static constexpr const double decay_base[Skills::NUM_SKILLS] = {0.3, 0.15, 0.15};

    // used to keep speed and aim balanced between eachother
    static constexpr const double weight_scaling[Skills::NUM_SKILLS] = {1375.0, 23.55, 23.55};

    class DiffObject {
       public:
        OsuDifficultyHitObject *ho;

        double strains[Skills::NUM_SKILLS];

        // https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Speed.cs
        // needed because raw speed strain and rhythm strain are combined in different ways
        double raw_speed_strain;
        double rhythm;

        Vector2 norm_start;  // start position normalized on radius

        double angle;  // precalc

        double jumpDistance;     // precalc
        double minJumpDistance;  // precalc
        double minJumpTime;      // precalc
        double travelDistance;   // precalc

        double delta_time;   // strain temp
        double strain_time;  // strain temp

        bool lazyCalcFinished;  // precalc temp
        Vector2 lazyEndPos;     // precalc temp
        double lazyTravelDist;  // precalc temp
        double lazyTravelTime;  // precalc temp
        double travelTime;      // precalc temp

        // NOTE(McKay): McOsu stores the first object in this array while lazer doesn't. newer lazer algorithms
        // require referencing objects "randomly", so we just keep the entire vector around.
        const std::vector<DiffObject> &objects;

        // WARNING: this will be -1 for the first object (as the name implies), see note above
        int prevObjectIndex;

        DiffObject(OsuDifficultyHitObject *base_object, float radius_scaling_factor,
                   std::vector<DiffObject> &diff_objects, int prevObjectIdx);

        inline const DiffObject *get_previous(int backwardsIdx) const {
            int foo = prevObjectIndex - backwardsIdx;
            if(foo < 0) foo = 0;  // msvc
            return (objects.size() > 0 && prevObjectIndex - backwardsIdx < (int)objects.size() ? &objects[foo] : NULL);
        }
        inline static double applyDiminishingExp(double val) { return std::pow(val, 0.99); }
        inline static double strainDecay(Skills::Skill type, double ms) {
            return std::pow(decay_base[Skills::skillToIndex(type)], ms / 1000.0);
        }

        void calculate_strains(const DiffObject &prev, const DiffObject *next, double hitWindow300);
        void calculate_strain(const DiffObject &prev, const DiffObject *next, double hitWindow300,
                              const Skills::Skill dtype);
        static double calculate_difficulty(const Skills::Skill type, const std::vector<DiffObject> &dobjects,
                                           std::vector<double> *outStrains = NULL, double *outRelevantNotes = NULL);
        static double spacing_weight1(const double distance, const Skills::Skill diff_type);
        double spacing_weight2(const Skills::Skill diff_type, const DiffObject &prev, const DiffObject *next,
                               double hitWindow300);
    };

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
    static f64 calculateStarDiffForHitObjectsInt(std::vector<DiffObject> &cachedDiffObjects,
                                                 std::vector<DiffObject> &diffObjects,
                                                 std::vector<OsuDifficultyHitObject> &sortedHitObjects, f32 CS, f32 OD,
                                                 f32 speedMultiplier, bool relax, bool touchDevice, f64 *aim,
                                                 f64 *aimSliderFactor, f64 *speed, f64 *speedNotes, i32 upToObjectIndex,
                                                 std::vector<f64> *outAimStrains, std::vector<f64> *outSpeedStrains,
                                                 const std::atomic<bool> &dead);

    // pp, fully static
    static f64 calculatePPv2(i32 modsLegacy, f64 timescale, f64 ar, f64 od, f64 aim, f64 aimSliderFactor, f64 speed,
                             f64 speedNotes, i32 numCircles, i32 numSliders, i32 numSpinners, i32 maxPossibleCombo,
                             i32 combo, i32 misses, i32 c300, i32 c100, i32 c50);
};
