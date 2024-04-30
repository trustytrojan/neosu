#pragma once
#include "DifficultyCalculator.h"
#include "Osu.h"
#include "Resource.h"

class Osu;
class Beatmap;
class HitObject;

class Database;

class BackgroundImageHandler;

// purpose:
// 1) contain all infos which are ALWAYS kept in memory for beatmaps
// 2) be the data source for Beatmap when starting a difficulty
// 3) allow async calculations/loaders to work on the contained data (e.g. background image loader)
// 4) be a container for difficulties (all top level DatabaseBeatmap objects are containers)

class DatabaseBeatmap {
   public:
    // raw structs

    struct TIMINGPOINT {
        long offset;

        float msPerBeat;

        int sampleType;
        int sampleSet;
        int volume;

        bool timingChange;
        bool kiai;

        unsigned long long sortHack;
    };

    struct BREAK {
        int startTime;
        int endTime;
    };

    // custom structs

    struct LOAD_DIFFOBJ_RESULT {
        int errorCode;

        std::vector<OsuDifficultyHitObject> diffobjects;

        int maxPossibleCombo;

        LOAD_DIFFOBJ_RESULT() {
            errorCode = 0;

            maxPossibleCombo = 0;
        }
    };

    struct LOAD_GAMEPLAY_RESULT {
        int errorCode;

        std::vector<HitObject *> hitobjects;
        std::vector<BREAK> breaks;
        std::vector<Color> combocolors;

        int randomSeed;

        LOAD_GAMEPLAY_RESULT() {
            errorCode = 0;

            randomSeed = 0;
        }
    };

    struct TIMING_INFO {
        long offset;

        float beatLengthBase;
        float beatLength;

        float volume;
        int sampleType;
        int sampleSet;

        bool isNaN;
    };

    DatabaseBeatmap(Osu *osu, std::string filePath, std::string folder, bool filePathIsInMemoryBeatmap = false);
    DatabaseBeatmap(Osu *osu, std::vector<DatabaseBeatmap *> *difficulties);
    ~DatabaseBeatmap();

    static LOAD_DIFFOBJ_RESULT loadDifficultyHitObjects(const std::string &osuFilePath, float AR, float CS,
                                                        float speedMultiplier, bool calculateStarsInaccurately = false);
    static LOAD_DIFFOBJ_RESULT loadDifficultyHitObjects(const std::string &osuFilePath, float AR, float CS,
                                                        float speedMultiplier, bool calculateStarsInaccurately,
                                                        const std::atomic<bool> &dead);
    static bool loadMetadata(DatabaseBeatmap *databaseBeatmap);
    static LOAD_GAMEPLAY_RESULT loadGameplay(DatabaseBeatmap *databaseBeatmap, Beatmap *beatmap);

    void setDifficulties(std::vector<DatabaseBeatmap *> *difficulties);

    void setLengthMS(unsigned long lengthMS) { m_iLengthMS = lengthMS; }

    void setStarsNoMod(float starsNoMod) { m_fStarsNomod = starsNoMod; }

    void setNumObjects(int numObjects) { m_iNumObjects = numObjects; }
    void setNumCircles(int numCircles) { m_iNumCircles = numCircles; }
    void setNumSliders(int numSliders) { m_iNumSliders = numSliders; }
    void setNumSpinners(int numSpinners) { m_iNumSpinners = numSpinners; }

    void setLocalOffset(long localOffset) { m_iLocalOffset = localOffset; }

    inline Osu *getOsu() const { return m_osu; }

    inline std::string getFolder() const { return m_sFolder; }
    inline std::string getFilePath() const { return m_sFilePath; }

    inline unsigned long long getSortHack() const { return m_iSortHack; }

    inline const std::vector<DatabaseBeatmap *> &getDifficulties() const { return *m_difficulties; }

    inline const MD5Hash &getMD5Hash() const { return m_sMD5Hash; }

    TIMING_INFO getTimingInfoForTime(unsigned long positionMS);
    static TIMING_INFO getTimingInfoForTimeAndTimingPoints(unsigned long positionMS,
                                                           const zarray<TIMINGPOINT> &timingpoints);

    // raw metadata

    inline int getVersion() const { return m_iVersion; }
    inline int getGameMode() const { return m_iGameMode; }
    inline int getID() const { return m_iID; }
    inline int getSetID() const { return m_iSetID; }

