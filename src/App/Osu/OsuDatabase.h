#pragma once
#include "BanchoProtocol.h"  // Packet
#include "OsuReplay.h"
#include "OsuScore.h"
#include "Score.h"
#include "UString.h"
#include "cbase.h"

class Timer;
class ConVar;

class Osu;
class OsuFile;
class OsuDatabaseBeatmap;
class OsuDatabaseLoader;

struct TIMINGPOINT {
    double msPerBeat;
    double offset;
    bool timingChange;
};
TIMINGPOINT read_timing_point(Packet *packet);

Packet load_db(std::string path);
bool save_db(Packet *db, std::string path);

class OsuDatabase {
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
        std::vector<Score *> ppScores;
        unsigned long long totalScore;
    };

    struct SCORE_SORTING_COMPARATOR {
        virtual ~SCORE_SORTING_COMPARATOR() { ; }
        virtual bool operator()(Score const &a, Score const &b) const = 0;
    };

    struct SCORE_SORTING_METHOD {
        UString name;
        SCORE_SORTING_COMPARATOR *comparator;
    };

   public:
    OsuDatabase(Osu *osu);
    ~OsuDatabase();

    void update();

    void load();
    void cancel();
    void save();

    OsuDatabaseBeatmap *addBeatmap(std::string beatmapFolderPath);

    int addScore(MD5Hash beatmapMD5Hash, Score score);
    void deleteScore(MD5Hash beatmapMD5Hash, uint64_t scoreUnixTimestamp);
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

    inline const std::vector<OsuDatabaseBeatmap *> getDatabaseBeatmaps() const { return m_databaseBeatmaps; }
    OsuDatabaseBeatmap *getBeatmap(const MD5Hash &md5hash);
    OsuDatabaseBeatmap *getBeatmapDifficulty(const MD5Hash &md5hash);

    inline std::unordered_map<MD5Hash, std::vector<Score>> *getScores() { return &m_scores; }
    inline const std::vector<SCORE_SORTING_METHOD> &getScoreSortingMethods() const { return m_scoreSortingMethods; }

    inline unsigned long long getAndIncrementScoreSortHackCounter() { return m_iSortHackCounter++; }

    std::unordered_map<MD5Hash, std::vector<Score>> m_online_scores;
    std::string getOsuSongsFolder();

    OsuDatabaseBeatmap *loadRawBeatmap(std::string beatmapPath);  // only used for raw loading without db

   private:
    friend class OsuDatabaseLoader;

    static ConVar *m_name_ref;
    static ConVar *m_osu_songbrowser_scores_sortingtype_ref;

    void addScoreRaw(const MD5Hash &beatmapMD5Hash, const Score &score);

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
    std::vector<OsuDatabaseBeatmap *> m_databaseBeatmaps;

    // osu!.db
    int m_iVersion;
    int m_iFolderCount;

    // scores.db (legacy and custom)
    bool m_bScoresLoaded;
    std::unordered_map<MD5Hash, std::vector<Score>> m_scores;
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
    std::unordered_map<MD5Hash, OsuDatabaseBeatmap *> m_rawHashToDiff2;
    std::unordered_map<MD5Hash, OsuDatabaseBeatmap *> m_rawHashToBeatmap;

    // stars.cache
    struct STARS_CACHE_ENTRY {
        float starsNomod;
    };
    std::unordered_map<MD5Hash, STARS_CACHE_ENTRY> m_starsCache;
};
