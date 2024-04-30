#pragma once
#include "BanchoProtocol.h"  // Packet
#include "Replay.h"
#include "UString.h"
#include "cbase.h"
#include "score.h"

class Timer;
class ConVar;

class Osu;
class OsuFile;
class DatabaseBeatmap;
class DatabaseLoader;

struct TIMINGPOINT {
    double msPerBeat;
    double offset;
    bool timingChange;
};
TIMINGPOINT read_timing_point(Packet *packet);

Packet load_db(std::string path);
bool save_db(Packet *db, std::string path);

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
    Database(Osu *osu);
    ~Database();

    void update();

    void load();
    void cancel();
    void save();

    DatabaseBeatmap *addBeatmap(std::string beatmapFolderPath);

    int addScore(MD5Hash beatmapMD5Hash, FinishedScore score);
    void deleteScore(MD5Hash beatmapMD5Hash, u64 scoreUnixTimestamp);
    void sortScores(MD5Hash beatmapMD5Hash);
    void forceScoreUpdateOnNextCalculatePlayerStats() { m_bDidScoresChangeForStats = true; }
    void forceScoresSaveOnNextShutdown() { m_bDidScoresChangeForSave = true; }

    std::vector<UString> getPlayerNamesWithPPScores();
    std::vector<UString> getPlayerNamesWithScoresForUserSwitcher();
    PlayerPPScores getPlayerPPScores(UString playerName);
    PlayerStats calculatePlayerStats(UString playerName);
    static float getWeightForIndex(int i);
    static float getBonusPPForNumScores(int numScores);
    static unsigned long long getRequiredScoreForLevel(int level);
    static int getLevelForScore(unsigned long long score, int maxLevel = 120);

    inline float getProgress() const { return m_fLoadingProgress.load(); }
    inline bool isFinished() const { return (getProgress() >= 1.0f); }
    inline bool foundChanges() const { return m_bFoundChanges; }

    inline const std::vector<DatabaseBeatmap *> getDatabaseBeatmaps() const { return m_databaseBeatmaps; }
    DatabaseBeatmap *getBeatmap(const MD5Hash &md5hash);
    DatabaseBeatmap *getBeatmapDifficulty(const MD5Hash &md5hash);

    inline std::unordered_map<MD5Hash, std::vector<FinishedScore>> *getScores() { return &m_scores; }
    inline const std::vector<SCORE_SORTING_METHOD> &getScoreSortingMethods() const { return m_scoreSortingMethods; }

    inline unsigned long long getAndIncrementScoreSortHackCounter() { return m_iSortHackCounter++; }

    std::unordered_map<MD5Hash, std::vector<FinishedScore>> m_online_scores;
    std::string getOsuSongsFolder();

    DatabaseBeatmap *loadRawBeatmap(std::string beatmapPath);  // only used for raw loading without db

   private:
    friend class DatabaseLoader;

    static ConVar *m_name_ref;
    static ConVar *m_osu_songbrowser_scores_sortingtype_ref;

    void addScoreRaw(const MD5Hash &beatmapMD5Hash, const FinishedScore &score);

    std::string parseLegacyCfgBeatmapDirectoryParameter();
    void scheduleLoadRaw();
    void loadDB(Packet *db, bool &fallbackToRawLoad);

    void loadStars();
    void saveStars();

    void loadScores();
    void saveScores();

    void onScoresRename(UString args);
    void onScoresExport();

    Osu *m_osu;
    Timer *m_importTimer;
    bool m_bIsFirstLoad;   // only load differences after first raw load
    bool m_bFoundChanges;  // for total refresh detection of raw loading

    // global
    int m_iNumBeatmapsToLoad;
    std::atomic<float> m_fLoadingProgress;
    std::atomic<bool> m_bInterruptLoad;
    std::vector<DatabaseBeatmap *> m_databaseBeatmaps;

    // osu!.db
    int m_iVersion;
    int m_iFolderCount;

    // scores.db (legacy and custom)
    bool m_bScoresLoaded;
    std::unordered_map<MD5Hash, std::vector<FinishedScore>> m_scores;
    bool m_bDidScoresChangeForSave;
    bool m_bDidScoresChangeForStats;
    unsigned long long m_iSortHackCounter;
    PlayerStats m_prevPlayerStats;
    std::vector<SCORE_SORTING_METHOD> m_scoreSortingMethods;

    // raw load
    bool m_bRawBeatmapLoadScheduled;
    int m_iCurRawBeatmapLoadIndex;
    std::string m_sRawBeatmapLoadOsuSongFolder;
    std::vector<std::string> m_rawBeatmapFolders;
    std::vector<std::string> m_rawLoadBeatmapFolders;
    std::unordered_map<MD5Hash, DatabaseBeatmap *> m_rawHashToDiff2;
    std::unordered_map<MD5Hash, DatabaseBeatmap *> m_rawHashToBeatmap;

    // stars.cache
    struct STARS_CACHE_ENTRY {
        float starsNomod;
    };
    std::unordered_map<MD5Hash, STARS_CACHE_ENTRY> m_starsCache;
};
