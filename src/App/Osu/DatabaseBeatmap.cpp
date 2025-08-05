#include "DatabaseBeatmap.h"

#include <assert.h>

#include <algorithm>
#include <cinttypes>
#include <iostream>
#include <limits>
#include <sstream>
#include <utility>

#include "SString.h"
#include "Bancho.h"  // md5
#include "Beatmap.h"
#include "ConVar.h"
#include "Database.h"
#include "Engine.h"
#include "File.h"
#include "GameRules.h"
#include "HitObjects.h"
#include "NotificationOverlay.h"
#include "Osu.h"
#include "Skin.h"
#include "SliderCurves.h"
#include "SongBrowser/SongBrowser.h"
namespace {  // static namespace

bool timingPointSortComparator(DatabaseBeatmap::TIMINGPOINT const &a, DatabaseBeatmap::TIMINGPOINT const &b) {
    if(a.offset != b.offset) return a.offset < b.offset;

    // non-inherited timingpoints go before inherited timingpoints
    bool a_inherited = a.msPerBeat >= 0;
    bool b_inherited = b.msPerBeat >= 0;
    if(a_inherited != b_inherited) return a_inherited;

    if(a.sampleType != b.sampleType) return static_cast<int>(a.sampleType) < static_cast<int>(b.sampleType);
    if(a.sampleSet != b.sampleSet) return a.sampleSet < b.sampleSet;
    if(a.kiai != b.kiai) return a.kiai;

    return false;  // equivalent
}
}  // namespace

DatabaseBeatmap::DatabaseBeatmap(std::string filePath, std::string folder, BeatmapType type) {
    this->sFilePath = std::move(filePath);
    this->sFolder = std::move(folder);
    this->type = type;

    // raw metadata (note the special default values)

    this->iVersion = cv::beatmap_version.getInt();
    this->iGameMode = 0;
    this->iID = 0;
    this->iSetID = -1;

    this->iLengthMS = 0;
    this->iPreviewTime = -1;

    this->fAR = 5.0f;
    this->fCS = 5.0f;
    this->fHP = 5.0f;
    this->fOD = 5.0f;

    this->fStackLeniency = 0.7f;
    this->fSliderTickRate = 1.0f;
    this->fSliderMultiplier = 1.0f;

    // precomputed data

    this->fStarsNomod = 0.0f;

    this->iMinBPM = 0;
    this->iMaxBPM = 0;
    this->iMostCommonBPM = 0;

    this->iNumObjects = 0;
    this->iNumCircles = 0;
    this->iNumSliders = 0;
    this->iNumSpinners = 0;

    // custom data
    this->iLocalOffset = 0;
    this->iOnlineOffset = 0;
}

DatabaseBeatmap::DatabaseBeatmap(std::vector<DatabaseBeatmap *> *difficulties, BeatmapType type)
    : DatabaseBeatmap("", "", type) {
    this->difficulties = difficulties;
    if(this->difficulties->empty()) return;

    // set representative values for this container (i.e. use values from first difficulty)
    this->sTitle = (*this->difficulties)[0]->sTitle;
    this->sTitleUnicode = (*this->difficulties)[0]->sTitleUnicode;
    this->sArtist = (*this->difficulties)[0]->sArtist;
    this->sArtistUnicode = (*this->difficulties)[0]->sArtistUnicode;
    this->sCreator = (*this->difficulties)[0]->sCreator;
    this->sBackgroundImageFileName = (*this->difficulties)[0]->sBackgroundImageFileName;
    this->iSetID = (*this->difficulties)[0]->iSetID;

    // also calculate largest representative values
    this->iLengthMS = 0;
    this->fCS = 99.f;
    this->fAR = 0.0f;
    this->fOD = 0.0f;
    this->fHP = 0.0f;
    this->fStarsNomod = 0.0f;
    this->iMinBPM = 9001;
    this->iMaxBPM = 0;
    this->iMostCommonBPM = 0;
    this->last_modification_time = 0;
    for(auto diff : (*this->difficulties)) {
        if(diff->getLengthMS() > this->iLengthMS) this->iLengthMS = diff->getLengthMS();
        if(diff->getCS() < this->fCS) this->fCS = diff->getCS();
        if(diff->getAR() > this->fAR) this->fAR = diff->getAR();
        if(diff->getHP() > this->fHP) this->fHP = diff->getHP();
        if(diff->getOD() > this->fOD) this->fOD = diff->getOD();
        if(diff->getStarsNomod() > this->fStarsNomod) this->fStarsNomod = diff->getStarsNomod();
        if(diff->getMinBPM() < this->iMinBPM) this->iMinBPM = diff->getMinBPM();
        if(diff->getMaxBPM() > this->iMaxBPM) this->iMaxBPM = diff->getMaxBPM();
        if(diff->getMostCommonBPM() > this->iMostCommonBPM) this->iMostCommonBPM = diff->getMostCommonBPM();
        if(diff->last_modification_time > this->last_modification_time)
            this->last_modification_time = diff->last_modification_time;
    }
}

DatabaseBeatmap::~DatabaseBeatmap() {
    if(this->difficulties != NULL) {
        for(auto diff : (*this->difficulties)) {
            assert(diff->difficulties == NULL);
            delete diff;
        }
        delete this->difficulties;
    }
}

DatabaseBeatmap::PRIMITIVE_CONTAINER DatabaseBeatmap::loadPrimitiveObjects(const std::string &osuFilePath) {
    std::atomic<bool> dead;
    dead = false;
    return loadPrimitiveObjects(osuFilePath, dead);
}