    inline const std::string &getTitle() const { return m_sTitle; }
    inline const std::string &getArtist() const { return m_sArtist; }
    inline const std::string &getCreator() const { return m_sCreator; }
    inline const std::string &getDifficultyName() const { return m_sDifficultyName; }
    inline const std::string &getSource() const { return m_sSource; }
    inline const std::string &getTags() const { return m_sTags; }
    inline const std::string &getBackgroundImageFileName() const { return m_sBackgroundImageFileName; }
    inline const std::string &getAudioFileName() const { return m_sAudioFileName; }

    inline unsigned long getLengthMS() const { return m_iLengthMS; }
    inline int getPreviewTime() const { return m_iPreviewTime; }

    inline float getAR() const { return m_fAR; }
    inline float getCS() const { return m_fCS; }
    inline float getHP() const { return m_fHP; }
    inline float getOD() const { return m_fOD; }

    inline float getStackLeniency() const { return m_fStackLeniency; }
    inline float getSliderTickRate() const { return m_fSliderTickRate; }
    inline float getSliderMultiplier() const { return m_fSliderMultiplier; }

    inline const zarray<TIMINGPOINT> &getTimingpoints() const { return m_timingpoints; }

    std::string getFullSoundFilePath();

    // redundant data
    inline const std::string &getFullBackgroundImageFilePath() const { return m_sFullBackgroundImageFilePath; }

    // precomputed data

    inline float getStarsNomod() const { return m_fStarsNomod; }

    inline int getMinBPM() const { return m_iMinBPM; }
    inline int getMaxBPM() const { return m_iMaxBPM; }
    inline int getMostCommonBPM() const { return m_iMostCommonBPM; }

    inline int getNumObjects() const { return m_iNumObjects; }
    inline int getNumCircles() const { return m_iNumCircles; }
    inline int getNumSliders() const { return m_iNumSliders; }
    inline int getNumSpinners() const { return m_iNumSpinners; }

    // custom data

    long long last_modification_time;

    inline long getLocalOffset() const { return m_iLocalOffset; }
    inline long getOnlineOffset() const { return m_iOnlineOffset; }

    bool do_not_store = false;

   private:
    // raw metadata

    int m_iVersion;   // e.g. "osu file format v12" -> 12
    int m_iGameMode;  // 0 = osu!standard, 1 = Taiko, 2 = Catch the Beat, 3 = osu!mania
    long m_iID;       // online ID, if uploaded
    int m_iSetID;     // online set ID, if uploaded

    std::string m_sTitle;
    std::string m_sArtist;
    std::string m_sCreator;
    std::string m_sDifficultyName;  // difficulty name ("Version")
    std::string m_sSource;          // only used by search
    std::string m_sTags;            // only used by search
    std::string m_sBackgroundImageFileName;
    std::string m_sAudioFileName;

    unsigned long m_iLengthMS;
    int m_iPreviewTime;

    float m_fAR;
    float m_fCS;
    float m_fHP;
    float m_fOD;

    float m_fStackLeniency;
    float m_fSliderTickRate;
    float m_fSliderMultiplier;

    zarray<TIMINGPOINT> m_timingpoints;  // necessary for main menu anim

    // redundant data (technically contained in metadata, but precomputed anyway)

    std::string m_sFullSoundFilePath;
    std::string m_sFullBackgroundImageFilePath;

    // precomputed data (can-run-without-but-nice-to-have data)

    float m_fStarsNomod;

    int m_iMinBPM = 0;
    int m_iMaxBPM = 0;
    int m_iMostCommonBPM = 0;

    int m_iNumObjects;
    int m_iNumCircles;
    int m_iNumSliders;
    int m_iNumSpinners;

    // custom data (not necessary, not part of the beatmap file, and not precomputed)

    long m_iLocalOffset;
    long m_iOnlineOffset;

    // primitive objects

    struct HITCIRCLE {
        int x, y;
        unsigned long time;
        int sampleType;
        int number;
        int colorCounter;
        int colorOffset;
        bool clicked;
    };

    struct SLIDER {
        int x, y;
        char type;
        int repeat;
        float pixelLength;
        long time;
        int sampleType;
        int number;
        int colorCounter;
        int colorOffset;
        std::vector<Vector2> points;
        std::vector<int> hitSounds;

