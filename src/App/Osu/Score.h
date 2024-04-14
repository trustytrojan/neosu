#pragma once
#include "OsuReplay.h"
#include "cbase.h"

class OsuDatabaseBeatmap;

struct Score {
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
    uint32_t version;
    uint64_t unixTimestamp;

    uint32_t player_id = 0;
    std::string playerName;
    bool passed = false;
    bool ragequit = false;
    Grade grade = Grade::N;
    OsuDatabaseBeatmap *diff2;
    uint64_t play_time_ms = 0;

    std::string server;
    uint64_t online_score_id = 0;
    bool has_replay = false;
    std::vector<OsuReplay::Frame> replay;
    uint64_t legacyReplayTimestamp = 0;

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

    bool isLegacyScoreEqualToImportedLegacyScore(const Score &importedLegacyScore) const {
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

    bool isScoreEqualToCopiedScoreIgnoringPlayerName(const Score &copiedScore) const {
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