DatabaseBeatmap::PRIMITIVE_CONTAINER DatabaseBeatmap::loadPrimitiveObjects(const std::string &osuFilePath,
                                                                           const std::atomic<bool> &dead) {
    PRIMITIVE_CONTAINER c;
    {
        c.errorCode = 0;

        c.stackLeniency = 0.7f;

        c.sliderMultiplier = 1.0f;
        c.sliderTickRate = 1.0f;

        c.numCircles = 0;
        c.numSliders = 0;
        c.numSpinners = 0;
        c.numHitobjects = 0;

        c.version = 14;
    }

    const float sliderSanityRange = cv::slider_curve_max_length.getFloat();  // infinity sanity check, same as before
    const int sliderMaxRepeatRange =
        cv::slider_max_repeats.getInt();  // NOTE: osu! will refuse to play any beatmap which has sliders with more than
                                          // 9000 repeats, here we just clamp it instead

    // open osu file for parsing
    {
        File file(osuFilePath);
        if(!file.canRead()) {
            c.errorCode = 2;
            return c;
        }

        // std::istringstream ss("");

        // load the actual beatmap
        int hitobjectsWithoutSpinnerCounter = 0;
        int colorCounter = 1;
        int colorOffset = 0;
        int comboNumber = 1;
        int curBlock = -1;
        std::string curLine;
        while(file.canRead()) {
            if(dead.load()) {
                c.errorCode = 6;
                return c;
            }

            curLine = file.readLine();

            const char *curLineChar = curLine.c_str();
            if(!curLine.starts_with("//"))  // ignore comments, but only if at the beginning of a line (e.g. allow
                                            // Artist:DJ'TEKINA//SOMETHING)
            {
                if(curLine.find("[General]") != std::string::npos)
                    curBlock = 1;
                else if(curLine.find("[Difficulty]") != std::string::npos)
                    curBlock = 2;
                else if(curLine.find("[Events]") != std::string::npos)
                    curBlock = 3;
                else if(curLine.find("[TimingPoints]") != std::string::npos)
                    curBlock = 4;
                else if(curLine.find("[Colours]") != std::string::npos)
                    curBlock = 5;
                else if(curLine.find("[HitObjects]") != std::string::npos)
                    curBlock = 6;

                switch(curBlock) {
                    case -1:  // header (e.g. "osu file format v12")
                    {
                        sscanf(curLineChar, " osu file format v %i \n", &c.version);
                    } break;

                    case 1:  // General
                    {
                        sscanf(curLineChar, " StackLeniency : %f \n", &c.stackLeniency);
                    } break;

                    case 2:  // Difficulty
                    {
                        sscanf(curLineChar, " SliderMultiplier : %f \n", &c.sliderMultiplier);
                        sscanf(curLineChar, " SliderTickRate : %f \n", &c.sliderTickRate);
                    } break;

                    case 3:  // Events
                    {
                        i64 type, startTime, endTime;
                        if(sscanf(curLineChar, " %" PRId64 " , %" PRId64 " , %" PRId64 " \n", &type, &startTime,
                                  &endTime) == 3) {
                            if(type == 2) {
                                BREAK b{.startTime = startTime, .endTime = endTime};
                                c.breaks.push_back(b);
                            }
                        }
                    } break;

                    case 4:  // TimingPoints
                    {
                        // old beatmaps: Offset, Milliseconds per Beat
                        // old new beatmaps: Offset, Milliseconds per Beat, Meter, Sample Type, Sample Set, Volume,
                        // !Inherited new new beatmaps: Offset, Milliseconds per Beat, Meter, Sample Type, Sample Set,
                        // Volume, !Inherited, Kiai Mode

                        double tpOffset;
                        float tpMSPerBeat;
                        int tpMeter;
                        int tpSampleType, tpSampleSet;
                        int tpVolume;
                        int tpTimingChange;
                        int tpKiai = 0;  // optional
                        if(sscanf(curLineChar, " %lf , %f , %i , %i , %i , %i , %i , %i", &tpOffset, &tpMSPerBeat,
                                  &tpMeter, &tpSampleType, &tpSampleSet, &tpVolume, &tpTimingChange, &tpKiai) == 8 ||
                           sscanf(curLineChar, " %lf , %f , %i , %i , %i , %i , %i", &tpOffset, &tpMSPerBeat, &tpMeter,
                                  &tpSampleType, &tpSampleSet, &tpVolume, &tpTimingChange) == 7) {
                            DatabaseBeatmap::TIMINGPOINT t{
                                .offset = std::round(tpOffset),
                                .msPerBeat = tpMSPerBeat,

                                .sampleType = tpSampleType,
                                .sampleSet = tpSampleSet,
                                .volume = tpVolume,

                                .timingChange = tpTimingChange == 1,
                                .kiai = tpKiai > 0,
                            };
                            c.timingpoints.push_back(t);
                        } else if(sscanf(curLineChar, " %lf , %f", &tpOffset, &tpMSPerBeat) == 2) {
                            DatabaseBeatmap::TIMINGPOINT t{
                                .offset = std::round(tpOffset),
                                .msPerBeat = tpMSPerBeat,

                                .sampleType = 0,
                                .sampleSet = 0,
                                .volume = 100,

                                .timingChange = true,
                                .kiai = false,
                            };
                            c.timingpoints.push_back(t);
                        }
                    } break;

                    case 5:  // Colours
                    {
                        int comboNum;
                        int r, g, b;
                        if(sscanf(curLineChar, " Combo %i : %i , %i , %i \n", &comboNum, &r, &g, &b) == 4)
                            c.combocolors.push_back(argb(255, r, g, b));
                    } break;

                    case 6:  // HitObjects

                        // circles:
                        // x,y,time,type,hitSound,addition
                        // sliders:
                        // x,y,time,type,hitSound,sliderType|curveX:curveY|...,repeat,pixelLength,edgeHitsound,edgeAddition,addition
                        // spinners:
                        // x,y,time,type,hitSound,endTime,addition

                        // NOTE: calculating combo numbers and color offsets based on the parsing order is dangerous.
                        // maybe the hitobjects are not sorted by time in the file; these values should be calculated
                        // after sorting just to be sure?

                        int x, y;
                        u32 time;
                        int type;
                        int hitSound;

                        const bool intScan =
                            (sscanf(curLineChar, " %i , %i , %d , %i , %i", &x, &y, &time, &type, &hitSound) == 5);
                        bool floatScan = false;
                        if(!intScan) {
                            float fX, fY;
                            floatScan = (sscanf(curLineChar, " %f , %f , %d , %i , %i", &fX, &fY, &time, &type,
                                                &hitSound) == 5);
                            x = (std::isfinite(fX) && fX >= static_cast<float>(std::numeric_limits<int>::min()) &&
                                 fX <= static_cast<float>(std::numeric_limits<int>::max()))
                                    ? static_cast<int>(fX)
                                    : 0;
                            y = (std::isfinite(fY) && fY >= static_cast<float>(std::numeric_limits<int>::min()) &&
                                 fY <= static_cast<float>(std::numeric_limits<int>::max()))
                                    ? static_cast<int>(fY)
                                    : 0;
                        }

                        if(intScan || floatScan) {
                            if(!(type & 0x8)) hitobjectsWithoutSpinnerCounter++;

                            if(type & 0x4)  // new combo
                            {
                                comboNumber = 1;

                                // special case 1: if the current object is a spinner, then the raw color counter is not
                                // increased (but the offset still is!) special case 2: the first (non-spinner)
                                // hitobject in a beatmap is always a new combo, therefore the raw color counter is not
                                // increased for it (but the offset still is!)
                                if(!(type & 0x8) && hitobjectsWithoutSpinnerCounter > 1) colorCounter++;

                                colorOffset +=
                                    (type >> 4) & 7;  // special case 3: "Bits 4-6 (16, 32, 64) form a 3-bit number
                                                      // (0-7) that chooses how many combo colours to skip."
                            }

                            if(type & 0x1)  // circle
                            {
                                HITCIRCLE h{};
                                {
                                    h.x = x;
                                    h.y = y;
                                    h.time = time;
                                    h.sampleType = hitSound;
                                    h.number = comboNumber++;
                                    h.colorCounter = colorCounter;
                                    h.colorOffset = colorOffset;
                                    h.clicked = false;
                                }
                                c.hitcircles.push_back(h);
                            } else if(type & 0x2)  // slider
                            {
                                std::vector<UString> tokens = UString(curLineChar).split(",");
                                if(tokens.size() < 8) {
                                    debugLog("Invalid slider in beatmap: {:s}\n\ncurLine = {:s}\n", osuFilePath,
                                             curLineChar);
                                    continue;
                                    // engine->showMessageError("Error", UString::format("Invalid slider in beatmap:
                                    // %s\n\ncurLine = %s", m_sFilePath.toUtf8(), curLine)); return false;
                                }

                                std::vector<UString> sliderTokens = tokens[5].split("|");
                                if(sliderTokens.size() < 1)  // partially allow bullshit sliders (no controlpoints!),
                                                             // e.g. https://osu.ppy.sh/beatmapsets/791900#osu/1676490
                                {
                                    debugLog("Invalid slider tokens: {:s}\n\nIn beatmap: {:s}\n", curLineChar,
                                             osuFilePath);
                                    continue;
                                    // engine->showMessageError("Error", UString::format("Invalid slider tokens:
                                    // %s\n\nIn beatmap: %s", curLineChar, m_sFilePath.toUtf8())); return false;
                                }

                                std::vector<Vector2> points;
                                for(int i = 1; i < sliderTokens.size();
                                    i++)  // NOTE: starting at 1 due to slider type char
                                {
                                    std::vector<UString> sliderXY = sliderTokens[i].split(":");

                                    // array size check
                                    // infinity sanity check (this only exists because of https://osu.ppy.sh/b/1029976)
                                    // not a very elegant check, but it does the job
                                    if(sliderXY.size() != 2 || sliderXY[0].find("E") != -1 ||
                                       sliderXY[0].find("e") != -1 || sliderXY[1].find("E") != -1 ||
                                       sliderXY[1].find("e") != -1) {
                                        debugLog("Invalid slider positions: {:s}\n\nIn Beatmap: {:s}\n", curLineChar,
                                                 osuFilePath);
                                        continue;
                                        // engine->showMessageError("Error", UString::format("Invalid slider positions:
                                        // %s\n\nIn beatmap: %s", curLine, m_sFilePath.toUtf8())); return false;
                                    }

                                    points.emplace_back((int)std::clamp<float>(sliderXY[0].toFloat(),
                                                                               -sliderSanityRange, sliderSanityRange),
                                                        (int)std::clamp<float>(sliderXY[1].toFloat(),
                                                                               -sliderSanityRange, sliderSanityRange));
                                }

                                // special case: osu! logic for handling the hitobject point vs the controlpoints (since
                                // sliders have both, and older beatmaps store the start point inside the control
                                // points)
                                {
                                    const Vector2 xy =
                                        Vector2(std::clamp<float>(x, -sliderSanityRange, sliderSanityRange),
                                                std::clamp<float>(y, -sliderSanityRange, sliderSanityRange));
                                    if(points.size() > 0) {
                                        if(points[0] != xy) points.insert(points.begin(), xy);
                                    } else
                                        points.push_back(xy);
                                }

                                // partially allow bullshit sliders (add second point to make valid), e.g.
                                // https://osu.ppy.sh/beatmapsets/791900#osu/1676490
                                if(sliderTokens.size() < 2 && points.size() > 0) points.push_back(points[0]);

                                SLIDER s{
                                    .x = x,
                                    .y = y,
                                    .type = (sliderTokens[0].toUtf8())[0],
                                    .repeat = std::clamp<int>((int)tokens[6].toFloat(), 0, sliderMaxRepeatRange),
                                    .pixelLength =
                                        std::clamp<float>(tokens[7].toFloat(), -sliderSanityRange, sliderSanityRange),
                                    .time = time,
                                    .sampleType = hitSound,
                                    .number = comboNumber++,
                                    .colorCounter = colorCounter,
                                    .colorOffset = colorOffset,
                                    .points = points,
                                    // new beatmaps: slider hitsounds
                                    .hitSounds = tokens.size() > 8 ? tokens[8].split<int>("|") : std::vector<int>{},
                                    .sliderTime{},
                                    .sliderTimeWithoutRepeats{},
                                    .ticks{},
                                    .scoringTimesForStarCalc{}};
                                c.sliders.push_back(s);
                            } else if(type & 0x8)  // spinner
                            {
                                auto tokens = UString(curLineChar).split<float>(",");
                                if(tokens.size() < 6) {
                                    debugLog("Invalid spinner in beatmap: {:s}\n\ncurLine = {:s}\n", osuFilePath,
                                             curLineChar);
                                    continue;
                                    // engine->showMessageError("Error", UString::format("Invalid spinner in beatmap:
                                    // %s\n\ncurLine = %s", m_sFilePath.toUtf8(), curLine)); return false;
                                }

                                SPINNER s{};
                                {
                                    s.x = x;
                                    s.y = y;
                                    s.time = time;
                                    s.sampleType = hitSound;
                                    s.endTime = static_cast<int>(tokens[5]);
                                }
                                c.spinners.push_back(s);
                            }
                        }
                        break;
                }
            }
        }
    }

    // late bail if too many hitobjects would run out of memory and crash
    c.numCircles = c.hitcircles.size();
    c.numSliders = c.sliders.size();
    c.numSpinners = c.spinners.size();
    c.numHitobjects = c.numCircles + c.numSliders + c.numSpinners;
    if(c.numHitobjects > (size_t)cv::beatmap_max_num_hitobjects.getInt()) {
        c.errorCode = 5;
        return c;
    }

    // sort timingpoints by time
    if(c.timingpoints.size() > 1) std::ranges::sort(c.timingpoints, timingPointSortComparator);

    return c;
}