        float sliderTime;
        float sliderTimeWithoutRepeats;
        std::vector<float> ticks;

        std::vector<OsuDifficultyHitObject::SLIDER_SCORING_TIME> scoringTimesForStarCalc;
    };

    struct SPINNER {
        int x, y;
        unsigned long time;
        int sampleType;
        unsigned long endTime;
    };

    struct PRIMITIVE_CONTAINER {
        int errorCode;

        std::vector<HITCIRCLE> hitcircles;
        std::vector<SLIDER> sliders;
        std::vector<SPINNER> spinners;
        std::vector<BREAK> breaks;

        zarray<TIMINGPOINT> timingpoints;
        std::vector<Color> combocolors;

        float stackLeniency;

        float sliderMultiplier;
        float sliderTickRate;

        int version;
    };

    struct CALCULATE_SLIDER_TIMES_CLICKS_TICKS_RESULT {
        int errorCode;
    };

    // class internal data (custom)

    friend class Database;
    friend class BackgroundImageHandler;
    friend class DatabaseBeatmapStarCalculator;

    static unsigned long long sortHackCounter;

    static ConVar *m_osu_slider_curve_max_length_ref;
    static ConVar *m_osu_stars_xexxar_angles_sliders_ref;
    static ConVar *m_osu_stars_stacking_ref;
    static ConVar *m_osu_debug_pp_ref;
    static ConVar *m_osu_slider_end_inside_check_offset_ref;

    static PRIMITIVE_CONTAINER loadPrimitiveObjects(const std::string &osuFilePath,
                                                    bool filePathIsInMemoryBeatmap = false);
    static PRIMITIVE_CONTAINER loadPrimitiveObjects(const std::string &osuFilePath, bool filePathIsInMemoryBeatmap,
                                                    const std::atomic<bool> &dead);
    static CALCULATE_SLIDER_TIMES_CLICKS_TICKS_RESULT calculateSliderTimesClicksTicks(int beatmapVersion,
                                                                                      std::vector<SLIDER> &sliders,
                                                                                      zarray<TIMINGPOINT> &timingpoints,
                                                                                      float sliderMultiplier,
                                                                                      float sliderTickRate);
    static CALCULATE_SLIDER_TIMES_CLICKS_TICKS_RESULT calculateSliderTimesClicksTicks(
        int beatmapVersion, std::vector<SLIDER> &sliders, zarray<TIMINGPOINT> &timingpoints, float sliderMultiplier,
        float sliderTickRate, const std::atomic<bool> &dead);

    Osu *m_osu;

    std::string m_sFolder;    // path to folder containing .osu file (e.g. "/path/to/beatmapfolder/")
    std::string m_sFilePath;  // path to .osu file (e.g. "/path/to/beatmapfolder/beatmap.osu")
    bool m_bFilePathIsInMemoryBeatmap;

    unsigned long long m_iSortHack;

    std::vector<DatabaseBeatmap *> *m_difficulties = nullptr;

    MD5Hash m_sMD5Hash;

    // helper functions

    struct TimingPointSortComparator {
        bool operator()(DatabaseBeatmap::TIMINGPOINT const &a, DatabaseBeatmap::TIMINGPOINT const &b) const {
            // first condition: offset
            // second condition: if offset is the same, non-inherited timingpoints go before inherited timingpoints

            // strict weak ordering!
            if(a.offset == b.offset && ((a.msPerBeat >= 0 && b.msPerBeat < 0) == (b.msPerBeat >= 0 && a.msPerBeat < 0)))
                return a.sortHack < b.sortHack;
            else
                return (a.offset < b.offset) || (a.offset == b.offset && a.msPerBeat >= 0 && b.msPerBeat < 0);
        }
    };
};

class DatabaseBeatmapBackgroundImagePathLoader : public Resource {
   public:
    DatabaseBeatmapBackgroundImagePathLoader(const std::string &filePath);

    inline const std::string &getLoadedBackgroundImageFileName() const { return m_sLoadedBackgroundImageFileName; }

   private:
    virtual void init();
    virtual void initAsync();
    virtual void destroy() { ; }

    std::string m_sFilePath;
    std::string m_sLoadedBackgroundImageFileName;
};

