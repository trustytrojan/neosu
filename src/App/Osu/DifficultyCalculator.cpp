#include "DifficultyCalculator.h"

#include "Beatmap.h"
#include "ConVar.h"
#include "Engine.h"
#include "GameRules.h"
#include "Osu.h"
#include "Replay.h"
#include "SliderCurves.h"

using namespace std;

ConVar osu_stars_slider_curve_points_separation(
    "osu_stars_slider_curve_points_separation", 20.0f, FCVAR_DEFAULT,
    "massively reduce curve accuracy for star calculations to save memory/performance");

u64 OsuDifficultyHitObject::sortHackCounter = 0;

OsuDifficultyHitObject::OsuDifficultyHitObject(TYPE type, Vector2 pos, i32 time)
    : OsuDifficultyHitObject(type, pos, time, time) {}

OsuDifficultyHitObject::OsuDifficultyHitObject(TYPE type, Vector2 pos, i32 time, i32 endTime)
    : OsuDifficultyHitObject(type, pos, time, endTime, 0.0f, '\0', std::vector<Vector2>(), 0.0f,
                             std::vector<SLIDER_SCORING_TIME>(), 0, true) {}

OsuDifficultyHitObject::OsuDifficultyHitObject(TYPE type, Vector2 pos, i32 time, i32 endTime, f32 spanDuration,
                                               u8 osuSliderCurveType, std::vector<Vector2> controlPoints,
                                               f32 pixelLength, std::vector<SLIDER_SCORING_TIME> scoringTimes,
                                               i32 repeats, bool calculateSliderCurveInConstructor) {
    this->type = type;
    this->pos = pos;
    this->time = time;
    this->endTime = endTime;
    this->spanDuration = spanDuration;
    this->osuSliderCurveType = osuSliderCurveType;
    this->pixelLength = pixelLength;
    this->scoringTimes = scoringTimes;

    this->curve = NULL;
    this->scheduledCurveAlloc = false;
    this->scheduledCurveAllocStackOffset = 0.0f;
    this->repeats = repeats;

    this->stack = 0;
    this->originalPos = this->pos;
    this->sortHack = sortHackCounter++;

    // build slider curve, if this is a (valid) slider
    if(this->type == TYPE::SLIDER && controlPoints.size() > 1) {
        if(calculateSliderCurveInConstructor) {
            // old: too much kept memory allocations for over 14000 sliders in
            // https://osu.ppy.sh/beatmapsets/592138#osu/1277649

            // NOTE: this is still used for beatmaps with less than 5000 sliders (because precomputing all curves is way
            // faster, especially for star cache loader) NOTE: the 5000 slider threshold was chosen by looking at the
            // longest non-aspire marathon maps NOTE: 15000 slider curves use ~1.6 GB of RAM, for 32-bit executables
            // with 2GB cap limiting to 5000 sliders gives ~530 MB which should be survivable without crashing (even
            // though the heap gets fragmented to fuck)

            // 6000 sliders @ The Weather Channel - 139 Degrees (Tiggz Mumbson) [Special Weather Statement].osu
            // 3674 sliders @ Various Artists - Alternator Compilation (Monstrata) [Marathon].osu
            // 4599 sliders @ Renard - Because Maybe! (Mismagius) [- Nogard Marathon -].osu
            // 4921 sliders @ Renard - Because Maybe! (Mismagius) [- Nogard Marathon v2 -].osu
            // 14960 sliders @ pishifat - H E L L O  T H E R E (Kondou-Shinichi) [Sliders in the 69th centries].osu
            // 5208 sliders @ MillhioreF - haitai but every hai adds another haitai in the background (Chewy-san)
            // [Weriko Rank the dream (nerf) but loli].osu

            this->curve = SliderCurve::createCurve(this->osuSliderCurveType, controlPoints, this->pixelLength,
                                                   osu_stars_slider_curve_points_separation.getFloat());
        } else {
            // new: delay curve creation to when it's needed, and also immediately delete afterwards (at the cost of
            // having to store a copy of the control points)
            this->scheduledCurveAlloc = true;
            this->scheduledCurveAllocControlPoints = controlPoints;
        }
    }
}

OsuDifficultyHitObject::~OsuDifficultyHitObject() { SAFE_DELETE(curve); }

OsuDifficultyHitObject::OsuDifficultyHitObject(OsuDifficultyHitObject &&dobj) {
    // move
    this->type = dobj.type;
    this->pos = dobj.pos;
    this->time = dobj.time;
    this->endTime = dobj.endTime;
    this->spanDuration = dobj.spanDuration;
    this->osuSliderCurveType = dobj.osuSliderCurveType;
    this->pixelLength = dobj.pixelLength;
    this->scoringTimes = std::move(dobj.scoringTimes);

    this->curve = dobj.curve;
    this->scheduledCurveAlloc = dobj.scheduledCurveAlloc;
    this->scheduledCurveAllocControlPoints = std::move(dobj.scheduledCurveAllocControlPoints);
    this->scheduledCurveAllocStackOffset = dobj.scheduledCurveAllocStackOffset;
    this->repeats = dobj.repeats;

    this->stack = dobj.stack;
    this->originalPos = dobj.originalPos;
    this->sortHack = dobj.sortHack;

    // reset source
    dobj.curve = NULL;
    dobj.scheduledCurveAlloc = false;
}

OsuDifficultyHitObject &OsuDifficultyHitObject::operator=(OsuDifficultyHitObject &&dobj) {
    // move
    this->type = dobj.type;
    this->pos = dobj.pos;
    this->time = dobj.time;
    this->endTime = dobj.endTime;
    this->spanDuration = dobj.spanDuration;
    this->osuSliderCurveType = dobj.osuSliderCurveType;
    this->pixelLength = dobj.pixelLength;
    this->scoringTimes = std::move(dobj.scoringTimes);

    this->curve = dobj.curve;
    this->scheduledCurveAlloc = dobj.scheduledCurveAlloc;
    this->scheduledCurveAllocControlPoints = std::move(dobj.scheduledCurveAllocControlPoints);
    this->scheduledCurveAllocStackOffset = dobj.scheduledCurveAllocStackOffset;
    this->repeats = dobj.repeats;

    this->stack = dobj.stack;
    this->originalPos = dobj.originalPos;
    this->sortHack = dobj.sortHack;

    // reset source
    dobj.curve = NULL;
    dobj.scheduledCurveAlloc = false;

    return *this;
}

void OsuDifficultyHitObject::updateStackPosition(f32 stackOffset) {
    scheduledCurveAllocStackOffset = stackOffset;

    pos = originalPos - Vector2(stack * stackOffset, stack * stackOffset);

    updateCurveStackPosition(stackOffset);
}

void OsuDifficultyHitObject::updateCurveStackPosition(f32 stackOffset) {
    if(curve != NULL) curve->updateStackPosition(stack * stackOffset, false);
}