DatabaseBeatmap::CALCULATE_SLIDER_TIMES_CLICKS_TICKS_RESULT DatabaseBeatmap::calculateSliderTimesClicksTicks(
    int beatmapVersion, std::vector<SLIDER> &sliders, zarray<DatabaseBeatmap::TIMINGPOINT> &timingpoints,
    float sliderMultiplier, float sliderTickRate) {
    std::atomic<bool> dead;
    dead = false;
    return calculateSliderTimesClicksTicks(beatmapVersion, sliders, timingpoints, sliderMultiplier, sliderTickRate,
                                           false);
}

DatabaseBeatmap::CALCULATE_SLIDER_TIMES_CLICKS_TICKS_RESULT DatabaseBeatmap::calculateSliderTimesClicksTicks(
    int beatmapVersion, std::vector<SLIDER> &sliders, zarray<DatabaseBeatmap::TIMINGPOINT> &timingpoints,
    float sliderMultiplier, float sliderTickRate, const std::atomic<bool> &dead) {
    CALCULATE_SLIDER_TIMES_CLICKS_TICKS_RESULT r;
    { r.errorCode = 0; }

    if(timingpoints.size() < 1) {
        r.errorCode = 3;
        return r;
    }

    struct SliderHelper {
        static float getSliderTickDistance(float sliderMultiplier, float sliderTickRate) {
            return ((100.0f * sliderMultiplier) / sliderTickRate);
        }

        static float getSliderTimeForSlider(const SLIDER &slider, const TIMING_INFO &timingInfo,
                                            float sliderMultiplier) {
            const float duration = timingInfo.beatLength * (slider.pixelLength / sliderMultiplier) / 100.0f;
            return duration >= 1.0f ? duration : 1.0f;  // sanity check
        }

        static float getSliderVelocity(const TIMING_INFO &timingInfo, float sliderMultiplier, float sliderTickRate) {
            const float beatLength = timingInfo.beatLength;
            if(beatLength > 0.0f)
                return (getSliderTickDistance(sliderMultiplier, sliderTickRate) * sliderTickRate *
                        (1000.0f / beatLength));
            else
                return getSliderTickDistance(sliderMultiplier, sliderTickRate) * sliderTickRate;
        }

        static float getTimingPointMultiplierForSlider(const TIMING_INFO &timingInfo)  // needed for slider ticks
        {
            float beatLengthBase = timingInfo.beatLengthBase;
            if(beatLengthBase == 0.0f)  // sanity check
                beatLengthBase = 1.0f;

            return timingInfo.beatLength / beatLengthBase;
        }
    };

    for(auto &s : sliders) {
        if(dead.load()) {
            r.errorCode = 6;
            return r;
        }

        // sanity reset
        s.ticks.clear();
        s.scoringTimesForStarCalc.clear();

        // calculate duration
        const TIMING_INFO timingInfo = getTimingInfoForTimeAndTimingPoints(s.time, timingpoints);
        s.sliderTimeWithoutRepeats = SliderHelper::getSliderTimeForSlider(s, timingInfo, sliderMultiplier);
        s.sliderTime = s.sliderTimeWithoutRepeats * s.repeat;

        // calculate ticks
        {
            const float minTickPixelDistanceFromEnd =
                0.01f * SliderHelper::getSliderVelocity(timingInfo, sliderMultiplier, sliderTickRate);
            const float tickPixelLength =
                (beatmapVersion < 8 ? SliderHelper::getSliderTickDistance(sliderMultiplier, sliderTickRate)
                                    : SliderHelper::getSliderTickDistance(sliderMultiplier, sliderTickRate) /
                                          SliderHelper::getTimingPointMultiplierForSlider(timingInfo));
            const float tickDurationPercentOfSliderLength =
                tickPixelLength / (s.pixelLength == 0.0f ? 1.0f : s.pixelLength);
            const int max_ticks = cv::slider_max_ticks.getInt();
            const int tickCount = std::min((int)std::ceil(s.pixelLength / tickPixelLength) - 1,
                                           max_ticks);  // NOTE: hard sanity limit number of ticks per slider

            if(tickCount > 0 && !timingInfo.isNaN && !std::isnan(s.pixelLength) &&
               !std::isnan(tickPixelLength))  // don't generate ticks for NaN timingpoints and infinite values
            {
                const float tickTOffset = tickDurationPercentOfSliderLength;
                float pixelDistanceToEnd = s.pixelLength;
                float t = tickTOffset;
                for(int i = 0; i < tickCount; i++, t += tickTOffset) {
                    // skip ticks which are too close to the end of the slider
                    pixelDistanceToEnd -= tickPixelLength;
                    if(pixelDistanceToEnd <= minTickPixelDistanceFromEnd) break;

                    s.ticks.push_back(t);
                }
            }
        }

        // bail if too many predicted heuristic scoringTimes would run out of memory and crash
        if((size_t)std::abs(s.repeat) * s.ticks.size() > (size_t)cv::beatmap_max_num_slider_scoringtimes.getInt()) {
            r.errorCode = 5;
            return r;
        }

        // calculate s.scoringTimesForStarCalc, which should include every point in time where the cursor must be within
        // the followcircle radius and at least one key must be pressed: see
        // https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Difficulty/Preprocessing/OsuDifficultyHitObject.cs
        const long osuSliderEndInsideCheckOffset = (long)cv::slider_end_inside_check_offset.getInt();

        // 1) "skip the head circle"

        // 2) add repeat times (either at slider begin or end)
        for(int i = 0; i < (s.repeat - 1); i++) {
            const f32 time = s.time + (s.sliderTimeWithoutRepeats * (i + 1));  // see Slider.cpp
            s.scoringTimesForStarCalc.push_back(OsuDifficultyHitObject::SLIDER_SCORING_TIME{
                .type = OsuDifficultyHitObject::SLIDER_SCORING_TIME::TYPE::REPEAT,
                .time = time,
            });
        }

        // 3) add tick times (somewhere within slider, repeated for every repeat)
        for(int i = 0; i < s.repeat; i++) {
            for(int t = 0; t < s.ticks.size(); t++) {
                const float tickPercentRelativeToRepeatFromStartAbs =
                    (((i + 1) % 2) != 0 ? s.ticks[t] : 1.0f - s.ticks[t]);  // see Slider.cpp
                const f32 time =
                    s.time + (s.sliderTimeWithoutRepeats * i) +
                    (tickPercentRelativeToRepeatFromStartAbs * s.sliderTimeWithoutRepeats);  // see Slider.cpp
                s.scoringTimesForStarCalc.push_back(OsuDifficultyHitObject::SLIDER_SCORING_TIME{
                    .type = OsuDifficultyHitObject::SLIDER_SCORING_TIME::TYPE::TICK,
                    .time = time,
                });
            }
        }

        // 4) add slider end (potentially before last tick for bullshit sliders, but sorting takes care of that)
        // see https://github.com/ppy/osu/pull/4193#issuecomment-460127543
        const f32 time =
            std::max(static_cast<f32>(s.time) + s.sliderTime / 2.0f,
                     (static_cast<f32>(s.time) + s.sliderTime) - static_cast<f32>(osuSliderEndInsideCheckOffset));
        s.scoringTimesForStarCalc.push_back(OsuDifficultyHitObject::SLIDER_SCORING_TIME{
            .type = OsuDifficultyHitObject::SLIDER_SCORING_TIME::TYPE::END,
            .time = time,
        });

        if(dead.load()) {
            r.errorCode = 6;
            return r;
        }

        constexpr auto sliderScoringTimeComparator = [](const OsuDifficultyHitObject::SLIDER_SCORING_TIME &a,
                                                        const OsuDifficultyHitObject::SLIDER_SCORING_TIME &b) -> bool {
            if(a.time != b.time) return a.time < b.time;
            if(a.type != b.type) return static_cast<int>(a.type) < static_cast<int>(b.type);
            return false;  // equivalent
        };

        // 5) sort scoringTimes from earliest to latest
        if(s.scoringTimesForStarCalc.size() > 1) {
            std::ranges::sort(s.scoringTimesForStarCalc, sliderScoringTimeComparator);
        }
    }

    return r;
}