class DatabaseBeatmapStarCalculator : public Resource {
   public:
    DatabaseBeatmapStarCalculator();

    bool isDead() const { return m_bDead.load(); }
    void kill() { m_bDead = true; }
    void revive() { m_bDead = false; }

    void setBeatmapDifficulty(DatabaseBeatmap *diff2, float AR, float CS, float OD, float speedMultiplier, bool relax,
                              bool touchDevice);

    inline DatabaseBeatmap *getBeatmapDifficulty() const { return m_diff2; }

    inline double getTotalStars() const { return m_totalStars.load(); }
    inline double getAimStars() const { return m_aimStars.load(); }
    inline double getSpeedStars() const { return m_speedStars.load(); }
    inline double getPPv2() const { return m_pp.load(); }  // NOTE: pp with currently active mods (runtime mods)

    inline long getLengthMS() const { return m_iLengthMS.load(); }

    inline const std::vector<double> &getAimStrains() const { return m_aimStrains; }
    inline const std::vector<double> &getSpeedStrains() const { return m_speedStrains; }

    inline int getNumObjects() const { return m_iNumObjects.load(); }
    inline int getNumCircles() const { return m_iNumCircles.load(); }
    inline int getNumSpinners() const { return m_iNumSpinners.load(); }

   private:
    virtual void init();
    virtual void initAsync();
    virtual void destroy() { ; }

    std::atomic<bool> m_bDead;

    DatabaseBeatmap *m_diff2;

    float m_fAR;
    float m_fCS;
    float m_fOD;
    float m_fSpeedMultiplier;
    bool m_bRelax;
    bool m_bTouchDevice;

    std::atomic<double> m_totalStars;
    std::atomic<double> m_aimStars;
    std::atomic<double> m_aimSliderFactor;
    std::atomic<double> m_speedStars;
    std::atomic<double> m_speedNotes;
    std::atomic<double> m_pp;

    std::atomic<long> m_iLengthMS;

    std::vector<double> m_aimStrains;
    std::vector<double> m_speedStrains;

    // custom
    int m_iErrorCode;
    std::atomic<int> m_iNumObjects;
    std::atomic<int> m_iNumCircles;
    std::atomic<int> m_iNumSliders;
    std::atomic<int> m_iNumSpinners;
    int m_iMaxPossibleCombo;
};

struct BPMInfo {
    i32 min;
    i32 max;
    i32 most_common;
};

template <typename T>
struct BPMInfo getBPM(const zarray<T> &timing_points) {
    if(timing_points.empty()) {
        return BPMInfo{
            .min = 0,
            .max = 0,
            .most_common = 0,
        };
    }

    struct Tuple {
        i32 bpm;
        i32 duration;
    };

    zarray<Tuple> bpms;
    bpms.reserve(timing_points.size());

    long lastTime = timing_points[timing_points.size() - 1].offset;
    for(size_t i = 0; i < timing_points.size(); i++) {
        const T &t = timing_points[i];
        if(t.offset > lastTime) continue;
        if(t.msPerBeat < 0) continue;

        // "osu-stable forced the first control point to start at 0."
        // "This is reproduced here to maintain compatibility around osu!mania scroll speed and song
        // select display."
        const long currentTime = (i == 0 ? 0 : t.offset);
        const long nextTime = (i == timing_points.size() - 1 ? lastTime : timing_points[i + 1].offset);

        i32 bpm = t.msPerBeat / 60000;
        i32 duration = std::max(nextTime - currentTime, (long)0);

        bool found = false;
        for(auto tuple : bpms) {
            if(tuple.bpm == bpm) {
                tuple.duration += duration;
                found = true;
                break;
            }
        }

        if(!found) {
            bpms.push_back(Tuple{
                .bpm = bpm,
                .duration = duration,
            });
        }
    }

    i32 min = 9001;
    i32 max = 0;
    i32 mostCommonBPM = 0;
    i32 longestDuration = 0;
    for(auto tuple : bpms) {
        if(tuple.bpm > max) max = tuple.bpm;
        if(tuple.bpm < min) min = tuple.bpm;

        if(tuple.duration > longestDuration) {
            longestDuration = tuple.duration;
            mostCommonBPM = tuple.bpm;
        }
    }

    return BPMInfo{
        .min = min,
        .max = max,
        .most_common = mostCommonBPM,
    };
}
