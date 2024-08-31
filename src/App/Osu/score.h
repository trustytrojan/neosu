#pragma once
#include "Replay.h"
#include "cbase.h"

class ConVar;
class DatabaseBeatmap;
class BeatmapInterface;
class HitObject;

struct FinishedScore {
    u64 score = 0;
    u64 spinner_bonus = 0;
    Replay::Mods mods;

    u64 unixTimestamp = 0;
    u32 player_id = 0;
    std::string playerName;
    DatabaseBeatmap *diff2 = NULL;
    u64 play_time_ms = 0;

    enum class Grade {
        XH,
        SH,
        X,
        S,
        A,
        B,
        C,
        D,
        F,
        N  // means "no grade"
    };
    Grade grade = Grade::N;

    std::string client;
    std::string server;
    u64 bancho_score_id = 0;
    u64 peppy_replay_tms = 0;  // online scores don't have peppy_replay_tms

    int num300s = 0;
    int num100s = 0;
    int num50s = 0;
    int numGekis = 0;
    int numKatus = 0;
    int numMisses = 0;

    int comboMax = 0;
    bool perfect = false;
    bool passed = false;
    bool ragequit = false;

    u32 ppv2_version = 0;
    float ppv2_score = 0.f;
    float ppv2_total_stars = 0.f;
    float ppv2_aim_stars = 0.f;
    float ppv2_speed_stars = 0.f;

    std::string ppv3_algorithm;
    float ppv3_score = 0.f;

    // Absolute hit deltas of every hitobject (0-254). 255 == miss
    // This is exclusive to PPV3-converted scores
    std::vector<u8> hitdeltas;

    int numSliderBreaks = 0;
    float unstableRate = 0.f;
    float hitErrorAvgMin = 0.f;
    float hitErrorAvgMax = 0.f;
    int maxPossibleCombo = -1;
    int numHitObjects = -1;
    int numCircles = -1;

    u64 sortHack;
    MD5Hash beatmap_hash;
    std::vector<LegacyReplay::Frame> replay;  // not always loaded

    bool is_peppy_imported() { return bancho_score_id != 0 || peppy_replay_tms != 0; }
    f64 get_pp() const;
    Grade calculate_grade() const;
};

class LiveScore {
   public:
    enum class HIT {
        // score
        HIT_NULL,
        HIT_MISS,
        HIT_50,
        HIT_100,
        HIT_300,

        // only used for health + SS/PF mods
        HIT_MISS_SLIDERBREAK,
        HIT_MU,
        HIT_100K,
        HIT_300K,
        HIT_300G,
        HIT_SLIDER10,  // tick
        HIT_SLIDER30,  // repeat
        HIT_SPINNERSPIN,
        HIT_SPINNERBONUS
    };

    static float calculateAccuracy(int num300s, int num100s, int num50s, int numMisses);

   public:
    LiveScore();

    void reset();  // only Beatmap may call this function!

    void addHitResult(BeatmapInterface *beatmap, HitObject *hitObject, LiveScore::HIT hit, long delta,
                      bool ignoreOnHitErrorBar, bool hitErrorBarOnly, bool ignoreCombo,
                      bool ignoreScore);  // only Beatmap may call this function!
    void addHitResultComboEnd(LiveScore::HIT hit);
    void addSliderBreak();  // only Beatmap may call this function!
    void addPoints(int points, bool isSpinner);
    void setComboFull(int comboFull) { m_iComboFull = comboFull; }
    void setComboEndBitmask(int comboEndBitmask) { m_iComboEndBitmask = comboEndBitmask; }
    void setDead(bool dead);

    void addKeyCount(int key);

    void setStarsTomTotal(float starsTomTotal) { m_fStarsTomTotal = starsTomTotal; }
    void setStarsTomAim(float starsTomAim) { m_fStarsTomAim = starsTomAim; }
    void setStarsTomSpeed(float starsTomSpeed) { m_fStarsTomSpeed = starsTomSpeed; }
    void setPPv2(float ppv2) { m_fPPv2 = ppv2; }
    void setIndex(int index) { m_iIndex = index; }