DatabaseBeatmap::LOAD_DIFFOBJ_RESULT DatabaseBeatmap::loadDifficultyHitObjects(const std::string &osuFilePath, float AR,
                                                                               float CS, float speedMultiplier,
                                                                               bool calculateStarsInaccurately) {
    std::atomic<bool> dead;
    dead = false;
    return loadDifficultyHitObjects(osuFilePath, AR, CS, speedMultiplier, calculateStarsInaccurately, dead);
}

DatabaseBeatmap::LOAD_DIFFOBJ_RESULT DatabaseBeatmap::loadDifficultyHitObjects(const std::string &osuFilePath, float AR,
                                                                               float CS, float speedMultiplier,
                                                                               bool calculateStarsInaccurately,
                                                                               const std::atomic<bool> &dead) {
    // load primitive arrays
    PRIMITIVE_CONTAINER c = loadPrimitiveObjects(osuFilePath, dead);
    return loadDifficultyHitObjects(c, AR, CS, speedMultiplier, calculateStarsInaccurately, dead);
}

DatabaseBeatmap::LOAD_DIFFOBJ_RESULT DatabaseBeatmap::loadDifficultyHitObjects(PRIMITIVE_CONTAINER &c, float AR,
                                                                               float CS, float speedMultiplier,
                                                                               bool calculateStarsInaccurately,
                                                                               const std::atomic<bool> &dead) {
    LOAD_DIFFOBJ_RESULT result = LOAD_DIFFOBJ_RESULT();

    // build generalized OsuDifficultyHitObjects from the vectors (hitcircles, sliders, spinners)
    // the OsuDifficultyHitObject class is the one getting used in all pp/star calculations, it encompasses every object
    // type for simplicity

    if(c.errorCode != 0) {
        result.errorCode = c.errorCode;
        return result;
    }

    // calculate sliderTimes, and build slider clicks and ticks
    CALCULATE_SLIDER_TIMES_CLICKS_TICKS_RESULT sliderTimeCalcResult = calculateSliderTimesClicksTicks(
        c.version, c.sliders, c.timingpoints, c.sliderMultiplier, c.sliderTickRate, dead);
    if(sliderTimeCalcResult.errorCode != 0) {
        result.errorCode = sliderTimeCalcResult.errorCode;
        return result;
    }

    // now we can calculate the max possible combo (because that needs ticks/clicks to be filled, mostly convenience)
    {
        result.maxPossibleCombo += c.hitcircles.size();
        for(const auto &s : c.sliders) {
            const int repeats = std::max((s.repeat - 1), 0);
            result.maxPossibleCombo +=
                2 + repeats + (repeats + 1) * s.ticks.size();  // start/end + repeat arrow + ticks
        }
        result.maxPossibleCombo += c.spinners.size();
    }

    // and generate the difficultyhitobjects
    result.diffobjects.reserve(c.hitcircles.size() + c.sliders.size() + c.spinners.size());

    for(auto &hitcircle : c.hitcircles) {
        result.diffobjects.emplace_back(OsuDifficultyHitObject::TYPE::CIRCLE, Vector2(hitcircle.x, hitcircle.y),
                                        (long)hitcircle.time);
    }

    const bool calculateSliderCurveInConstructor =
        (c.sliders.size() < 5000);  // NOTE: for explanation see OsuDifficultyHitObject constructor
    for(auto &slider : c.sliders) {
        if(dead.load()) {
            result.errorCode = 6;
            return result;
        }

        if(!calculateStarsInaccurately) {
            result.diffobjects.emplace_back(
                OsuDifficultyHitObject::TYPE::SLIDER, Vector2(slider.x, slider.y), slider.time,
                slider.time + (long)slider.sliderTime, slider.sliderTimeWithoutRepeats, slider.type, slider.points,
                slider.pixelLength, slider.scoringTimesForStarCalc, slider.repeat, calculateSliderCurveInConstructor);
        } else {
            result.diffobjects.emplace_back(
                OsuDifficultyHitObject::TYPE::SLIDER, Vector2(slider.x, slider.y), slider.time,
                slider.time + (long)slider.sliderTime, slider.sliderTimeWithoutRepeats, slider.type,
                std::vector<Vector2>(),  // NOTE: ignore curve when calculating inaccurately
                slider.pixelLength,
                std::vector<OsuDifficultyHitObject::SLIDER_SCORING_TIME>(),  // NOTE: ignore curve when calculating
                                                                             // inaccurately
                slider.repeat,
                false);  // NOTE: ignore curve when calculating inaccurately
        }
    }

    for(auto &spinner : c.spinners) {
        result.diffobjects.emplace_back(OsuDifficultyHitObject::TYPE::SPINNER, Vector2(spinner.x, spinner.y),
                                        (long)spinner.time, (long)spinner.endTime);
    }

    if(dead.load()) {
        result.errorCode = 6;
        return result;
    }

    // sort hitobjects by time
    constexpr auto diffHitObjectSortComparator = [](const OsuDifficultyHitObject &a,
                                                    const OsuDifficultyHitObject &b) -> bool {
        if(a.time != b.time) return a.time < b.time;
        if(a.type != b.type) return static_cast<int>(a.type) < static_cast<int>(b.type);
        if(a.pos.x != b.pos.x) return a.pos.x < b.pos.x;
        if(a.pos.y != b.pos.y) return a.pos.y < b.pos.y;
        return false;  // equivalent
    };

    if(result.diffobjects.size() > 1) {
        std::ranges::sort(result.diffobjects, diffHitObjectSortComparator);
    }

    if(dead.load()) {
        result.errorCode = 6;
        return result;
    }

    // calculate stacks
    // see Beatmap.cpp
    // NOTE: this must be done before the speed multiplier is applied!
    // HACKHACK: code duplication ffs
    if(cv::stars_stacking.getBool() &&
       !calculateStarsInaccurately)  // NOTE: ignore stacking when calculating inaccurately
    {
        const float finalAR = AR;
        const float finalCS = CS;
        const float rawHitCircleDiameter = GameRules::getRawHitCircleDiameter(finalCS);

        const float STACK_LENIENCE = 3.0f;

        const float approachTime = GameRules::getApproachTimeForStacking(finalAR);

        if(c.version > 5) {
            // peppy's algorithm
            // https://gist.github.com/peppy/1167470

            for(int i = result.diffobjects.size() - 1; i >= 0; i--) {
                int n = i;

                OsuDifficultyHitObject *objectI = &result.diffobjects[i];

                const bool isSpinner = (objectI->type == OsuDifficultyHitObject::TYPE::SPINNER);

                if(objectI->stack != 0 || isSpinner) continue;

                const bool isHitCircle = (objectI->type == OsuDifficultyHitObject::TYPE::CIRCLE);
                const bool isSlider = (objectI->type == OsuDifficultyHitObject::TYPE::SLIDER);

                if(isHitCircle) {
                    while(--n >= 0) {
                        OsuDifficultyHitObject *objectN = &result.diffobjects[n];

                        const bool isSpinnerN = (objectN->type == OsuDifficultyHitObject::TYPE::SPINNER);

                        if(isSpinnerN) continue;

                        if(objectI->time - (approachTime * c.stackLeniency) > (objectN->endTime)) break;

                        Vector2 objectNEndPosition =
                            objectN->getOriginalRawPosAt(objectN->time + objectN->getDuration());
                        if(objectN->getDuration() != 0 &&
                           (objectNEndPosition - objectI->getOriginalRawPosAt(objectI->time)).length() <
                               STACK_LENIENCE) {
                            int offset = objectI->stack - objectN->stack + 1;
                            for(int j = n + 1; j <= i; j++) {
                                if((objectNEndPosition -
                                    result.diffobjects[j].getOriginalRawPosAt(result.diffobjects[j].time))
                                       .length() < STACK_LENIENCE)
                                    result.diffobjects[j].stack = (result.diffobjects[j].stack - offset);
                            }

                            break;
                        }

                        if((objectN->getOriginalRawPosAt(objectN->time) - objectI->getOriginalRawPosAt(objectI->time))
                               .length() < STACK_LENIENCE) {
                            objectN->stack = (objectI->stack + 1);
                            objectI = objectN;
                        }
                    }
                } else if(isSlider) {
                    while(--n >= 0) {
                        OsuDifficultyHitObject *objectN = &result.diffobjects[n];

                        const bool isSpinner = (objectN->type == OsuDifficultyHitObject::TYPE::SPINNER);

                        if(isSpinner) continue;

                        if(objectI->time - (approachTime * c.stackLeniency) > objectN->time) break;

                        if(((objectN->getDuration() != 0
                                 ? objectN->getOriginalRawPosAt(objectN->time + objectN->getDuration())
                                 : objectN->getOriginalRawPosAt(objectN->time)) -
                            objectI->getOriginalRawPosAt(objectI->time))
                               .length() < STACK_LENIENCE) {
                            objectN->stack = (objectI->stack + 1);
                            objectI = objectN;
                        }
                    }
                }
            }
        } else  // version < 6
        {
            // old stacking algorithm for old beatmaps
            // https://github.com/ppy/osu/blob/master/osu.Game.Rulesets.Osu/Beatmaps/OsuBeatmapProcessor.cs

            for(int i = 0; i < result.diffobjects.size(); i++) {
                OsuDifficultyHitObject *currHitObject = &result.diffobjects[i];

                const bool isSlider = (currHitObject->type == OsuDifficultyHitObject::TYPE::SLIDER);

                if(currHitObject->stack != 0 && !isSlider) continue;

                long startTime = currHitObject->time + currHitObject->getDuration();
                int sliderStack = 0;

                for(int j = i + 1; j < result.diffobjects.size(); j++) {
                    OsuDifficultyHitObject *objectJ = &result.diffobjects[j];

                    if(objectJ->time - (approachTime * c.stackLeniency) > startTime) break;

                    // "The start position of the hitobject, or the position at the end of the path if the hitobject is
                    // a slider"
                    Vector2 position2 =
                        isSlider
                            ? currHitObject->getOriginalRawPosAt(currHitObject->time + currHitObject->getDuration())
                            : currHitObject->getOriginalRawPosAt(currHitObject->time);

                    if((objectJ->getOriginalRawPosAt(objectJ->time) -
                        currHitObject->getOriginalRawPosAt(currHitObject->time))
                           .length() < 3) {
                        currHitObject->stack++;
                        startTime = objectJ->time + objectJ->getDuration();
                    } else if((objectJ->getOriginalRawPosAt(objectJ->time) - position2).length() < 3) {
                        // "Case for sliders - bump notes down and right, rather than up and left."
                        sliderStack++;
                        objectJ->stack -= sliderStack;
                        startTime = objectJ->time + objectJ->getDuration();
                    }
                }
            }
        }

        // update hitobject positions
        float stackOffset = rawHitCircleDiameter / 128.0f / GameRules::broken_gamefield_rounding_allowance * 6.4f;
        for(int i = 0; i < result.diffobjects.size(); i++) {
            if(dead.load()) {
                result.errorCode = 6;
                return result;
            }

            if(result.diffobjects[i].curve && result.diffobjects[i].stack != 0)
                result.diffobjects[i].updateStackPosition(stackOffset);
        }
    }

    // apply speed multiplier (if present)
    if(speedMultiplier != 1.0f && speedMultiplier > 0.0f) {
        const double invSpeedMultiplier = 1.0 / (double)speedMultiplier;
        for(int i = 0; i < result.diffobjects.size(); i++) {
            if(dead.load()) {
                result.errorCode = 6;
                return result;
            }

            result.diffobjects[i].time = (long)((double)result.diffobjects[i].time * invSpeedMultiplier);
            result.diffobjects[i].endTime = (long)((double)result.diffobjects[i].endTime * invSpeedMultiplier);

            if(!calculateStarsInaccurately)  // NOTE: ignore slider curves when calculating inaccurately
            {
                result.diffobjects[i].spanDuration = (double)result.diffobjects[i].spanDuration * invSpeedMultiplier;
                for(auto &scoringTime : result.diffobjects[i].scoringTimes) {
                    scoringTime.time = ((f64)scoringTime.time * invSpeedMultiplier);
                }
            }
        }
    }

    return result;
}