Vector2 OsuDifficultyHitObject::getOriginalRawPosAt(i32 pos) {
    // NOTE: the delayed curve creation has been deliberately disabled here for stacking purposes for beatmaps with
    // insane slider counts for performance reasons NOTE: this means that these aspire maps will have incorrect stars
    // due to incorrect slider stacking, but the delta is below 0.02 even for the most insane maps which currently exist
    // NOTE: if this were to ever get implemented properly, then a sliding window for curve allocation must be added to
    // the stack algorithm itself (as doing it in here is O(n!) madness) NOTE: to validate the delta, use Acid Rain
    // [Aspire] - Karoo13 (6.76* with slider stacks -> 6.75* without slider stacks)

    if(type != TYPE::SLIDER || (curve == NULL)) {
        return originalPos;
    } else {
        if(pos <= time)
            return curve->originalPointAt(0.0f);
        else if(pos >= endTime) {
            if(repeats % 2 == 0) {
                return curve->originalPointAt(0.0f);
            } else {
                return curve->originalPointAt(1.0f);
            }
        } else {
            return curve->originalPointAt(getT(pos, false));
        }
    }
}

f32 OsuDifficultyHitObject::getT(i32 pos, bool raw) {
    f32 t = (f32)((i32)pos - (i32)time) / spanDuration;
    if(raw)
        return t;
    else {
        f32 floorVal = (f32)std::floor(t);
        return ((i32)floorVal % 2 == 0) ? t - floorVal : floorVal + 1 - t;
    }
}

ConVar *DifficultyCalculator::m_osu_slider_scorev2_ref = NULL;

f64 DifficultyCalculator::calculateStarDiffForHitObjects(std::vector<OsuDifficultyHitObject> &sortedHitObjects, f32 CS,
                                                         f32 OD, f32 speedMultiplier, bool relax, bool touchDevice,
                                                         f64 *aim, f64 *aimSliderFactor, f64 *speed, f64 *speedNotes,
                                                         i32 upToObjectIndex, std::vector<f64> *outAimStrains,
                                                         std::vector<f64> *outSpeedStrains) {
    std::atomic<bool> dead;
    dead = false;
    return calculateStarDiffForHitObjects(sortedHitObjects, CS, OD, speedMultiplier, relax, touchDevice, aim,
                                          aimSliderFactor, speed, speedNotes, upToObjectIndex, outAimStrains,
                                          outSpeedStrains, dead);
}