    void setNumEZRetries(int numEZRetries) { m_iNumEZRetries = numEZRetries; }

    inline float getStarsTomTotal() const { return m_fStarsTomTotal; }
    inline float getStarsTomAim() const { return m_fStarsTomAim; }
    inline float getStarsTomSpeed() const { return m_fStarsTomSpeed; }
    inline float getPPv2() const { return m_fPPv2; }
    inline int getIndex() const { return m_iIndex; }

    unsigned long long getScore();
    inline FinishedScore::Grade getGrade() const { return m_grade; }
    inline int getCombo() const { return m_iCombo; }
    inline int getComboMax() const { return m_iComboMax; }
    inline int getComboFull() const { return m_iComboFull; }
    inline int getComboEndBitmask() const { return m_iComboEndBitmask; }
    inline float getAccuracy() const { return m_fAccuracy; }
    inline float getUnstableRate() const { return m_fUnstableRate; }
    inline float getHitErrorAvgMin() const { return m_fHitErrorAvgMin; }
    inline float getHitErrorAvgMax() const { return m_fHitErrorAvgMax; }
    inline float getHitErrorAvgCustomMin() const { return m_fHitErrorAvgCustomMin; }
    inline float getHitErrorAvgCustomMax() const { return m_fHitErrorAvgCustomMax; }
    inline int getNumMisses() const { return m_iNumMisses; }
    inline int getNumSliderBreaks() const { return m_iNumSliderBreaks; }
    inline int getNum50s() const { return m_iNum50s; }
    inline int getNum100s() const { return m_iNum100s; }
    inline int getNum100ks() const { return m_iNum100ks; }
    inline int getNum300s() const { return m_iNum300s; }
    inline int getNum300gs() const { return m_iNum300gs; }

    inline int getNumEZRetries() const { return m_iNumEZRetries; }

    inline bool isDead() const { return m_bDead; }
    inline bool hasDied() const { return m_bDied; }

    inline bool isUnranked() const { return m_bIsUnranked; }
    void setCheated() { m_bIsUnranked = true; }

    static double getHealthIncrease(BeatmapInterface *beatmap, LiveScore::HIT hit);
    static double getHealthIncrease(LiveScore::HIT hit, double HP = 5.0f, double hpMultiplierNormal = 1.0f,
                                    double hpMultiplierComboEnd = 1.0f, double hpBarMaximumForNormalization = 200.0f);

    int getKeyCount(int key);
    Replay::Mods getMods();
    u32 getModsLegacy();
    UString getModsStringForRichPresence();

   private:
    void onScoreChange();

    std::vector<HIT> m_hitresults;
    std::vector<int> m_hitdeltas;

    FinishedScore::Grade m_grade;

    float m_fStarsTomTotal;
    float m_fStarsTomAim;
    float m_fStarsTomSpeed;
    float m_fPPv2;
    int m_iIndex;

    unsigned long long m_iScoreV1;
    unsigned long long m_iScoreV2;
    unsigned long long m_iScoreV2ComboPortion;
    unsigned long long m_iBonusPoints;
    int m_iCombo;
    int m_iComboMax;
    int m_iComboFull;
    int m_iComboEndBitmask;
    float m_fAccuracy;
    float m_fHitErrorAvgMin;
    float m_fHitErrorAvgMax;
    float m_fHitErrorAvgCustomMin;
    float m_fHitErrorAvgCustomMax;
    float m_fUnstableRate;

    int m_iNumMisses;
    int m_iNumSliderBreaks;
    int m_iNum50s;
    int m_iNum100s;
    int m_iNum100ks;
    int m_iNum300s;
    int m_iNum300gs;

    bool m_bDead;
    bool m_bDied;

    int m_iNumK1;
    int m_iNumK2;
    int m_iNumM1;
    int m_iNumM2;

    // custom
    int m_iNumEZRetries;
    bool m_bIsUnranked;
};