bool DatabaseBeatmap::loadMetadata(bool compute_md5) {
    if(this->difficulties != NULL) return false;  // we are a beatmapset, not a difficulty

    // reset
    this->timingpoints.clear();

    if(cv::debug.getBool()) debugLog("DatabaseBeatmap::loadMetadata() : {:s}\n", this->sFilePath.c_str());

    std::vector<char> fileBuffer;
    const u8 *beatmapFile{nullptr};
    size_t beatmapFileSize{0};

    {
        File file(this->sFilePath);
        if(file.canRead()) {
            beatmapFileSize = file.getFileSize();
            fileBuffer = file.takeFileBuffer();
            beatmapFile = reinterpret_cast<const u8 *>(fileBuffer.data());
        }
        // close the file here
    }

    if(fileBuffer.empty()) {
        debugLog("Osu Error: Couldn't read file {:s}\n", this->sFilePath.c_str());
        return false;
    }

    // compute MD5 hash (very slow)
    if(compute_md5) {
        this->sMD5Hash = {Bancho::md5((u8 *)beatmapFile, beatmapFileSize)};
    }

    // load metadata
    bool foundAR = false;
    int curBlock = -1;
    char stringBuffer[1024];
    std::string curLine;

    const u8 *start = beatmapFile;
    const u8 *end = beatmapFile + beatmapFileSize;
    while(start < end) {
        const u8 *lineEnd = (u8 *)memchr(start, '\n', end - start);
        if(!lineEnd) lineEnd = end;

        std::string curLine((const char *)start, lineEnd - start);
        start = lineEnd + 1;

        // ignore comments, but only if at the beginning of
        // a line (e.g. allow Artist:DJ'TEKINA//SOMETHING)
        if(curLine.starts_with("//")) continue;

        const char *curLineChar = curLine.c_str();
        if(curLine.find("[General]") != std::string::npos)
            curBlock = 0;
        else if(curLine.find("[Metadata]") != std::string::npos)
            curBlock = 1;
        else if(curLine.find("[Difficulty]") != std::string::npos)
            curBlock = 2;
        else if(curLine.find("[Events]") != std::string::npos)
            curBlock = 3;
        else if(curLine.find("[TimingPoints]") != std::string::npos)
            curBlock = 4;
        else if(curLine.find("[HitObjects]") != std::string::npos)
            break;  // NOTE: stop early

        switch(curBlock) {
            case -1:  // header (e.g. "osu file format v12")
            {
                if(sscanf(curLineChar, " osu file format v %i \n", &this->iVersion) == 1) {
                    if(this->iVersion > cv::beatmap_version.getInt()) {
                        debugLog("Ignoring unknown/invalid beatmap version {:d}\n", this->iVersion);
                        return false;
                    }
                }
            } break;

            case 0:  // General
            {
                memset(stringBuffer, '\0', 1024);
                if(sscanf(curLineChar, " AudioFilename : %1023[^\n]", stringBuffer) == 1) {
                    this->sAudioFileName = stringBuffer;
                    SString::trim(&this->sAudioFileName);
                }

                sscanf(curLineChar, " StackLeniency : %f \n", &this->fStackLeniency);
                sscanf(curLineChar, " PreviewTime : %i \n", &this->iPreviewTime);
                sscanf(curLineChar, " Mode : %i \n", &this->iGameMode);
            } break;

            case 1:  // Metadata
            {
                memset(stringBuffer, '\0', 1024);
                if(sscanf(curLineChar, " Title :%1023[^\n]", stringBuffer) == 1) {
                    this->sTitle = stringBuffer;
                    SString::trim(&this->sTitle);
                }

                memset(stringBuffer, '\0', 1024);
                if(sscanf(curLineChar, " TitleUnicode :%1023[^\n]", stringBuffer) == 1) {
                    this->sTitleUnicode = stringBuffer;
                    SString::trim(&this->sTitle);
                }

                memset(stringBuffer, '\0', 1024);
                if(sscanf(curLineChar, " Artist :%1023[^\n]", stringBuffer) == 1) {
                    this->sArtist = stringBuffer;
                    SString::trim(&this->sArtist);
                }

                memset(stringBuffer, '\0', 1024);
                if(sscanf(curLineChar, " ArtistUnicode :%1023[^\n]", stringBuffer) == 1) {
                    this->sArtistUnicode = stringBuffer;
                    SString::trim(&this->sArtist);
                }

                memset(stringBuffer, '\0', 1024);
                if(sscanf(curLineChar, " Creator :%1023[^\n]", stringBuffer) == 1) {
                    this->sCreator = stringBuffer;
                    SString::trim(&this->sCreator);
                }

                memset(stringBuffer, '\0', 1024);
                if(sscanf(curLineChar, " Version :%1023[^\n]", stringBuffer) == 1) {
                    this->sDifficultyName = stringBuffer;
                    SString::trim(&this->sDifficultyName);
                }

                memset(stringBuffer, '\0', 1024);
                if(sscanf(curLineChar, " Source :%1023[^\n]", stringBuffer) == 1) {
                    this->sSource = stringBuffer;
                    SString::trim(&this->sSource);
                }

                memset(stringBuffer, '\0', 1024);
                if(sscanf(curLineChar, " Tags :%1023[^\n]", stringBuffer) == 1) {
                    this->sTags = stringBuffer;
                    SString::trim(&this->sTags);
                }

                sscanf(curLineChar, " BeatmapID : %ld \n", &this->iID);
                sscanf(curLineChar, " BeatmapSetID : %i \n", &this->iSetID);
            } break;

            case 2:  // Difficulty
            {
                sscanf(curLineChar, " CircleSize : %f \n", &this->fCS);
                if(sscanf(curLineChar, " ApproachRate : %f \n", &this->fAR) == 1) foundAR = true;
                sscanf(curLineChar, " HPDrainRate : %f \n", &this->fHP);
                sscanf(curLineChar, " OverallDifficulty : %f \n", &this->fOD);
                sscanf(curLineChar, " SliderMultiplier : %f \n", &this->fSliderMultiplier);
                sscanf(curLineChar, " SliderTickRate : %f \n", &this->fSliderTickRate);
            } break;

            case 3:  // Events
            {
                memset(stringBuffer, '\0', 1024);
                int type, startTime;
                if(sscanf(curLineChar, R"( %i , %i , "%1023[^"]")", &type, &startTime, stringBuffer) == 3) {
                    if(type == 0) {
                        this->sBackgroundImageFileName = stringBuffer;
                        this->sFullBackgroundImageFilePath = this->sFolder;
                        this->sFullBackgroundImageFilePath.append(this->sBackgroundImageFileName);
                    }
                }
            } break;

            case 4:  // TimingPoints
            {
                // old beatmaps: Offset, Milliseconds per Beat
                // old new beatmaps: Offset, Milliseconds per Beat, Meter, Sample Type, Sample Set, Volume,
                // !Inherited new new beatmaps: Offset, Milliseconds per Beat, Meter, Sample Type, Sample Set,
                // Volume, !Inherited, Kiai Mode

                double tpOffset;
                float tpMSPerBeat;
                int tpMeter;
                int tpSampleType, tpSampleSet;
                int tpVolume;
                int tpTimingChange;
                int tpKiai = 0;  // optional
                if(sscanf(curLineChar, " %lf , %f , %i , %i , %i , %i , %i , %i", &tpOffset, &tpMSPerBeat, &tpMeter,
                          &tpSampleType, &tpSampleSet, &tpVolume, &tpTimingChange, &tpKiai) == 8 ||
                   sscanf(curLineChar, " %lf , %f , %i , %i , %i , %i , %i", &tpOffset, &tpMSPerBeat, &tpMeter,
                          &tpSampleType, &tpSampleSet, &tpVolume, &tpTimingChange) == 7) {
                    DatabaseBeatmap::TIMINGPOINT t{};
                    {
                        t.offset = (long)std::round(tpOffset);
                        t.msPerBeat = tpMSPerBeat;

                        t.sampleType = tpSampleType;
                        t.sampleSet = tpSampleSet;
                        t.volume = tpVolume;

                        t.timingChange = tpTimingChange == 1;
                        t.kiai = tpKiai > 0;
                    }
                    this->timingpoints.push_back(t);
                } else if(sscanf(curLineChar, " %lf , %f", &tpOffset, &tpMSPerBeat) == 2) {
                    DatabaseBeatmap::TIMINGPOINT t{};
                    {
                        t.offset = (long)std::round(tpOffset);
                        t.msPerBeat = tpMSPerBeat;

                        t.sampleType = 0;
                        t.sampleSet = 0;
                        t.volume = 100;

                        t.timingChange = true;
                        t.kiai = false;
                    }
                    this->timingpoints.push_back(t);
                }
            } break;
        }
    }

    if(this->sTitleUnicode.empty()) this->sTitleUnicode = this->sTitle;
    if(this->sArtistUnicode.empty()) this->sArtistUnicode = this->sArtist;

    // gamemode filter
    if(this->iGameMode != 0) return false;  // nothing more to do here

    // general sanity checks
    if((this->timingpoints.size() < 1)) {
        if(cv::debug.getBool()) debugLog("DatabaseBeatmap::loadMetadata() : no timingpoints in beatmap!\n");
        return false;  // nothing more to do here
    }

    // build sound file path
    this->sFullSoundFilePath = this->sFolder;
    this->sFullSoundFilePath.append(this->sAudioFileName);

    // sort timingpoints and calculate BPM range
    if(this->timingpoints.size() > 0) {
        // sort timingpoints by time
        if(this->timingpoints.size() > 1) {
            std::ranges::sort(this->timingpoints, timingPointSortComparator);
        }

        if(this->iMostCommonBPM == 0) {
            if(cv::debug.getBool()) debugLog("DatabaseBeatmap::loadMetadata() : calculating BPM range ...\n");
            auto bpm = getBPM(this->timingpoints);
            this->iMinBPM = bpm.min;
            this->iMaxBPM = bpm.max;
            this->iMostCommonBPM = bpm.most_common;
        }
    }

    // special case: old beatmaps have AR = OD, there is no ApproachRate stored
    if(!foundAR) this->fAR = this->fOD;

    return true;
}

DatabaseBeatmap::LOAD_GAMEPLAY_RESULT DatabaseBeatmap::loadGameplay(DatabaseBeatmap *databaseBeatmap,
                                                                    BeatmapInterface *beatmap) {
    LOAD_GAMEPLAY_RESULT result = LOAD_GAMEPLAY_RESULT();

    // NOTE: reload metadata (force ensures that all necessary data is ready for creating hitobjects and playing etc.,
    // also if beatmap file is changed manually in the meantime)
    // XXX: file io, md5 calc, all on main thread!!
    if(!databaseBeatmap->loadMetadata()) {
        result.errorCode = 1;
        return result;
    }

    // load primitives, put in temporary container
    PRIMITIVE_CONTAINER c = loadPrimitiveObjects(databaseBeatmap->sFilePath);
    if(c.errorCode != 0) {
        result.errorCode = c.errorCode;
        return result;
    }
    result.breaks = std::move(c.breaks);
    result.combocolors = std::move(c.combocolors);

    // override some values with data from primitive load, even though they should already be loaded from metadata
    // (sanity)
    databaseBeatmap->timingpoints.swap(c.timingpoints);
    databaseBeatmap->fSliderMultiplier = c.sliderMultiplier;
    databaseBeatmap->fSliderTickRate = c.sliderTickRate;
    databaseBeatmap->fStackLeniency = c.stackLeniency;
    databaseBeatmap->iVersion = c.version;

    // check if we have any timingpoints at all
    if(databaseBeatmap->timingpoints.size() == 0) {
        result.errorCode = 3;
        return result;
    }

    // update numObjects
    databaseBeatmap->iNumObjects = c.numHitobjects;
    databaseBeatmap->iNumCircles = c.numCircles;
    databaseBeatmap->iNumSliders = c.numSliders;
    databaseBeatmap->iNumSpinners = c.numSpinners;

    // check if we have any hitobjects at all
    if(databaseBeatmap->iNumObjects < 1) {
        result.errorCode = 4;
        return result;
    }

    // calculate sliderTimes, and build slider clicks and ticks
    CALCULATE_SLIDER_TIMES_CLICKS_TICKS_RESULT sliderTimeCalcResult =
        calculateSliderTimesClicksTicks(c.version, c.sliders, databaseBeatmap->timingpoints,
                                        databaseBeatmap->fSliderMultiplier, databaseBeatmap->fSliderTickRate);
    if(sliderTimeCalcResult.errorCode != 0) {
        result.errorCode = sliderTimeCalcResult.errorCode;
        return result;
    }

    // build hitobjects from the primitive data we loaded from the osu file
    {
        // also calculate max possible combo
        int maxPossibleCombo = 0;

        for(auto &h : c.hitcircles) {
            result.hitobjects.push_back(
                new Circle(h.x, h.y, h.time, h.sampleType, h.number, false, h.colorCounter, h.colorOffset, beatmap));
        }
        maxPossibleCombo += c.hitcircles.size();

        for(auto &s : c.sliders) {
            if(cv::mod_strict_tracking.getBool() && cv::mod_strict_tracking_remove_slider_ticks.getBool())
                s.ticks.clear();

            if(cv::mod_reverse_sliders.getBool()) std::ranges::reverse(s.points);

            result.hitobjects.push_back(new Slider(s.type, s.repeat, s.pixelLength, s.points, s.hitSounds, s.ticks,
                                                   s.sliderTime, s.sliderTimeWithoutRepeats, s.time, s.sampleType,
                                                   s.number, false, s.colorCounter, s.colorOffset, beatmap));

            const int repeats = std::max((s.repeat - 1), 0);
            maxPossibleCombo += 2 + repeats + (repeats + 1) * s.ticks.size();  // start/end + repeat arrow + ticks
        }

        for(auto &s : c.spinners) {
            result.hitobjects.push_back(new Spinner(s.x, s.y, s.time, s.sampleType, false, s.endTime, beatmap));
        }
        maxPossibleCombo += c.spinners.size();

        beatmap->iMaxPossibleCombo = maxPossibleCombo;
    }

    // sort hitobjects by starttime
    if(result.hitobjects.size() > 1) {
        std::ranges::sort(result.hitobjects, Beatmap::sortHitObjectByStartTimeComp);
    }

    // update beatmap length stat
    if(databaseBeatmap->iLengthMS == 0 && result.hitobjects.size() > 0)
        databaseBeatmap->iLengthMS = result.hitobjects[result.hitobjects.size() - 1]->click_time +
                                     result.hitobjects[result.hitobjects.size() - 1]->duration;

    // set isEndOfCombo + precalculate Score v2 combo portion maximum
    if(beatmap != NULL) {
        unsigned long long scoreV2ComboPortionMaximum = 1;

        if(result.hitobjects.size() > 0) scoreV2ComboPortionMaximum = 0;

        int combo = 0;
        for(size_t i = 0; i < result.hitobjects.size(); i++) {
            HitObject *currentHitObject = result.hitobjects[i];
            const HitObject *nextHitObject = (i + 1 < result.hitobjects.size() ? result.hitobjects[i + 1] : NULL);

            const Circle *circlePointer = dynamic_cast<Circle *>(currentHitObject);
            const Slider *sliderPointer = dynamic_cast<Slider *>(currentHitObject);
            const Spinner *spinnerPointer = dynamic_cast<Spinner *>(currentHitObject);

            int scoreComboMultiplier = std::max(combo - 1, 0);

            if(circlePointer != NULL || spinnerPointer != NULL) {
                scoreV2ComboPortionMaximum += (unsigned long long)(300.0 * (1.0 + (double)scoreComboMultiplier / 10.0));
                combo++;
            } else if(sliderPointer != NULL) {
                combo += 1 + sliderPointer->getClicks().size();
                scoreComboMultiplier = std::max(combo - 1, 0);
                scoreV2ComboPortionMaximum += (unsigned long long)(300.0 * (1.0 + (double)scoreComboMultiplier / 10.0));
                combo++;
            }

            if(nextHitObject == NULL || nextHitObject->combo_number == 1) {
                currentHitObject->is_end_of_combo = true;
            }
        }

        beatmap->iScoreV2ComboPortionMaximum = scoreV2ComboPortionMaximum;
    }

    // special rule for first hitobject (for 1 approach circle with HD)
    if(cv::show_approach_circle_on_first_hidden_object.getBool()) {
        if(result.hitobjects.size() > 0) result.hitobjects[0]->setForceDrawApproachCircle(true);
    }

    // custom override for forcing a hard number cap and/or sequence (visually only)
    // NOTE: this is done after we have already calculated/set isEndOfCombos
    {
        if(cv::ignore_beatmap_combo_numbers.getBool()) {
            // NOTE: spinners don't increment the combo number
            int comboNumber = 1;
            for(auto currentHitObject : result.hitobjects) {
                const Spinner *spinnerPointer = dynamic_cast<Spinner *>(currentHitObject);

                if(spinnerPointer == NULL) {
                    currentHitObject->combo_number = comboNumber;
                    comboNumber++;
                }
            }
        }

        const int numberMax = cv::number_max.getInt();
        if(numberMax > 0) {
            for(auto currentHitObject : result.hitobjects) {
                const int currentComboNumber = currentHitObject->combo_number;
                const int newComboNumber = (currentComboNumber % numberMax);

                currentHitObject->combo_number = (newComboNumber == 0) ? numberMax : newComboNumber;
            }
        }
    }

    debugLog("DatabaseBeatmap::loadGameplay() loaded {:d} hitobjects\n", result.hitobjects.size());

    return result;
}

MapOverrides DatabaseBeatmap::get_overrides() {
    MapOverrides overrides;
    overrides.local_offset = this->iLocalOffset;
    overrides.online_offset = this->iOnlineOffset;
    overrides.nb_circles = this->iNumCircles;
    overrides.nb_sliders = this->iNumSliders;
    overrides.nb_spinners = this->iNumSpinners;
    overrides.star_rating = this->fStarsNomod;
    overrides.loudness = this->loudness.load();
    overrides.min_bpm = this->iMinBPM;
    overrides.max_bpm = this->iMaxBPM;
    overrides.avg_bpm = this->iMostCommonBPM;
    overrides.draw_background = this->draw_background;
    return overrides;
}

void DatabaseBeatmap::update_overrides() {
    if(this->type != BeatmapType::PEPPY_DIFFICULTY) return;

    // XXX: not actually thread safe, if m_sMD5Hash gets updated by loadGameplay()
    //      or other values in get_overrides()
    db->peppy_overrides_mtx.lock();
    db->peppy_overrides[this->sMD5Hash] = this->get_overrides();
    db->peppy_overrides_mtx.unlock();
}

DatabaseBeatmap::TIMING_INFO DatabaseBeatmap::getTimingInfoForTime(unsigned long positionMS) {
    return getTimingInfoForTimeAndTimingPoints(positionMS, this->timingpoints);
}

DatabaseBeatmap::TIMING_INFO DatabaseBeatmap::getTimingInfoForTimeAndTimingPoints(
    unsigned long positionMS, const zarray<DatabaseBeatmap::TIMINGPOINT> &timingpoints) {
    TIMING_INFO ti{};
    ti.offset = 0;
    ti.beatLengthBase = 1;
    ti.beatLength = 1;
    ti.volume = 100;
    ti.sampleType = 0;
    ti.sampleSet = 0;
    ti.isNaN = false;

    if(timingpoints.size() < 1) return ti;

    // initial values
    ti.offset = timingpoints[0].offset;
    ti.volume = timingpoints[0].volume;
    ti.sampleSet = timingpoints[0].sampleSet;
    ti.sampleType = timingpoints[0].sampleType;

    // new (peppy's algorithm)
    // (correctly handles aspire & NaNs)
    {
        const bool allowMultiplier = true;

        int point = 0;
        int samplePoint = 0;
        int audioPoint = 0;

        for(int i = 0; i < timingpoints.size(); i++) {
            if(timingpoints[i].offset <= positionMS) {
                audioPoint = i;

                if(timingpoints[i].timingChange)
                    point = i;
                else
                    samplePoint = i;
            }
        }

        double mult = 1;

        if(allowMultiplier && samplePoint > point && timingpoints[samplePoint].msPerBeat < 0) {
            if(timingpoints[samplePoint].msPerBeat >= 0)
                mult = 1;
            else
                mult = std::clamp<float>((float)-timingpoints[samplePoint].msPerBeat, 10.0f, 1000.0f) / 100.0f;
        }

        ti.beatLengthBase = timingpoints[point].msPerBeat;
        ti.offset = timingpoints[point].offset;

        ti.isNaN = std::isnan(timingpoints[samplePoint].msPerBeat) || std::isnan(timingpoints[point].msPerBeat);
        ti.beatLength = ti.beatLengthBase * mult;

        ti.volume = timingpoints[audioPoint].volume;
        ti.sampleType = timingpoints[audioPoint].sampleType;
        ti.sampleSet = timingpoints[audioPoint].sampleSet;
    }

    return ti;
}

void DatabaseBeatmapBackgroundImagePathLoader::init() {
    // (nothing)
    this->bReady = true;
}

void DatabaseBeatmapBackgroundImagePathLoader::initAsync() {
    if(this->bInterrupted) return;

    {
        File file(this->sFilePath);
        if(this->bInterrupted || !file.canRead()) return;

        int curBlock = -1;
        char stringBuffer[1024];
        while(file.canRead()) {
            if(this->bInterrupted) {
                return;
            }
            std::string curLine = file.readLine();
            if(!curLine.starts_with("//"))  // ignore comments, but only if at the beginning of a line (e.g. allow
                                            // Artist:DJ'TEKINA//SOMETHING)
            {
                if(curLine.find("[Events]") != std::string::npos)
                    curBlock = 1;
                else if(curLine.find("[TimingPoints]") != std::string::npos)
                    break;  // NOTE: stop early
                else if(curLine.find("[Colours]") != std::string::npos)
                    break;  // NOTE: stop early
                else if(curLine.find("[HitObjects]") != std::string::npos)
                    break;  // NOTE: stop early

                switch(curBlock) {
                    case 1:  // Events
                    {
                        memset(stringBuffer, '\0', 1024);
                        int type, startTime;
                        if(sscanf(curLine.c_str(), R"( %i , %i , "%1023[^"]")", &type, &startTime, stringBuffer) == 3) {
                            if(type == 0) this->sLoadedBackgroundImageFileName = stringBuffer;
                        }
                    } break;
                }
            }
        }
    }

    if(this->bInterrupted) {
        return;
    }

    this->bAsyncReady = true;
    this->bReady = true;  // NOTE: on purpose. there is nothing to do in init(), so finish 1 frame early
}

std::string DatabaseBeatmap::getFullSoundFilePath() {
    // this modifies this->sFullSoundFilePath inplace
    if(File::existsCaseInsensitive(this->sFullSoundFilePath) == File::FILETYPE::FILE) {
        return this->sFullSoundFilePath;
    }

    // wasn't found but return what we have anyways
    return this->sFullSoundFilePath;
}
