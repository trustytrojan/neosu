#pragma once
#include <mutex>

#include "BanchoProtocol.h"  // Packet
#include "LegacyReplay.h"
#include "Overrides.h"
#include "UString.h"
#include "cbase.h"
#include "score.h"

namespace Timing
{
    class Timer;
}
class ConVar;

class OsuFile;
class DatabaseBeatmap;
class DatabaseLoader;
typedef DatabaseBeatmap BeatmapDifficulty;
typedef DatabaseBeatmap BeatmapSet;

#define NEOSU_MAPS_DB_VERSION 20240812
#define NEOSU_SCORE_DB_VERSION 20240725

// Field ordering matters here
#pragma pack(push, 1)
struct TIMINGPOINT {
    double msPerBeat;
    double offset;
    bool timingChange;
};
#pragma pack(pop)

class Database {
   public:
    struct PlayerStats {
        UString name;
        float pp;
        float accuracy;
        int numScoresWithPP;
        int level;
        float percentToNextLevel;
        unsigned long long totalScore;
    };

    struct PlayerPPScores {
        std::vector<FinishedScore *> ppScores;
        unsigned long long totalScore;
    };

    struct SCORE_SORTING_COMPARATOR {
        virtual ~SCORE_SORTING_COMPARATOR() { ; }
        virtual bool operator()(FinishedScore const &a, FinishedScore const &b) const = 0;
    };

    struct SCORE_SORTING_METHOD {
        UString name;
        SCORE_SORTING_COMPARATOR *comparator;
    };

   public:
    Database();
    ~Database();

    void load();
    void cancel();
    void save();

    BeatmapSet *addBeatmapSet(std::string beatmapFolderPath, i32 set_id_override = -1);

    int addScore(FinishedScore score);
    void deleteScore(MD5Hash beatmapMD5Hash, u64 scoreUnixTimestamp);
    void sortScoresInPlace(std::vector<FinishedScore> &scores);
    void sortScores(MD5Hash beatmapMD5Hash);
    void forceScoreUpdateOnNextCalculatePlayerStats() { this->bDidScoresChangeForStats = true; }

    std::vector<UString> getPlayerNamesWithPPScores();
    std::vector<UString> getPlayerNamesWithScoresForUserSwitcher();
    PlayerPPScores getPlayerPPScores(UString playerName);
    PlayerStats calculatePlayerStats(UString playerName);
    static float getWeightForIndex(int i);
    static float getBonusPPForNumScores(size_t numScores);
    static unsigned long long getRequiredScoreForLevel(int level);
    static int getLevelForScore(unsigned long long score, int maxLevel = 120);

    inline float getProgress() const { return this->fLoadingProgress.load(); }
    inline bool isLoading() const {
        float progress = this->getProgress();
        return progress > 0.f && progress < 1.f;
    }
    inline bool isFinished() const { return (this->getProgress() >= 1.0f); }
    inline bool foundChanges() const { return this->bFoundChanges; }

    inline const std::vector<DatabaseBeatmap *> getDatabaseBeatmaps() const { return this->beatmapsets; }
    DatabaseBeatmap *getBeatmapDifficulty(const MD5Hash &md5hash);
    DatabaseBeatmap *getBeatmapDifficulty(i32 map_id);
    DatabaseBeatmap *getBeatmapSet(i32 set_id);

    inline std::unordered_map<MD5Hash, std::vector<FinishedScore>> *getScores() { return &this->scores; }
    inline const std::vector<SCORE_SORTING_METHOD> &getScoreSortingMethods() const { return this->scoreSortingMethods; }

    std::unordered_map<MD5Hash, std::vector<FinishedScore>> online_scores;
    std::string getOsuSongsFolder();

    BeatmapSet *loadRawBeatmap(std::string beatmapPath);  // only used for raw loading without db

    void loadDB();
    std::mutex peppy_overrides_mtx;
    std::unordered_map<MD5Hash, MapOverrides> peppy_overrides;
    std::vector<BeatmapDifficulty *> maps_to_recalc;
    std::vector<BeatmapDifficulty *> loudness_to_calc;
    std::vector<FinishedScore> scores_to_convert;

    std::mutex scores_mtx;
    std::unordered_map<MD5Hash, std::vector<FinishedScore>> scores;

   private:
    friend class DatabaseLoader;

    void saveMaps();

    void loadScores();
    u32 importOldNeosuScores();
    u32 importPeppyScores();
    void saveScores();
    bool addScoreRaw(const FinishedScore &score);

    Timing::Timer *importTimer;
    bool bIsFirstLoad;   // only load differences after first raw load
    bool bFoundChanges;  // for total refresh detection of raw loading

    // global
    int iNumBeatmapsToLoad;
    std::atomic<float> fLoadingProgress;
    std::atomic<bool> bInterruptLoad;
    std::vector<BeatmapSet *> beatmapsets;
    std::vector<BeatmapSet *> neosu_sets;

    std::mutex beatmap_difficulties_mtx;
    std::unordered_map<MD5Hash, BeatmapDifficulty *> beatmap_difficulties;

    bool neosu_maps_loaded = false;

    // osu!.db
    int iVersion;
    int iFolderCount;

    // scores.db (legacy and custom)
    bool bScoresLoaded = false;

    bool bDidScoresChangeForStats;
    PlayerStats prevPlayerStats;
    std::vector<SCORE_SORTING_METHOD> scoreSortingMethods;
};

extern Database *db;
