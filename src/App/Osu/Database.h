#pragma once
#include "BanchoProtocol.h"  // Packet
#include "Replay.h"
#include "UString.h"
#include "cbase.h"
#include "score.h"

class Timer;
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

Packet load_db(std::string path);
bool save_db(Packet *db, std::string path);

struct MapOverrides {
    MD5Hash map_md5;
    i16 local_offset = 0;
    i16 online_offset = 0;
    u16 nb_circles = 0;
    u16 nb_sliders = 0;
    u16 nb_spinners = 0;
    f64 star_rating = 0.0;
    i32 min_bpm = 0;
    i32 max_bpm = 0;
    i32 avg_bpm = 0;
    u8 draw_background = 1;
};

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

    void update();

    void load();
    void cancel();
    void save();

    BeatmapSet *addBeatmapSet(std::string beatmapFolderPath);

    int addScore(FinishedScore score);
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
    inline bool isLoading() const {
        float progress = getProgress();
        return progress > 0.f && progress < 1.f;
    }
    inline bool isFinished() const { return (getProgress() >= 1.0f); }
    inline bool foundChanges() const { return m_bFoundChanges; }

    inline const std::vector<DatabaseBeatmap *> getDatabaseBeatmaps() const { return m_beatmapsets; }
    DatabaseBeatmap *getBeatmapDifficulty(const MD5Hash &md5hash);
    DatabaseBeatmap *getBeatmapDifficulty(i32 map_id);
    DatabaseBeatmap *getBeatmapSet(i32 set_id);

    inline std::unordered_map<MD5Hash, std::vector<FinishedScore>> *getScores() { return &m_scores; }
    inline const std::vector<SCORE_SORTING_METHOD> &getScoreSortingMethods() const { return m_scoreSortingMethods; }

    inline unsigned long long getAndIncrementScoreSortHackCounter() { return m_iSortHackCounter++; }

    std::unordered_map<MD5Hash, std::vector<FinishedScore>> m_online_scores;
    std::string getOsuSongsFolder();

    BeatmapSet *loadRawBeatmap(std::string beatmapPath);  // only used for raw loading without db

    void loadDB();

   private:
    friend class DatabaseLoader;

    std::string parseLegacyCfgBeatmapDirectoryParameter();

    void saveMaps();

    void loadScores();
    u32 importOldNeosuScores();
    u32 importPeppyScores();
    void saveScores();
    bool addScoreRaw(const FinishedScore &score);

    Timer *m_importTimer;
    bool m_bIsFirstLoad;   // only load differences after first raw load
    bool m_bFoundChanges;  // for total refresh detection of raw loading

    // global
    int m_iNumBeatmapsToLoad;
    std::atomic<float> m_fLoadingProgress;
    std::atomic<bool> m_bInterruptLoad;
    std::vector<BeatmapSet *> m_beatmapsets;
    std::vector<BeatmapSet *> m_neosu_sets;
    std::vector<MapOverrides> m_peppy_overrides;
    std::unordered_map<MD5Hash, BeatmapDifficulty *> m_beatmap_difficulties;
    bool m_neosu_maps_loaded = false;

    std::vector<BeatmapDifficulty *> m_maps_to_recalc;

    // osu!.db
    int m_iVersion;
    int m_iFolderCount;

    // scores.db (legacy and custom)
    bool m_bScoresLoaded = false;
    std::unordered_map<MD5Hash, std::vector<FinishedScore>> m_scores;
    bool m_bDidScoresChangeForSave;
    bool m_bDidScoresChangeForStats;
    unsigned long long m_iSortHackCounter;
    PlayerStats m_prevPlayerStats;
    std::vector<SCORE_SORTING_METHOD> m_scoreSortingMethods;
};
