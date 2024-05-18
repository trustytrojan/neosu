#pragma once
#include "Replay.h"
#include "cbase.h"

class ConVar;
class DatabaseBeatmap;
class Beatmap;
class HitObject;

struct FinishedScore {
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

    bool isLegacyScore;          // used for identifying loaded osu! scores (which don't have any custom data available)
    bool isImportedLegacyScore;  // used for identifying imported osu! scores (which were previously legacy scores,
                                 // so they don't have any
                                 // numSliderBreaks/unstableRate/hitErrorAvgMin/hitErrorAvgMax)
    u32 version;
    u64 unixTimestamp;

    u32 player_id = 0;
    std::string playerName;
    bool passed = false;
    bool ragequit = false;
    Grade grade = Grade::N;
    DatabaseBeatmap *diff2;
    u64 play_time_ms = 0;

    std::string server;
    u64 online_score_id = 0;
    bool has_replay = false;
    std::vector<Replay::Frame> replay;
    u64 legacyReplayTimestamp = 0;

    int num300s;
    int num100s;
    int num50s;
    int numGekis;
    int numKatus;
    int numMisses;

    unsigned long long score;
    int comboMax;
    bool perfect;
    int modsLegacy;

    // custom
    int numSliderBreaks;
    float pp;
    float unstableRate;
    float hitErrorAvgMin;
    float hitErrorAvgMax;
    float starsTomTotal;
    float starsTomAim;
    float starsTomSpeed;
    float speedMultiplier;
    float CS, AR, OD, HP;
    int maxPossibleCombo;
    int numHitObjects;
    int numCircles;
    std::string experimentalModsConVars;

    // runtime
    unsigned long long sortHack;
    MD5Hash md5hash;

    bool isLegacyScoreEqualToImportedLegacyScore(const FinishedScore &importedLegacyScore) const {
        if(!isLegacyScore) return false;
        if(!importedLegacyScore.isImportedLegacyScore) return false;

        const bool isScoreValueEqual = (score == importedLegacyScore.score);
        const bool isTimestampEqual = (unixTimestamp == importedLegacyScore.unixTimestamp);
        const bool isComboMaxEqual = (comboMax == importedLegacyScore.comboMax);
        const bool isModsLegacyEqual = (modsLegacy == importedLegacyScore.modsLegacy);
        const bool isNum300sEqual = (num300s == importedLegacyScore.num300s);
        const bool isNum100sEqual = (num100s == importedLegacyScore.num100s);
        const bool isNum50sEqual = (num50s == importedLegacyScore.num50s);
        const bool isNumGekisEqual = (numGekis == importedLegacyScore.numGekis);
        const bool isNumKatusEqual = (numKatus == importedLegacyScore.numKatus);
        const bool isNumMissesEqual = (numMisses == importedLegacyScore.numMisses);

        return (isScoreValueEqual && isTimestampEqual && isComboMaxEqual && isModsLegacyEqual && isNum300sEqual &&
                isNum100sEqual && isNum50sEqual && isNumGekisEqual && isNumKatusEqual && isNumMissesEqual);
    }

    bool isScoreEqualToCopiedScoreIgnoringPlayerName(const FinishedScore &copiedScore) const {
        const bool isScoreValueEqual = (score == copiedScore.score);
        const bool isTimestampEqual = (unixTimestamp == copiedScore.unixTimestamp);
        const bool isComboMaxEqual = (comboMax == copiedScore.comboMax);
        const bool isModsLegacyEqual = (modsLegacy == copiedScore.modsLegacy);
        const bool isNum300sEqual = (num300s == copiedScore.num300s);
        const bool isNum100sEqual = (num100s == copiedScore.num100s);
        const bool isNum50sEqual = (num50s == copiedScore.num50s);
        const bool isNumGekisEqual = (numGekis == copiedScore.numGekis);
        const bool isNumKatusEqual = (numKatus == copiedScore.numKatus);
        const bool isNumMissesEqual = (numMisses == copiedScore.numMisses);

        const bool isSpeedMultiplierEqual = (speedMultiplier == copiedScore.speedMultiplier);
        const bool isCSEqual = (CS == copiedScore.CS);
        const bool isAREqual = (AR == copiedScore.AR);
        const bool isODEqual = (OD == copiedScore.OD);
        const bool isHPEqual = (HP == copiedScore.HP);
        const bool areExperimentalModsConVarsEqual = (experimentalModsConVars == copiedScore.experimentalModsConVars);

        return (isScoreValueEqual && isTimestampEqual && isComboMaxEqual && isModsLegacyEqual && isNum300sEqual &&
                isNum100sEqual && isNum50sEqual && isNumGekisEqual && isNumKatusEqual && isNumMissesEqual

                && isSpeedMultiplierEqual && isCSEqual && isAREqual && isODEqual && isHPEqual &&
                areExperimentalModsConVarsEqual);
    }
};

class LiveScore {
   public:
    static const u32 VERSION = 20240412;

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
    static FinishedScore::Grade calculateGrade(int num300s, int num100s, int num50s, int numMisses, bool modHidden,
                                               bool modFlashlight);

   public:
    LiveScore();

    void reset();  // only Beatmap may call this function!

    void addHitResult(Beatmap *beatmap, HitObject *hitObject, LiveScore::HIT hit, long delta, bool ignoreOnHitErrorBar,
                      bool hitErrorBarOnly, bool ignoreCombo,
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

    static double getHealthIncrease(Beatmap *beatmap, LiveScore::HIT hit);
    static double getHealthIncrease(LiveScore::HIT hit, double HP = 5.0f, double hpMultiplierNormal = 1.0f,
                                    double hpMultiplierComboEnd = 1.0f, double hpBarMaximumForNormalization = 200.0f);

    int getKeyCount(int key);
    int getModsLegacy();
    UString getModsStringForRichPresence();

   private:
    static ConVar *m_osu_draw_statistics_pp_ref;
    static ConVar *m_osu_drain_type_ref;

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