f64 DifficultyCalculator::calculateStarDiffForHitObjects(std::vector<OsuDifficultyHitObject> &sortedHitObjects, f32 CS,
                                                         f32 OD, f32 speedMultiplier, bool relax, bool touchDevice,
                                                         f64 *aim, f64 *aimSliderFactor, f64 *speed, f64 *speedNotes,
                                                         i32 upToObjectIndex, std::vector<f64> *outAimStrains,
                                                         std::vector<f64> *outSpeedStrains,
                                                         const std::atomic<bool> &dead) {
    // NOTE: depends on speed multiplier + CS + OD + relax + touchDevice

    // NOTE: upToObjectIndex is applied way below, during the construction of the 'dobjects'

    // NOTE: osu always returns 0 stars for beatmaps with only 1 object, except if that object is a slider
    if(sortedHitObjects.size() < 2) {
        if(sortedHitObjects.size() < 1) return 0.0;
        if(sortedHitObjects[0].type != OsuDifficultyHitObject::TYPE::SLIDER) return 0.0;
    }

    // global independent variables/constants
    const bool applyBrokenGamefieldRoundingAllowance =
        false;  // NOTE: un-compensate because lazer doesn't use this (and lazer code is used for calculating global
                // online ranking pp/stars now)
    f32 circleRadiusInOsuPixels =
        GameRules::getRawHitCircleDiameter(clamp<float>(CS, 0.0f, 12.142f), applyBrokenGamefieldRoundingAllowance) /
        2.0f;  // NOTE: clamped CS because neosu allows CS > ~12.1429 (at which poi32 the diameter becomes negative)
    const f32 hitWindow300 = 2.0f * GameRules::getRawHitWindow300(OD) / speedMultiplier;

    // ******************************************************************************************************************************************
    // //

    // see setDistances() @
    // https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Preprocessing/OsuDifficultyHitObject.cs

    static const f32 normalized_radius = 50.0f;  // normalization factor
    static const f32 maximum_slider_radius = normalized_radius * 2.4f;
    static const f32 assumed_slider_radius = normalized_radius * 1.8f;
    static const f32 circlesize_buff_treshold = 30;  // non-normalized diameter where the circlesize buff starts

    // multiplier to normalize positions so that we can calc as if everything was the same circlesize.
    // also handle high CS bonus

    f32 radius_scaling_factor = normalized_radius / circleRadiusInOsuPixels;
    if(circleRadiusInOsuPixels < circlesize_buff_treshold) {
        const f32 smallCircleBonus = min(circlesize_buff_treshold - circleRadiusInOsuPixels, 5.0f) / 50.0f;
        radius_scaling_factor *= 1.0f + smallCircleBonus;
    }

    static const i32 NUM_SKILLS = 3;

    class Skills {
       public:
        enum class Skill {
            SPEED,
            AIM_SLIDERS,
            AIM_NO_SLIDERS,
        };

        static i32 skillToIndex(const Skill skill) {
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
    static const f64 decay_base[NUM_SKILLS] = {0.3, 0.15, 0.15};

    // used to keep speed and aim balanced between eachother
    static const f64 weight_scaling[NUM_SKILLS] = {1375.0, 23.55, 23.55};

    class DiffObject {
       public:
        OsuDifficultyHitObject *ho;

        f64 strains[NUM_SKILLS];

        // https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Speed.cs
        // needed because raw speed strain and rhythm strain are combined in different ways
        f64 raw_speed_strain;
        f64 rhythm;

        Vector2 norm_start;  // start position normalized on radius

        f64 angle;  // precalc

        f64 jumpDistance;     // precalc
        f64 minJumpDistance;  // precalc
        f64 minJumpTime;      // precalc
        f64 travelDistance;   // precalc

        f64 delta_time;   // strain temp
        f64 strain_time;  // strain temp

        bool lazyCalcFinished;  // precalc temp
        Vector2 lazyEndPos;     // precalc temp
        f64 lazyTravelDist;     // precalc temp
        f64 lazyTravelTime;     // precalc temp
        f64 travelTime;         // precalc temp

        // NOTE: neosu stores the first object in this array while lazer doesn't. newer lazer algorithms
        // require referencing objects "randomly", so we just keep the entire vector around.
        const std::vector<DiffObject> &objects;

        // WARNING: this will be -1 for the first object (as the name implies), see note above
        i32 prevObjectIndex;

        DiffObject(OsuDifficultyHitObject *base_object, f32 radius_scaling_factor,
                   std::vector<DiffObject> &diff_objects, i32 prevObjectIdx)
            : objects(diff_objects) {
            ho = base_object;

            for(i32 i = 0; i < NUM_SKILLS; i++) {
                strains[i] = 0.0;
            }
            raw_speed_strain = 0.0;
            rhythm = 0.0;

            norm_start = ho->pos * radius_scaling_factor;

            angle = std::numeric_limits<float>::quiet_NaN();

            jumpDistance = 0.0;
            minJumpDistance = 0.0;
            minJumpTime = 0.0;
            travelDistance = 0.0;

            delta_time = 0.0;
            strain_time = 0.0;

            lazyCalcFinished = false;
            lazyEndPos = ho->pos;
            lazyTravelDist = 0.0;
            lazyTravelTime = 0.0;
            travelTime = 0.0;

            prevObjectIndex = prevObjectIdx;
        }

        inline const DiffObject *get_previous(i32 backwardsIdx) const {
            return (objects.size() > 0 && prevObjectIndex - backwardsIdx < (i32)objects.size()
                        ? &objects[max(0, prevObjectIndex - backwardsIdx)]
                        : NULL);
        }

        void calculate_strains(const DiffObject &prev, const DiffObject *next, f64 hitWindow300) {
            calculate_strain(prev, next, hitWindow300, Skills::Skill::SPEED);
            calculate_strain(prev, next, hitWindow300, Skills::Skill::AIM_SLIDERS);
            calculate_strain(prev, next, hitWindow300, Skills::Skill::AIM_NO_SLIDERS);
        }

        void calculate_strain(const DiffObject &prev, const DiffObject *next, f64 hitWindow300,
                              const Skills::Skill dtype) {
            f64 currentStrainOfDiffObject = 0;

            const i32 time_elapsed = ho->time - prev.ho->time;

            // update our delta time
            delta_time = (f64)time_elapsed;
            strain_time = (f64)max(time_elapsed, 25);

            switch(ho->type) {
                case OsuDifficultyHitObject::TYPE::SLIDER:
                case OsuDifficultyHitObject::TYPE::CIRCLE:
                    currentStrainOfDiffObject = spacing_weight2(dtype, prev, next, hitWindow300);
                    break;

                case OsuDifficultyHitObject::TYPE::SPINNER:
                    break;

                case OsuDifficultyHitObject::TYPE::INVALID:
                    // NOTE: silently ignore
                    return;
            }

            // see Process() @ https://github.com/ppy/osu/blob/master/osu.Game/Rulesets/Difficulty/Skills/Skill.cs
            f64 currentStrain = prev.strains[Skills::skillToIndex(dtype)];
            {
                currentStrain *= strainDecay(dtype, dtype == Skills::Skill::SPEED ? strain_time : delta_time);
                currentStrain += currentStrainOfDiffObject * weight_scaling[Skills::skillToIndex(dtype)];
            }
            strains[Skills::skillToIndex(dtype)] = currentStrain;
        }

        // see https://github.com/ppy/osu/blob/master/osu.Game/Rulesets/Difficulty/Skills/StrainSkill.cs
        static f64 calculate_difficulty(const Skills::Skill type, const std::vector<DiffObject> &dobjects,
                                        std::vector<f64> *outStrains = NULL, f64 *outRelevantNotes = NULL) {
            static const f64 strain_step = 400.0;  // the length of each strain section
            static const f64 decay_weight =
                0.9;  // max strains are weighted from highest to lowest, and this is how much the weight decays.

            if(dobjects.size() < 1) return 0.0;

            f64 interval_end = std::ceil((f64)dobjects[0].ho->time / strain_step) * strain_step;
            f64 max_strain = 0.0;

            // used for calculating relevant note count for speed pp
            std::vector<f64> objectStrains;

            std::vector<f64> highestStrains;
            for(size_t i = 0; i < dobjects.size(); i++) {
                const DiffObject &cur = dobjects[i];
                const DiffObject &prev = dobjects[i > 0 ? i - 1 : i];

                // make previous peak strain decay until the current object
                while(cur.ho->time > interval_end) {
                    highestStrains.push_back(max_strain);

                    if(i < 1)  // !prev
                        max_strain = 0.0;
                    else
                        max_strain = prev.strains[Skills::skillToIndex(type)] *
                                     (type == Skills::Skill::SPEED ? prev.rhythm : 1.0) *
                                     strainDecay(type, (interval_end - (f64)prev.ho->time));

                    interval_end += strain_step;
                }

                // calculate max strain for this interval
                f64 cur_strain =
                    cur.strains[Skills::skillToIndex(type)] * (type == Skills::Skill::SPEED ? cur.rhythm : 1.0);
                max_strain = max(max_strain, cur_strain);
                if(outRelevantNotes) objectStrains.push_back(cur_strain);
            }

            // the peak strain will not be saved for the last section in the above loop
            highestStrains.push_back(max_strain);

            if(outStrains != NULL) (*outStrains) = highestStrains;  // save a copy

            // calculate relevant speed note count
            // RelevantNoteCount @
            // https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Speed.cs
            if(outRelevantNotes) {
                f64 maxStrain =
                    (objectStrains.size() < 1 ? 0.0 : *max_element(objectStrains.begin(), objectStrains.end()));
                if(objectStrains.size() < 1 || maxStrain == 0.0)
                    *outRelevantNotes = 0.0;
                else {
                    f64 tempSum = 0.0;
                    for(size_t i = 0; i < objectStrains.size(); i++) {
                        tempSum += 1.0 / (1.0 + std::exp(-(objectStrains[i] / maxStrain * 12.0 - 6.0)));
                    }
                    *outRelevantNotes = tempSum;
                }
            }

            // see DifficultyValue() @
            // https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/OsuStrainSkill.cs

            static const size_t reducedSectionCount = 10;
            static const f64 reducedStrainBaseline = 0.75;
            static const f64 difficultyMultiplier = 1.06;

            f64 difficulty = 0.0;
            f64 weight = 1.0;

            // sort strains from greatest to lowest
            std::sort(highestStrains.begin(), highestStrains.end(), std::greater<f64>());

            // https://github.com/ppy/osu/pull/13483/
            {
                size_t skillSpecificReducedSectionCount = reducedSectionCount;
                {
                    switch(type) {
                        case Skills::Skill::SPEED:
                            skillSpecificReducedSectionCount = 5;
                            break;
                        case Skills::Skill::AIM_SLIDERS:
                        case Skills::Skill::AIM_NO_SLIDERS:
                            break;
                    }
                }

                // "We are reducing the highest strains first to account for extreme difficulty spikes"
                for(size_t i = 0; i < min(highestStrains.size(), skillSpecificReducedSectionCount); i++) {
                    const f64 scale = std::log10(
                        lerp<f64>(1.0, 10.0, clamp<f64>((f64)i / (f64)skillSpecificReducedSectionCount, 0.0, 1.0)));
                    highestStrains[i] *= lerp<f64>(reducedStrainBaseline, 1.0, scale);
                }

                // re-sort
                std::sort(highestStrains.begin(), highestStrains.end(), std::greater<f64>());

                // weigh the top strains
                for(size_t i = 0; i < highestStrains.size(); i++) {
                    difficulty += highestStrains[i] * weight;
                    weight *= decay_weight;
                }
            }

            f64 skillSpecificDifficultyMultiplier = difficultyMultiplier;
            {
                switch(type) {
                    case Skills::Skill::SPEED:
                        skillSpecificDifficultyMultiplier = 1.04;
                        break;
                    case Skills::Skill::AIM_SLIDERS:
                    case Skills::Skill::AIM_NO_SLIDERS:
                        break;
                }
            }

            return difficulty * skillSpecificDifficultyMultiplier;
        }

        // https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/
        f64 spacing_weight2(const Skills::Skill diff_type, const DiffObject &prev, const DiffObject *next,
                            f64 hitWindow300) {
            static const f64 single_spacing_threshold = 125.0;

            static const f64 min_speed_bonus = 75.0; /* ~200BPM 1/4 streams */
            static const f64 speed_balancing_factor = 40.0;

            static const i32 history_time_max = 5000;
            static const f64 rhythm_multiplier = 0.75;

            // f64 angle_bonus = 1.0; // (apparently unused now in lazer?)

            switch(diff_type) {
                case Skills::Skill::SPEED: {
                    // see https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Skills/Speed.cs
                    if(ho->type == OsuDifficultyHitObject::TYPE::SPINNER) {
                        raw_speed_strain = 0.0;
                        rhythm = 0.0;

                        return 0.0;
                    }

                    // https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Evaluators/SpeedEvaluator.cs
                    const f64 distance = min(single_spacing_threshold, prev.travelDistance + minJumpDistance);

                    f64 strain_time = this->strain_time;
                    strain_time /= clamp<f64>((strain_time / hitWindow300) / 0.93, 0.92, 1.0);

                    f64 doubletapness = 1.0;
                    if(next != NULL) {
                        f64 cur_delta = max(1.0, delta_time);
                        f64 next_delta = max(1, next->ho->time - ho->time);  // next delta time isn't initialized yet
                        f64 delta_diff = std::abs(next_delta - cur_delta);
                        f64 speedRatio = cur_delta / max(cur_delta, delta_diff);
                        f64 windowRatio = pow(min(1.0, cur_delta / hitWindow300), 2.0);

                        doubletapness = pow(speedRatio, 1 - windowRatio);
                    }

                    f64 speed_bonus = 1.0;
                    if(strain_time < min_speed_bonus)
                        speed_bonus = 1.0 + 0.75 * pow((min_speed_bonus - strain_time) / speed_balancing_factor, 2.0);

                    raw_speed_strain = (speed_bonus + speed_bonus * pow(distance / single_spacing_threshold, 3.5)) *
                                       doubletapness / strain_time;

                    // https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Evaluators/RhythmEvaluator.cs
                    i32 previousIslandSize = 0;

                    f64 rhythmComplexitySum = 0;
                    i32 islandSize = 1;
                    f64 startRatio =
                        0;  // store the ratio of the current start of an island to buff for tighter rhythms

                    bool firstDeltaSwitch = false;

                    i32 historicalNoteCount = min(prevObjectIndex, 32);

                    i32 rhythmStart = 0;

                    while(rhythmStart < historicalNoteCount - 2 &&
                          ho->time - get_previous(rhythmStart)->ho->time < history_time_max) {
                        rhythmStart++;
                    }

                    for(i32 i = rhythmStart; i > 0; i--) {
                        const DiffObject *currObj = get_previous(i - 1);
                        const DiffObject *prevObj = get_previous(i);
                        const DiffObject *lastObj = get_previous(i + 1);

                        f64 currHistoricalDecay = (f64)(history_time_max - (ho->time - currObj->ho->time)) /
                                                  history_time_max;  // scales note 0 to 1 from history to now

                        currHistoricalDecay =
                            min((f64)(historicalNoteCount - i) / historicalNoteCount,
                                currHistoricalDecay);  // either we're limited by time or limited by object count.

                        f64 currDelta = currObj->strain_time;
                        f64 prevDelta = prevObj->strain_time;
                        f64 lastDelta = lastObj->strain_time;
                        f64 currRatio =
                            1.0 +
                            6.0 * min(0.5, pow(std::sin(PI / (min(prevDelta, currDelta) / max(prevDelta, currDelta))),
                                               2.0));  // fancy function to calculate rhythmbonuses.

                        f64 windowPenalty = min(
                            1.0, max(0.0, std::abs(prevDelta - currDelta) - hitWindow300 * 0.3) / (hitWindow300 * 0.3));

                        windowPenalty = min(1.0, windowPenalty);

                        f64 effectiveRatio = windowPenalty * currRatio;

                        if(firstDeltaSwitch) {
                            if(!(prevDelta > 1.25 * currDelta || prevDelta * 1.25 < currDelta)) {
                                if(islandSize < 7) islandSize++;  // island is still progressing, count size.
                            } else {
                                if(get_previous(i - 1)->ho->type ==
                                   OsuDifficultyHitObject::TYPE::SLIDER)  // bpm change is into slider, this is easy acc
                                                                          // window
                                    effectiveRatio *= 0.125;

                                if(get_previous(i)->ho->type ==
                                   OsuDifficultyHitObject::TYPE::SLIDER)  // bpm change was from a slider, this is
                                                                          // easier typically than circle -> circle
                                    effectiveRatio *= 0.25;

                                if(previousIslandSize == islandSize)  // repeated island size (ex: triplet -> triplet)
                                    effectiveRatio *= 0.25;

                                if(previousIslandSize % 2 ==
                                   islandSize % 2)  // repeated island polartiy (2 -> 4, 3 -> 5)
                                    effectiveRatio *= 0.50;

                                if(lastDelta > prevDelta + 10.0 &&
                                   prevDelta > currDelta + 10.0)  // previous increase happened a note ago,
                                                                  // 1/1->1/2-1/4, dont want to buff this.
                                    effectiveRatio *= 0.125;

                                rhythmComplexitySum += std::sqrt(effectiveRatio * startRatio) * currHistoricalDecay *
                                                       std::sqrt(4.0 + islandSize) / 2.0 *
                                                       std::sqrt(4.0 + previousIslandSize) / 2.0;

                                startRatio = effectiveRatio;

                                previousIslandSize = islandSize;  // log the last island size.

                                if(prevDelta * 1.25 < currDelta)  // we're slowing down, stop counting
                                    firstDeltaSwitch = false;     // if we're speeding up, this stays true and  we keep
                                                                  // counting island size.

                                islandSize = 1;
                            }
                        } else if(prevDelta > 1.25 * currDelta)  // we want to be speeding up.
                        {
                            // Begin counting island until we change speed again.
                            firstDeltaSwitch = true;
                            startRatio = effectiveRatio;
                            islandSize = 1;
                        }
                    }

                    rhythm = std::sqrt(4.0 + rhythmComplexitySum * rhythm_multiplier) / 2.0;

                    return raw_speed_strain;
                } break;

                case Skills::Skill::AIM_SLIDERS:
                case Skills::Skill::AIM_NO_SLIDERS: {
                    // https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Evaluators/AimEvaluator.cs
                    static const f64 wide_angle_multiplier = 1.5;
                    static const f64 acute_angle_multiplier = 1.95;
                    static const f64 slider_multiplier = 1.35;
                    static const f64 velocity_change_multiplier = 0.75;

                    const bool withSliders = (diff_type == Skills::Skill::AIM_SLIDERS);

                    if(ho->type == OsuDifficultyHitObject::TYPE::SPINNER || prevObjectIndex <= 1 ||
                       prev.ho->type == OsuDifficultyHitObject::TYPE::SPINNER)
                        return 0.0;

                    auto calcWideAngleBonus = [](f64 angle) {
                        return pow(std::sin(3.0 / 4.0 * (min(5.0 / 6.0 * PI, max(PI / 6.0, angle)) - PI / 6.0)), 2.0);
                    };
                    auto calcAcuteAngleBonus = [=](f64 angle) { return 1.0 - calcWideAngleBonus(angle); };

                    const DiffObject *prevPrev = get_previous(1);
                    f64 currVelocity = jumpDistance / strain_time;

                    if(prev.ho->type == OsuDifficultyHitObject::TYPE::SLIDER && withSliders) {
                        f64 travelVelocity = prev.travelDistance / prev.travelTime;
                        f64 movementVelocity = minJumpDistance / minJumpTime;
                        currVelocity = max(currVelocity, movementVelocity + travelVelocity);
                    }
                    f64 aimStrain = currVelocity;

                    f64 prevVelocity = prev.jumpDistance / prev.strain_time;
                    if(prevPrev->ho->type == OsuDifficultyHitObject::TYPE::SLIDER && withSliders) {
                        f64 travelVelocity = prevPrev->travelDistance / prevPrev->travelTime;
                        f64 movementVelocity = prev.minJumpDistance / prev.minJumpTime;
                        prevVelocity = max(prevVelocity, movementVelocity + travelVelocity);
                    }

                    f64 wideAngleBonus = 0;
                    f64 acuteAngleBonus = 0;
                    f64 sliderBonus = 0;
                    f64 velocityChangeBonus = 0;

                    if(max(strain_time, prev.strain_time) < 1.25 * min(strain_time, prev.strain_time)) {
                        if(!std::isnan(angle) && !std::isnan(prev.angle) && !std::isnan(prevPrev->angle)) {
                            f64 angleBonus = min(currVelocity, prevVelocity);

                            wideAngleBonus = calcWideAngleBonus(angle);
                            acuteAngleBonus =
                                strain_time > 100
                                    ? 0.0
                                    : (calcAcuteAngleBonus(angle) * calcAcuteAngleBonus(prev.angle) *
                                       min(angleBonus, 125.0 / strain_time) *
                                       pow(std::sin(PI / 2.0 * min(1.0, (100.0 - strain_time) / 25.0)), 2.0) *
                                       pow(std::sin(PI / 2.0 * (clamp<f64>(jumpDistance, 50.0, 100.0) - 50.0) / 50.0),
                                           2.0));

                            wideAngleBonus *=
                                angleBonus * (1.0 - min(wideAngleBonus, pow(calcWideAngleBonus(prev.angle), 3.0)));
                            acuteAngleBonus *= 0.5 + 0.5 * (1.0 - min(acuteAngleBonus,
                                                                      pow(calcAcuteAngleBonus(prevPrev->angle), 3.0)));
                        }
                    }

                    if(max(prevVelocity, currVelocity) != 0.0) {
                        prevVelocity = (prev.jumpDistance + prevPrev->travelDistance) / prev.strain_time;
                        currVelocity = (jumpDistance + prev.travelDistance) / strain_time;

                        f64 distRatio = pow(std::sin(PI / 2.0 * std::abs(prevVelocity - currVelocity) /
                                                     max(prevVelocity, currVelocity)),
                                            2.0);
                        f64 overlapVelocityBuff =
                            min(125.0 / min(strain_time, prev.strain_time), std::abs(prevVelocity - currVelocity));
                        velocityChangeBonus =
                            overlapVelocityBuff * distRatio *
                            pow(min(strain_time, prev.strain_time) / max(strain_time, prev.strain_time), 2.0);
                    }

                    if(prev.ho->type == OsuDifficultyHitObject::TYPE::SLIDER)
                        sliderBonus = prev.travelDistance / prev.travelTime;

                    aimStrain +=
                        max(acuteAngleBonus * acute_angle_multiplier,
                            wideAngleBonus * wide_angle_multiplier + velocityChangeBonus * velocity_change_multiplier);
                    if(withSliders) aimStrain += sliderBonus * slider_multiplier;

                    return aimStrain;
                } break;
            }

            return 0.0;
        }

        inline static f64 applyDiminishingExp(f64 val) { return pow(val, 0.99); }

        inline static f64 strainDecay(Skills::Skill type, f64 ms) {
            return pow(decay_base[Skills::skillToIndex(type)], ms / 1000.0);
        }
    };

    // ******************************************************************************************************************************************
    // //

    // see
    // https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Preprocessing/OsuDifficultyHitObject.cs

    class DistanceCalc {
       public:
        static void computeSliderCursorPosition(DiffObject &slider, f32 circleRadius) {
            if(slider.lazyCalcFinished || slider.ho->curve == NULL) return;

            slider.lazyTravelTime = slider.ho->scoringTimes.back().time - slider.ho->time;

            f64 endTimeMin = slider.lazyTravelTime / slider.ho->spanDuration;
            if(std::fmod(endTimeMin, 2.0) >= 1.0)
                endTimeMin = 1.0 - std::fmod(endTimeMin, 1.0);
            else
                endTimeMin = std::fmod(endTimeMin, 1.0);

            slider.lazyEndPos = slider.ho->curve->pointAt(endTimeMin);

            Vector2 cursor_pos = slider.ho->pos;
            f64 scaling_factor = 50.0 / circleRadius;

            for(size_t i = 0; i < slider.ho->scoringTimes.size(); i++) {
                Vector2 diff;

                if(slider.ho->scoringTimes[i].type == OsuDifficultyHitObject::SLIDER_SCORING_TIME::TYPE::END) {
                    // NOTE: In lazer, the position of the slider end is at the visual end, but the time is at the
                    // scoring end
                    diff = slider.ho->curve->pointAt(slider.ho->repeats % 2 ? 1.0 : 0.0) - cursor_pos;
                } else {
                    f64 progress =
                        (clamp<long>(slider.ho->scoringTimes[i].time - slider.ho->time, 0, slider.ho->getDuration())) /
                        slider.ho->spanDuration;
                    if(std::fmod(progress, 2.0) >= 1.0)
                        progress = 1.0 - std::fmod(progress, 1.0);
                    else
                        progress = std::fmod(progress, 1.0);

                    diff = slider.ho->curve->pointAt(progress) - cursor_pos;
                }

                f64 diff_len = scaling_factor * diff.length();

                f64 req_diff = 90.0;

                if(i == slider.ho->scoringTimes.size() - 1) {
                    // Slider end
                    Vector2 lazy_diff = slider.lazyEndPos - cursor_pos;
                    if(lazy_diff.length() < diff.length()) diff = lazy_diff;
                    diff_len = scaling_factor * diff.length();
                } else if(slider.ho->scoringTimes[i].type ==
                          OsuDifficultyHitObject::SLIDER_SCORING_TIME::TYPE::REPEAT) {
                    // Slider repeat
                    req_diff = 50.0;
                }

                if(diff_len > req_diff) {
                    cursor_pos += (diff * (f32)((diff_len - req_diff) / diff_len));
                    diff_len *= (diff_len - req_diff) / diff_len;
                    slider.lazyTravelDist += diff_len;
                }

                if(i == slider.ho->scoringTimes.size() - 1) slider.lazyEndPos = cursor_pos;
            }

            slider.lazyCalcFinished = true;
        }

        static Vector2 getEndCursorPosition(DiffObject &hitObject, f32 circleRadius) {
            if(hitObject.ho->type == OsuDifficultyHitObject::TYPE::SLIDER) {
                computeSliderCursorPosition(hitObject, circleRadius);
                return hitObject
                    .lazyEndPos;  // (slider.lazyEndPos is already initialized to ho->pos in DiffObject constructor)
            }

            return hitObject.ho->pos;
        }
    };

    // ******************************************************************************************************************************************
    // //

    // initialize dobjects
    std::vector<DiffObject> diffObjects;
    diffObjects.reserve((upToObjectIndex < 0) ? sortedHitObjects.size() : upToObjectIndex + 1);
    for(size_t i = 0; i < sortedHitObjects.size() && (upToObjectIndex < 0 || i < upToObjectIndex + 1);
        i++)  // respect upToObjectIndex!
    {
        if(dead.load()) return 0.0;

        diffObjects.push_back(DiffObject(&sortedHitObjects[i], radius_scaling_factor, diffObjects,
                                         (i32)i - 1));  // this already initializes the angle to NaN
    }

    const size_t numDiffObjects = diffObjects.size();

    // calculate angles and travel/jump distances (before calculating strains)
    const f32 starsSliderCurvePointsSeparation = osu_stars_slider_curve_points_separation.getFloat();
    for(size_t i = 0; i < numDiffObjects; i++) {
        if(dead.load()) return 0.0;

        // see setDistances() @
        // https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Preprocessing/OsuDifficultyHitObject.cs

        if(i > 0) {
            // calculate travel/jump distances
            DiffObject &cur = diffObjects[i];
            DiffObject &prev1 = diffObjects[i - 1];

            // MCKAY:
            {
                // delay curve creation to when it's needed (1)
                if(prev1.ho->scheduledCurveAlloc && prev1.ho->curve == NULL) {
                    prev1.ho->curve = SliderCurve::createCurve(prev1.ho->osuSliderCurveType,
                                                               prev1.ho->scheduledCurveAllocControlPoints,
                                                               prev1.ho->pixelLength, starsSliderCurvePointsSeparation);
                    prev1.ho->updateCurveStackPosition(
                        prev1.ho->scheduledCurveAllocStackOffset);  // NOTE: respect stacking
                }
            }

            if(cur.ho->type == OsuDifficultyHitObject::TYPE::SLIDER) {
                DistanceCalc::computeSliderCursorPosition(cur, circleRadiusInOsuPixels);
                cur.travelDistance = cur.lazyTravelDist * pow(1.0 + (cur.ho->repeats - 1) / 2.5, 1.0 / 2.5);
                cur.travelTime = max(cur.lazyTravelTime, 25.0);
            }

            // don't need to jump to reach spinners
            if(cur.ho->type == OsuDifficultyHitObject::TYPE::SPINNER ||
               prev1.ho->type == OsuDifficultyHitObject::TYPE::SPINNER)
                continue;

            const Vector2 lastCursorPosition = DistanceCalc::getEndCursorPosition(prev1, circleRadiusInOsuPixels);

            f64 cur_strain_time = (f64)max(cur.ho->time - prev1.ho->time, 25);  // strain_time isn't initialized here
            cur.jumpDistance = (cur.norm_start - lastCursorPosition * radius_scaling_factor).length();
            cur.minJumpDistance = cur.jumpDistance;
            cur.minJumpTime = cur_strain_time;

            if(prev1.ho->type == OsuDifficultyHitObject::TYPE::SLIDER) {
                f64 last_travel = max(prev1.lazyTravelTime, 25.0);
                cur.minJumpTime = max(cur_strain_time - last_travel, 25.0);

                // NOTE: "curve shouldn't be null here, but Yin [test7] causes that to happen"
                // NOTE: the curve can be null if controlPoints.size() < 1 because the OsuDifficultyHitObject()
                // constructor will then not set scheduledCurveAlloc to true (which is perfectly fine and correct)
                f32 tail_jump_dist =
                    (prev1.ho->curve ? prev1.ho->curve->pointAt(prev1.ho->repeats % 2 ? 1.0 : 0.0) : prev1.ho->pos)
                        .distance(cur.ho->pos) *
                    radius_scaling_factor;
                cur.minJumpDistance =
                    max(0.0f, min((f32)cur.minJumpDistance - (maximum_slider_radius - assumed_slider_radius),
                                  tail_jump_dist - maximum_slider_radius));
            }

            // calculate angles
            if(i > 1) {
                DiffObject &prev2 = diffObjects[i - 2];
                if(prev2.ho->type == OsuDifficultyHitObject::TYPE::SPINNER) continue;

                const Vector2 lastLastCursorPosition =
                    DistanceCalc::getEndCursorPosition(prev2, circleRadiusInOsuPixels);

                // MCKAY:
                {
                    // and also immediately delete afterwards (2)
                    if(i > 2)  // NOTE: this trivial sliding window implementation will keep the last 2 curves alive
                               // at the end, but they get auto deleted later anyway so w/e
                    {
                        DiffObject &prev3 = diffObjects[i - 3];

                        if(prev3.ho->scheduledCurveAlloc) SAFE_DELETE(prev3.ho->curve);
                    }
                }

                const Vector2 v1 = lastLastCursorPosition - prev1.ho->pos;
                const Vector2 v2 = cur.ho->pos - lastCursorPosition;

                const f64 dot = v1.dot(v2);
                const f64 det = (v1.x * v2.y) - (v1.y * v2.x);

                cur.angle = std::fabs(std::atan2(det, dot));
            }
        }
    }

    // calculate strains/skills
    for(size_t i = 1; i < numDiffObjects; i++)  // NOTE: start at 1
    {
        diffObjects[i].calculate_strains(diffObjects[i - 1], (i == numDiffObjects - 1) ? NULL : &diffObjects[i + 1],
                                         hitWindow300);
    }

    // calculate final difficulty (weigh strains)
    f64 aimNoSliders = DiffObject::calculate_difficulty(Skills::Skill::AIM_NO_SLIDERS, diffObjects);
    *aim = DiffObject::calculate_difficulty(Skills::Skill::AIM_SLIDERS, diffObjects, outAimStrains);
    *speed = DiffObject::calculate_difficulty(Skills::Skill::SPEED, diffObjects, outSpeedStrains, speedNotes);

    static const f64 star_scaling_factor = 0.0675;

    aimNoSliders = std::sqrt(aimNoSliders) * star_scaling_factor;
    *aim = std::sqrt(*aim) * star_scaling_factor;
    *speed = std::sqrt(*speed) * star_scaling_factor;

    *aimSliderFactor = (*aim > 0) ? aimNoSliders / *aim : 1.0;

    if(touchDevice) *aim = pow(*aim, 0.8);

    if(relax) {
        *aim *= 0.9;
        *speed = 0.0;
    }

    f64 baseAimPerformance = pow(5.0 * max(1.0, *aim / 0.0675) - 4.0, 3.0) / 100000.0;
    f64 baseSpeedPerformance = pow(5.0 * max(1.0, *speed / 0.0675) - 4.0, 3.0) / 100000.0;
    f64 basePerformance = pow(pow(baseAimPerformance, 1.1) + pow(baseSpeedPerformance, 1.1), 1.0 / 1.1);
    return basePerformance > 0.00001
               ? 1.04464392682 /* Math.Cbrt(OsuPerformanceCalculator.PERFORMANCE_BASE_MULTIPLIER) */ * 0.027 *
                     (std::cbrt(100000.0 / pow(2.0, 1 / 1.1) * basePerformance) + 4.0)
               : 0.0;
}

f64 DifficultyCalculator::calculatePPv2(Beatmap *beatmap, f64 aim, f64 aimSliderFactor, f64 speed, f64 speedNotes,
                                        i32 numHitObjects, i32 numCircles, i32 numSliders, i32 numSpinners,
                                        i32 maxPossibleCombo, i32 combo, i32 misses, i32 c300, i32 c100, i32 c50) {
    // NOTE: depends on active mods + OD + AR

    if(m_osu_slider_scorev2_ref == NULL) m_osu_slider_scorev2_ref = convar->getConVarByName("osu_slider_scorev2");

    // get runtime mods
    i32 modsLegacy = osu->getScore()->getModsLegacy();
    {
        // special case: manual slider accuracy has been enabled (affects pp but not score)
        modsLegacy |= (m_osu_slider_scorev2_ref->getBool() ? ModFlags::ScoreV2 : 0);
    }

    return calculatePPv2(modsLegacy, osu->getSpeedMultiplier(), beatmap->getAR(), beatmap->getOD(), aim,
                         aimSliderFactor, speed, speedNotes, numHitObjects, numCircles, numSliders, numSpinners,
                         maxPossibleCombo, combo, misses, c300, c100, c50);
}

f64 DifficultyCalculator::calculatePPv2(i32 modsLegacy, f64 timescale, f64 ar, f64 od, f64 aim, f64 aimSliderFactor,
                                        f64 speed, f64 speedNotes, i32 numHitObjects, i32 numCircles, i32 numSliders,
                                        i32 numSpinners, i32 maxPossibleCombo, i32 combo, i32 misses, i32 c300,
                                        i32 c100, i32 c50) {
    // NOTE: depends on active mods + OD + AR

    // apply "timescale" aka speed multiplier to ar/od
    // (the original incoming ar/od values are guaranteed to not yet have any speed multiplier applied to them, but they
    // do have non-time-related mods already applied, like HR or any custom overrides) (yes, this does work correctly
    // when the override slider "locking" feature is used. in this case, the stored ar/od is already compensated such
    // that it will have the locked value AFTER applying the speed multiplier here) (all UI elements which display ar/od
    // from stored scores, like the ranking screen or score buttons, also do this calculation before displaying the
    // values to the user. of course the mod selection screen does too.)
    od = GameRules::getRawOverallDifficultyForSpeedMultiplier(GameRules::getRawHitWindow300(od), timescale);
    ar = GameRules::getRawApproachRateForSpeedMultiplier(GameRules::getRawApproachTime(ar), timescale);

    if(c300 < 0) c300 = numHitObjects - c100 - c50 - misses;

    if(combo < 0) combo = maxPossibleCombo;

    if(combo < 1) return 0.0;

    i32 totalHits = c300 + c100 + c50 + misses;
    f64 accuracy = (totalHits > 0 ? (f64)(c300 * 300 + c100 * 100 + c50 * 50) / (f64)(totalHits * 300) : 0.0);
    i32 amountHitObjectsWithAccuracy = (modsLegacy & ModFlags::ScoreV2 ? totalHits : numCircles);

    // calculateEffectiveMissCount @
    // https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/OsuPerformanceCalculator.cs required
    // because slider breaks aren't exposed to pp calculation
    f64 comboBasedMissCount = 0.0;
    if(numSliders > 0) {
        f64 fullComboThreshold = maxPossibleCombo - (0.1 * numSliders);
        if(maxPossibleCombo < fullComboThreshold)
            comboBasedMissCount = fullComboThreshold / max(1.0, (f64)maxPossibleCombo);
    }
    f64 effectiveMissCount = clamp<f64>(comboBasedMissCount, (f64)misses, (f64)(c50 + c100 + misses));

    // custom multipliers for nofail and spunout
    f64 multiplier = 1.14;  // keep final pp normalized across changes
    {
        if(modsLegacy & ModFlags::NoFail)
            multiplier *=
                max(0.9, 1.0 - 0.02 * effectiveMissCount);  // see https://github.com/ppy/osu-performance/pull/127/files

        if((modsLegacy & ModFlags::SpunOut) && totalHits > 0)
            multiplier *= 1.0 - pow((f64)numSpinners / (f64)totalHits,
                                    0.85);  // see https://github.com/ppy/osu-performance/pull/110/

        if((modsLegacy & ModFlags::Relax)) {
            f64 okMultiplier = max(0.0, od > 0.0 ? 1.0 - pow(od / 13.33, 1.8) : 1.0);   // 100
            f64 mehMultiplier = max(0.0, od > 0.0 ? 1.0 - pow(od / 13.33, 5.0) : 1.0);  // 50
            effectiveMissCount = min(effectiveMissCount + c100 * okMultiplier + c50 * mehMultiplier, (f64)totalHits);
        }
    }

    f64 aimValue = 0.0;
    if(!(modsLegacy & ModFlags::Autopilot)) {
        f64 rawAim = aim;
        aimValue = pow(5.0 * max(1.0, rawAim / 0.0675) - 4.0, 3.0) / 100000.0;

        // length bonus
        f64 lengthBonus = 0.95 + 0.4 * min(1.0, ((f64)totalHits / 2000.0)) +
                          (totalHits > 2000 ? std::log10(((f64)totalHits / 2000.0)) * 0.5 : 0.0);
        aimValue *= lengthBonus;

        // see https://github.com/ppy/osu-performance/pull/129/
        // Penalize misses by assessing # of misses relative to the total # of objects. Default a 3% reduction for any #
        // of misses.
        if(effectiveMissCount > 0 && totalHits > 0)
            aimValue *= 0.97 * pow(1.0 - pow(effectiveMissCount / (f64)totalHits, 0.775), effectiveMissCount);

        // combo scaling
        if(maxPossibleCombo > 0) aimValue *= min(pow((f64)combo, 0.8) / pow((f64)maxPossibleCombo, 0.8), 1.0);

        // ar bonus
        f64 approachRateFactor = 0.0;  // see https://github.com/ppy/osu-performance/pull/125/
        if((modsLegacy & ModFlags::Relax) == 0) {
            if(ar > 10.33)
                approachRateFactor =
                    0.3 *
                    (ar - 10.33);  // from 0.3 to 0.4 see https://github.com/ppy/osu-performance/pull/125/ // and
                                   // completely changed the logic in https://github.com/ppy/osu-performance/pull/135/
            else if(ar < 8.0)
                approachRateFactor =
                    0.05 * (8.0 - ar);  // from 0.01 to 0.1 see https://github.com/ppy/osu-performance/pull/125/
                                        // // and back again from 0.1 to 0.01 see
                                        // https://github.com/ppy/osu-performance/pull/133/ // and completely
                                        // changed the logic in https://github.com/ppy/osu-performance/pull/135/
        }

        aimValue *= 1.0 + approachRateFactor * lengthBonus;

        // hidden
        if(modsLegacy & ModFlags::Hidden)
            aimValue *= 1.0 + 0.04 * (max(12.0 - ar,
                                          0.0));  // NOTE: clamped to 0 because neosu allows AR > 12

        // "We assume 15% of sliders in a map are difficult since there's no way to tell from the performance
        // calculator."
        f64 estimateDifficultSliders = numSliders * 0.15;
        if(numSliders > 0) {
            f64 estimateSliderEndsDropped =
                clamp<f64>((f64)min(c100 + c50 + misses, maxPossibleCombo - combo), 0.0, estimateDifficultSliders);
            f64 sliderNerfFactor =
                (1.0 - aimSliderFactor) * pow(1.0 - estimateSliderEndsDropped / estimateDifficultSliders, 3.0) +
                aimSliderFactor;
            aimValue *= sliderNerfFactor;
        }

        // scale aim with acc
        aimValue *= accuracy;
        // also consider acc difficulty when doing that
        aimValue *= 0.98 + pow(od, 2.0) / 2500.0;
    }

    f64 speedValue = 0.0;
    if(!(modsLegacy & ModFlags::Relax)) {
        speedValue = pow(5.0 * max(1.0, speed / 0.0675) - 4.0, 3.0) / 100000.0;

        // length bonus
        f64 lengthBonus = 0.95 + 0.4 * min(1.0, ((f64)totalHits / 2000.0)) +
                          (totalHits > 2000 ? std::log10(((f64)totalHits / 2000.0)) * 0.5 : 0.0);
        speedValue *= lengthBonus;

        // see https://github.com/ppy/osu-performance/pull/129/
        // Penalize misses by assessing # of misses relative to the total # of objects. Default a 3% reduction for any #
        // of misses.
        if(effectiveMissCount > 0 && totalHits > 0)
            speedValue *=
                0.97 * pow(1.0 - pow(effectiveMissCount / (f64)totalHits, 0.775), pow(effectiveMissCount, 0.875));

        // combo scaling
        if(maxPossibleCombo > 0) speedValue *= min(pow((f64)combo, 0.8) / pow((f64)maxPossibleCombo, 0.8), 1.0);

        // ar bonus
        f64 approachRateFactor = 0.0;  // see https://github.com/ppy/osu-performance/pull/125/
        if(ar > 10.33)
            approachRateFactor =
                0.3 * (ar - 10.33);  // from 0.3 to 0.4 see https://github.com/ppy/osu-performance/pull/125/ // and
                                     // completely changed the logic in https://github.com/ppy/osu-performance/pull/135/

        speedValue *= 1.0 + approachRateFactor * lengthBonus;

        // hidden
        if(modsLegacy & ModFlags::Hidden)
            speedValue *= 1.0 + 0.04 * (max(12.0 - ar,
                                            0.0));  // NOTE: clamped to 0 because neosu allows AR > 12

        // "Calculate accuracy assuming the worst case scenario"
        f64 relevantTotalDiff = totalHits - speedNotes;
        f64 relevantCountGreat = max(0.0, c300 - relevantTotalDiff);
        f64 relevantCountOk = max(0.0, c100 - max(0.0, relevantTotalDiff - c300));
        f64 relevantCountMeh = max(0.0, c50 - max(0.0, relevantTotalDiff - c300 - c100));
        f64 relevantAccuracy = speedNotes == 0 ? 0
                                               : (relevantCountGreat * 6.0 + relevantCountOk * 2.0 + relevantCountMeh) /
                                                     (speedNotes * 6.0);

        // see https://github.com/ppy/osu-performance/pull/128/
        // Scale the speed value with accuracy and OD
        speedValue *=
            (0.95 + pow(od, 2.0) / 750.0) * pow((accuracy + relevantAccuracy) / 2.0, (14.5 - max(od, 8.0)) / 2.0);
        // Scale the speed value with # of 50s to punish doubletapping.
        speedValue *= pow(0.99, c50 < (totalHits / 500.0) ? 0.0 : c50 - (totalHits / 500.0));
    }

    f64 accuracyValue = 0.0;
    {
        f64 betterAccuracyPercentage;
        if(amountHitObjectsWithAccuracy > 0)
            betterAccuracyPercentage =
                ((f64)(c300 - (totalHits - amountHitObjectsWithAccuracy)) * 6.0 + (c100 * 2.0) + c50) /
                (f64)(amountHitObjectsWithAccuracy * 6.0);
        else
            betterAccuracyPercentage = 0.0;

        // it's possible to reach negative accuracy, cap at zero
        if(betterAccuracyPercentage < 0.0) betterAccuracyPercentage = 0.0;

        // arbitrary values tom crafted out of trial and error
        accuracyValue = pow(1.52163, od) * pow(betterAccuracyPercentage, 24.0) * 2.83;

        // length bonus
        accuracyValue *= min(1.15, pow(amountHitObjectsWithAccuracy / 1000.0, 0.3));

        // hidden bonus
        if(modsLegacy & ModFlags::Hidden) accuracyValue *= 1.08;
        // flashlight bonus
        if(modsLegacy & ModFlags::Flashlight) accuracyValue *= 1.02;
    }

    f64 totalValue = pow(pow(aimValue, 1.1) + pow(speedValue, 1.1) + pow(accuracyValue, 1.1), 1.0 / 1.1) * multiplier;
    return totalValue;
}
