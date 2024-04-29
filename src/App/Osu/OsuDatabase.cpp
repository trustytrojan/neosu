//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		osu!.db + collection.db + raw loader + scores etc.
//
// $NoKeywords: $osubdb
//===============================================================================//

#include "OsuDatabase.h"

#include <string.h>

#include <fstream>

#include "Bancho.h"  // md5
#include "BanchoNetworking.h"
#include "Collections.h"
#include "ConVar.h"
#include "Engine.h"
#include "File.h"
#include "Osu.h"
#include "OsuDatabaseBeatmap.h"
#include "OsuNotificationOverlay.h"
#include "OsuReplay.h"
#include "OsuScore.h"
#include "ResourceManager.h"
#include "Timer.h"

#ifdef _WIN32

ConVar osu_folder("osu_folder", "C:/Program Files (x86)/osu!/", FCVAR_NONE);

#elif defined __APPLE__

ConVar osu_folder("osu_folder", "/osu!/", FCVAR_NONE);

#else

ConVar osu_folder("osu_folder", "", FCVAR_NONE);

#endif

ConVar osu_folder_sub_songs("osu_folder_sub_songs", "Songs/", FCVAR_NONE);
ConVar osu_folder_sub_skins("osu_folder_sub_skins", "Skins/", FCVAR_NONE);

ConVar osu_database_enabled("osu_database_enabled", true, FCVAR_NONE);
ConVar osu_database_version("osu_database_version", OSU_VERSION_DATEONLY, FCVAR_NONE,
                            "maximum supported osu!.db version, above this will use fallback loader");
ConVar osu_database_ignore_version_warnings("osu_database_ignore_version_warnings", false, FCVAR_NONE);
ConVar osu_database_ignore_version("osu_database_ignore_version", true, FCVAR_NONE,
                                   "ignore upper version limit and force load the db file (may crash)");
ConVar osu_database_stars_cache_enabled("osu_database_stars_cache_enabled", false, FCVAR_NONE);
ConVar osu_scores_enabled("osu_scores_enabled", true, FCVAR_NONE);
ConVar osu_scores_legacy_enabled("osu_scores_legacy_enabled", true, FCVAR_NONE, "load osu!'s scores.db");
ConVar osu_scores_custom_enabled("osu_scores_custom_enabled", true, FCVAR_NONE, "load custom scores.db");
ConVar osu_scores_save_immediately("osu_scores_save_immediately", true, FCVAR_NONE,
                                   "write scores.db as soon as a new score is added");
ConVar osu_scores_sort_by_pp("osu_scores_sort_by_pp", true, FCVAR_NONE, "display pp in score browser instead of score");
ConVar osu_scores_bonus_pp("osu_scores_bonus_pp", true, FCVAR_NONE,
                           "whether to add bonus pp to total (real) pp or not");
ConVar osu_scores_rename("osu_scores_rename");
ConVar osu_scores_export("osu_scores_export");
ConVar osu_collections_legacy_enabled("osu_collections_legacy_enabled", true, FCVAR_NONE, "load osu!'s collection.db");
ConVar osu_collections_custom_enabled("osu_collections_custom_enabled", true, FCVAR_NONE, "load custom collections.db");
ConVar osu_collections_custom_version("osu_collections_custom_version", 20220110, FCVAR_NONE,
                                      "maximum supported custom collections.db version");
ConVar osu_collections_save_immediately("osu_collections_save_immediately", true, FCVAR_NONE,
                                        "write collections.db as soon as anything is changed");
ConVar osu_user_include_relax_and_autopilot_for_stats("osu_user_include_relax_and_autopilot_for_stats", false,
                                                      FCVAR_NONE);
ConVar osu_user_switcher_include_legacy_scores_for_names("osu_user_switcher_include_legacy_scores_for_names", true,
                                                         FCVAR_NONE);

TIMINGPOINT read_timing_point(Packet *packet) {
    const double bpm = read_float64(packet);
    const double offset = read_float64(packet);
    const bool timingChange = (bool)read_byte(packet);
    return (struct TIMINGPOINT){bpm, offset, timingChange};
}

Packet load_db(std::string path) {
    Packet db;

    FILE *dbfile = fopen(path.c_str(), "rb");
    if(dbfile == NULL) {
        debugLog("Failed to open '%s': %s\n", path.c_str(), strerror(errno));
    } else {
        fseek(dbfile, 0, SEEK_END);
        size_t dblen = ftell(dbfile);
        rewind(dbfile);

        db.memory = (uint8_t *)malloc(dblen);
        db.size = dblen;
        fread(db.memory, dblen, 1, dbfile);
        fclose(dbfile);
    }

    return db;
}

bool save_db(Packet *db, std::string path) {
    FILE *dbfile = fopen(path.c_str(), "wb");
    if(dbfile == NULL) {
        return false;
    }

    fwrite(db->memory, db->pos, 1, dbfile);
    fclose(dbfile);
    return true;
}

struct SortScoreByScore : public OsuDatabase::SCORE_SORTING_COMPARATOR {
    virtual ~SortScoreByScore() { ; }
    bool operator()(Score const &a, Score const &b) const {
        // first: score
        unsigned long long score1 = a.score;
        unsigned long long score2 = b.score;

        // second: time
        if(score1 == score2) {
            score1 = a.unixTimestamp;
            score2 = b.unixTimestamp;
        }

        // strict weak ordering!
        if(score1 == score2) return a.sortHack > b.sortHack;

        return score1 > score2;
    }
};

struct SortScoreByCombo : public OsuDatabase::SCORE_SORTING_COMPARATOR {
    virtual ~SortScoreByCombo() { ; }
    bool operator()(Score const &a, Score const &b) const {
        // first: combo
        unsigned long long score1 = a.comboMax;
        unsigned long long score2 = b.comboMax;

        // second: score
        if(score1 == score2) {
            score1 = a.score;
            score2 = b.score;
        }

        // third: time
        if(score1 == score2) {
            score1 = a.unixTimestamp;
            score2 = b.unixTimestamp;
        }

        // strict weak ordering!
        if(score1 == score2) return a.sortHack > b.sortHack;

        return score1 > score2;
    }
};

struct SortScoreByDate : public OsuDatabase::SCORE_SORTING_COMPARATOR {
    virtual ~SortScoreByDate() { ; }
    bool operator()(Score const &a, Score const &b) const {
        // first: time
        unsigned long long score1 = a.unixTimestamp;
        unsigned long long score2 = b.unixTimestamp;

        // strict weak ordering!
        if(score1 == score2) return a.sortHack > b.sortHack;

        return score1 > score2;
    }
};

struct SortScoreByMisses : public OsuDatabase::SCORE_SORTING_COMPARATOR {
    virtual ~SortScoreByMisses() { ; }
    bool operator()(Score const &a, Score const &b) const {
        // first: misses
        unsigned long long score1 = b.numMisses;  // swapped (lower numMisses is better)
        unsigned long long score2 = a.numMisses;

        // second: score
        if(score1 == score2) {
            score1 = a.score;
            score2 = b.score;
        }

        // third: time
        if(score1 == score2) {
            score1 = a.unixTimestamp;
            score2 = b.unixTimestamp;
        }

        // strict weak ordering!
        if(score1 == score2) return a.sortHack > b.sortHack;

        return score1 > score2;
    }
};

struct SortScoreByAccuracy : public OsuDatabase::SCORE_SORTING_COMPARATOR {
    virtual ~SortScoreByAccuracy() { ; }
    bool operator()(Score const &a, Score const &b) const {
        // first: accuracy
        unsigned long long score1 =
            (unsigned long long)(OsuScore::calculateAccuracy(a.num300s, a.num100s, a.num50s, a.numMisses) * 10000.0f);
        unsigned long long score2 =
            (unsigned long long)(OsuScore::calculateAccuracy(b.num300s, b.num100s, b.num50s, b.numMisses) * 10000.0f);

        // second: score
        if(score1 == score2) {
            score1 = a.score;
            score2 = b.score;
        }

        // third: time
        if(score1 == score2) {
            score1 = a.unixTimestamp;
            score2 = b.unixTimestamp;
        }

        // strict weak ordering!
        if(score1 == score2) return a.sortHack > b.sortHack;

        return score1 > score2;
    }
};

struct SortScoreByPP : public OsuDatabase::SCORE_SORTING_COMPARATOR {
    virtual ~SortScoreByPP() { ; }
    bool operator()(Score const &a, Score const &b) const {
        // first: pp
        unsigned long long score1 = (unsigned long long)std::max(a.pp * 100.0f, 0.0f);
        unsigned long long score2 = (unsigned long long)std::max(b.pp * 100.0f, 0.0f);

        // second: score
        if(score1 == score2) {
            score1 = a.score;
            score2 = b.score;
        }

        // third: time
        if(score1 == score2) {
            score1 = a.unixTimestamp;
            score2 = b.unixTimestamp;
        }

        // strict weak ordering!
        if(score1 == score2 ||
           a.isLegacyScore != b.isLegacyScore)  // force for type discrepancies (legacy scores don't contain pp data)
            return a.sortHack > b.sortHack;

        return score1 > score2;
    }
};

class OsuDatabaseLoader : public Resource {
   public:
    OsuDatabaseLoader(OsuDatabase *db) : Resource() {
        m_db = db;
        m_bNeedRawLoad = false;
        m_bNeedCleanup = false;

        m_bAsyncReady = false;
        m_bReady = false;
    };

   protected:
    virtual void init() {
        // legacy loading, if db is not found or by convar
        if(m_bNeedRawLoad)
            m_db->scheduleLoadRaw();
        else {
            // delete all previously loaded beatmaps here
            if(m_bNeedCleanup) {
                m_bNeedCleanup = false;
                for(int i = 0; i < m_toCleanup.size(); i++) {
                    delete m_toCleanup[i];
                }
                m_toCleanup.clear();
            }
        }

        m_bReady = true;

        delete this;  // commit sudoku
    }

    virtual void initAsync() {
        debugLog("OsuDatabaseLoader::initAsync()\n");

        // load scores
        m_db->loadScores();

        // load stars.cache
        m_db->loadStars();

        // check if osu database exists, load file completely
        std::string osuDbFilePath = osu_folder.getString().toUtf8();
        osuDbFilePath.append("osu!.db");

        // load database
        Packet db = load_db(osuDbFilePath);
        if(db.size > 0 && osu_database_enabled.getBool()) {
            m_bNeedCleanup = true;
            m_toCleanup.swap(m_db->m_databaseBeatmaps);
            m_db->m_databaseBeatmaps.clear();

            m_db->m_fLoadingProgress = 0.25f;
            m_db->loadDB(&db, m_bNeedRawLoad);
        } else
            m_bNeedRawLoad = true;

        m_bAsyncReady = true;
    }

    virtual void destroy() { ; }

   private:
    OsuDatabase *m_db;
    bool m_bNeedRawLoad;

    bool m_bNeedCleanup;
    std::vector<OsuDatabaseBeatmap *> m_toCleanup;
};

ConVar *OsuDatabase::m_name_ref = NULL;
ConVar *OsuDatabase::m_osu_songbrowser_scores_sortingtype_ref = NULL;

OsuDatabase::OsuDatabase(Osu *osu) {
    // convar refs
    if(m_name_ref == NULL) m_name_ref = convar->getConVarByName("name");
    if(m_osu_songbrowser_scores_sortingtype_ref == NULL)
        m_osu_songbrowser_scores_sortingtype_ref = convar->getConVarByName("osu_songbrowser_scores_sortingtype");

    osu_scores_rename.setCallback(fastdelegate::MakeDelegate(this, &OsuDatabase::onScoresRename));
    osu_scores_export.setCallback(fastdelegate::MakeDelegate(this, &OsuDatabase::onScoresExport));

    // vars
    m_importTimer = new Timer();
    m_bIsFirstLoad = true;
    m_bFoundChanges = true;

    m_iNumBeatmapsToLoad = 0;
    m_fLoadingProgress = 0.0f;
    m_bInterruptLoad = false;

    m_osu = osu;
    m_iVersion = 0;
    m_iFolderCount = 0;

    m_bScoresLoaded = false;
    m_bDidScoresChangeForSave = false;
    m_bDidScoresChangeForStats = true;
    m_iSortHackCounter = 0;

    m_iCurRawBeatmapLoadIndex = 0;
    m_bRawBeatmapLoadScheduled = false;

    m_prevPlayerStats.pp = 0.0f;
    m_prevPlayerStats.accuracy = 0.0f;
    m_prevPlayerStats.numScoresWithPP = 0;
    m_prevPlayerStats.level = 0;
    m_prevPlayerStats.percentToNextLevel = 0.0f;
    m_prevPlayerStats.totalScore = 0;

    m_scoreSortingMethods.push_back({"Sort By Accuracy", new SortScoreByAccuracy()});
    m_scoreSortingMethods.push_back({"Sort By Combo", new SortScoreByCombo()});
    m_scoreSortingMethods.push_back({"Sort By Date", new SortScoreByDate()});
    m_scoreSortingMethods.push_back({"Sort By Misses", new SortScoreByMisses()});
    m_scoreSortingMethods.push_back({"Sort By pp (Mc)", new SortScoreByPP()});
    m_scoreSortingMethods.push_back({"Sort By Score", new SortScoreByScore()});
}

OsuDatabase::~OsuDatabase() {
    SAFE_DELETE(m_importTimer);

    for(int i = 0; i < m_databaseBeatmaps.size(); i++) {
        delete m_databaseBeatmaps[i];
    }

    for(int i = 0; i < m_scoreSortingMethods.size(); i++) {
        delete m_scoreSortingMethods[i].comparator;
    }
}

void OsuDatabase::update() {
    // loadRaw() logic
    if(m_bRawBeatmapLoadScheduled) {
        Timer t;
        t.start();

        while(t.getElapsedTime() < 0.033f) {
            if(m_bInterruptLoad.load()) break;  // cancellation point

            if(m_rawLoadBeatmapFolders.size() > 0 && m_iCurRawBeatmapLoadIndex < m_rawLoadBeatmapFolders.size()) {
                std::string curBeatmap = m_rawLoadBeatmapFolders[m_iCurRawBeatmapLoadIndex++];
                m_rawBeatmapFolders.push_back(
                    curBeatmap);  // for future incremental loads, so that we know what's been loaded already

                std::string fullBeatmapPath = getOsuSongsFolder();
                fullBeatmapPath.append(curBeatmap);
                fullBeatmapPath.append("/");

                addBeatmap(fullBeatmapPath);
            }

            // update progress
            m_fLoadingProgress = (float)m_iCurRawBeatmapLoadIndex / (float)m_iNumBeatmapsToLoad;

            // check if we are finished
            if(m_iCurRawBeatmapLoadIndex >= m_iNumBeatmapsToLoad ||
               m_iCurRawBeatmapLoadIndex > (int)(m_rawLoadBeatmapFolders.size() - 1)) {
                m_rawLoadBeatmapFolders.clear();
                m_bRawBeatmapLoadScheduled = false;
                m_importTimer->update();

                debugLog("Refresh finished, added %i beatmaps in %f seconds.\n", m_databaseBeatmaps.size(),
                         m_importTimer->getElapsedTime());

                load_collections();

                m_fLoadingProgress = 1.0f;

                break;
            }

            t.update();
        }
    }
}

void OsuDatabase::load() {
    m_bInterruptLoad = false;
    m_fLoadingProgress = 0.0f;

    OsuDatabaseLoader *loader = new OsuDatabaseLoader(this);  // (deletes itself after finishing)

    engine->getResourceManager()->requestNextLoadAsync();
    engine->getResourceManager()->loadResource(loader);
}

void OsuDatabase::cancel() {
    m_bInterruptLoad = true;
    m_bRawBeatmapLoadScheduled = false;
    m_fLoadingProgress = 1.0f;  // force finished
    m_bFoundChanges = true;
}

void OsuDatabase::save() {
    saveScores();
    saveStars();
}

OsuDatabaseBeatmap *OsuDatabase::addBeatmap(std::string beatmapFolderPath) {
    OsuDatabaseBeatmap *beatmap = loadRawBeatmap(beatmapFolderPath);

    if(beatmap != NULL) m_databaseBeatmaps.push_back(beatmap);

    return beatmap;
}

int OsuDatabase::addScore(MD5Hash beatmapMD5Hash, Score score) {
    addScoreRaw(beatmapMD5Hash, score);
    sortScores(beatmapMD5Hash);

    m_bDidScoresChangeForSave = true;
    m_bDidScoresChangeForStats = true;

    if(osu_scores_save_immediately.getBool()) saveScores();

    // XXX: this is blocking main thread
    uint8_t *compressed_replay = NULL;
    size_t s_compressed_replay = 0;
    OsuReplay::compress_frames(score.replay, &compressed_replay, &s_compressed_replay);
    if(s_compressed_replay > 0) {
        std::string replay_path;
        std::stringstream ss;
        ss << MCENGINE_DATA_DIR "replays/" << score.unixTimestamp << ".replay.lzma";
        replay_path = ss.str();

        FILE *replay_file = fopen(replay_path.c_str(), "wb");
        if(replay_file != NULL) {
            fwrite(compressed_replay, s_compressed_replay, 1, replay_file);
            fclose(replay_file);
        }
    }

    // return sorted index
    for(int i = 0; i < m_scores[beatmapMD5Hash].size(); i++) {
        if(m_scores[beatmapMD5Hash][i].unixTimestamp == score.unixTimestamp) return i;
    }

    return -1;
}

void OsuDatabase::addScoreRaw(const MD5Hash &beatmapMD5Hash, const Score &score) {
    m_scores[beatmapMD5Hash].push_back(score);

    // cheap dynamic recalculations for neosu scores
    if(!score.isLegacyScore) {
        // as soon as we have >= 1 score with maxPossibleCombo info, all scores of that beatmap (even older ones without
        // the info) can get the 'perfect' flag set all scores >= 20180722 already have this populated during load, so
        // this only affects the brief period where pp was stored without maxPossibleCombo info
        {
            // find score with maxPossibleCombo info
            int maxPossibleCombo = -1;
            for(const Score &s : m_scores[beatmapMD5Hash]) {
                if(s.version > 20180722) {
                    if(s.maxPossibleCombo > 0) {
                        maxPossibleCombo = s.maxPossibleCombo;
                        break;
                    }
                }
            }

            // set 'perfect' flag on all relevant old scores of that same beatmap
            if(maxPossibleCombo > 0) {
                for(Score &s : m_scores[beatmapMD5Hash]) {
                    if(s.version <= 20180722 ||
                       s.maxPossibleCombo <
                           1)  // also set on scores which have broken maxPossibleCombo values for whatever reason
                        s.perfect = (s.comboMax > 0 && s.comboMax >= maxPossibleCombo);
                }
            }
        }
    }
}

void OsuDatabase::deleteScore(MD5Hash beatmapMD5Hash, uint64_t scoreUnixTimestamp) {
    for(int i = 0; i < m_scores[beatmapMD5Hash].size(); i++) {
        if(m_scores[beatmapMD5Hash][i].unixTimestamp == scoreUnixTimestamp) {
            m_scores[beatmapMD5Hash].erase(m_scores[beatmapMD5Hash].begin() + i);

            m_bDidScoresChangeForSave = true;
            m_bDidScoresChangeForStats = true;

            // debugLog("Deleted score for %s at %llu\n", beatmapMD5Hash.c_str(), scoreUnixTimestamp);

            break;
        }
    }
}

void OsuDatabase::sortScores(MD5Hash beatmapMD5Hash) {
    if(m_scores[beatmapMD5Hash].size() < 2) return;

    if(m_osu_songbrowser_scores_sortingtype_ref->getString() == UString("Online Leaderboard")) {
        // Online scores are already sorted
        return;
    }

    for(int i = 0; i < m_scoreSortingMethods.size(); i++) {
        if(m_osu_songbrowser_scores_sortingtype_ref->getString() == m_scoreSortingMethods[i].name) {
            struct COMPARATOR_WRAPPER {
                SCORE_SORTING_COMPARATOR *comp;
                bool operator()(Score const &a, Score const &b) const { return comp->operator()(a, b); }
            };
            COMPARATOR_WRAPPER comparatorWrapper;
            comparatorWrapper.comp = m_scoreSortingMethods[i].comparator;

            std::sort(m_scores[beatmapMD5Hash].begin(), m_scores[beatmapMD5Hash].end(), comparatorWrapper);
            return;
        }
    }

    if(convar->getConVarByName("osu_debug")->getBool()) {
        debugLog("ERROR: Invalid score sortingtype \"%s\"\n",
                 m_osu_songbrowser_scores_sortingtype_ref->getString().toUtf8());
    }
}

std::vector<UString> OsuDatabase::getPlayerNamesWithPPScores() {
    std::vector<MD5Hash> keys;
    keys.reserve(m_scores.size());

    for(auto kv : m_scores) {
        keys.push_back(kv.first);
    }

    std::unordered_set<std::string> tempNames;
    for(auto &key : keys) {
        for(Score &score : m_scores[key]) {
            if(!score.isLegacyScore) tempNames.insert(score.playerName);
        }
    }

    // always add local user, even if there were no scores
    auto local_name = m_name_ref->getString();
    tempNames.insert(std::string(local_name.toUtf8()));

    std::vector<UString> names;
    names.reserve(tempNames.size());
    for(auto k : tempNames) {
        if(k.length() > 0) names.push_back(UString(k.c_str()));
    }

    return names;
}

std::vector<UString> OsuDatabase::getPlayerNamesWithScoresForUserSwitcher() {
    const bool includeLegacyNames = osu_user_switcher_include_legacy_scores_for_names.getBool();

    std::unordered_set<std::string> tempNames;
    for(auto kv : m_scores) {
        const MD5Hash &key = kv.first;
        for(Score &score : m_scores[key]) {
            if(!score.isLegacyScore || includeLegacyNames) tempNames.insert(score.playerName);
        }
    }

    // always add local user, even if there were no scores
    auto local_name = m_name_ref->getString();
    tempNames.insert(std::string(local_name.toUtf8()));

    std::vector<UString> names;
    names.reserve(tempNames.size());
    for(auto k : tempNames) {
        if(k.length() > 0) names.push_back(UString(k.c_str()));
    }

    return names;
}

OsuDatabase::PlayerPPScores OsuDatabase::getPlayerPPScores(UString playerName) {
    std::vector<Score *> scores;

    // collect all scores with pp data
    std::vector<MD5Hash> keys;
    keys.reserve(m_scores.size());

    for(auto kv : m_scores) {
        keys.push_back(kv.first);
    }

    struct ScoreSortComparator {
        bool operator()(Score const *a, Score const *b) const {
            // sort by pp
            // strict weak ordering!
            if(a->pp == b->pp)
                return a->sortHack < b->sortHack;
            else
                return a->pp < b->pp;
        }
    };

    unsigned long long totalScore = 0;
    for(auto &key : keys) {
        if(m_scores[key].size() > 0) {
            Score *tempScore = &m_scores[key][0];

            // only add highest pp score per diff
            bool foundValidScore = false;
            float prevPP = -1.0f;
            for(Score &score : m_scores[key]) {
                UString uName = UString(score.playerName.c_str());

                if(!score.isLegacyScore &&
                   (osu_user_include_relax_and_autopilot_for_stats.getBool()
                        ? true
                        : !((score.modsLegacy & ModFlags::Relax) || (score.modsLegacy & ModFlags::Autopilot))) &&
                   uName == playerName) {
                    foundValidScore = true;

                    totalScore += score.score;

                    score.sortHack = m_iSortHackCounter++;

                    if(score.pp > prevPP || prevPP < 0.0f) {
                        prevPP = score.pp;
                        tempScore = &score;
                    }
                }
            }

            if(foundValidScore) scores.push_back(tempScore);
        }
    }

    // sort by pp
    std::sort(scores.begin(), scores.end(), ScoreSortComparator());

    PlayerPPScores ppScores;
    ppScores.ppScores = std::move(scores);
    ppScores.totalScore = totalScore;

    return ppScores;
}

OsuDatabase::PlayerStats OsuDatabase::calculatePlayerStats(UString playerName) {
    if(!m_bDidScoresChangeForStats && playerName == m_prevPlayerStats.name) return m_prevPlayerStats;

    const PlayerPPScores ps = getPlayerPPScores(playerName);

    // delay caching until we actually have scores loaded
    if(ps.ppScores.size() > 0) m_bDidScoresChangeForStats = false;

    // "If n is the amount of scores giving more pp than a given score, then the score's weight is 0.95^n"
    // "Total pp = PP[1] * 0.95^0 + PP[2] * 0.95^1 + PP[3] * 0.95^2 + ... + PP[n] * 0.95^(n-1)"
    // also, total accuracy is apparently weighted the same as pp

    // https://expectancyviolation.github.io/osu-acc/

    float pp = 0.0f;
    float acc = 0.0f;
    for(size_t i = 0; i < ps.ppScores.size(); i++) {
        const float weight = getWeightForIndex(ps.ppScores.size() - 1 - i);

        pp += ps.ppScores[i]->pp * weight;
        acc += OsuScore::calculateAccuracy(ps.ppScores[i]->num300s, ps.ppScores[i]->num100s, ps.ppScores[i]->num50s,
                                           ps.ppScores[i]->numMisses) *
               weight;
    }

    // bonus pp
    // https://osu.ppy.sh/wiki/en/Performance_points
    if(osu_scores_bonus_pp.getBool()) pp += getBonusPPForNumScores(ps.ppScores.size());

    // normalize accuracy
    if(ps.ppScores.size() > 0) acc /= (20.0f * (1.0f - getWeightForIndex(ps.ppScores.size())));

    // fill stats
    m_prevPlayerStats.name = playerName;
    m_prevPlayerStats.pp = pp;
    m_prevPlayerStats.accuracy = acc;
    m_prevPlayerStats.numScoresWithPP = ps.ppScores.size();

    if(ps.totalScore != m_prevPlayerStats.totalScore) {
        m_prevPlayerStats.level = getLevelForScore(ps.totalScore);

        const unsigned long long requiredScoreForCurrentLevel = getRequiredScoreForLevel(m_prevPlayerStats.level);
        const unsigned long long requiredScoreForNextLevel = getRequiredScoreForLevel(m_prevPlayerStats.level + 1);

        if(requiredScoreForNextLevel > requiredScoreForCurrentLevel)
            m_prevPlayerStats.percentToNextLevel = (double)(ps.totalScore - requiredScoreForCurrentLevel) /
                                                   (double)(requiredScoreForNextLevel - requiredScoreForCurrentLevel);
    }

    m_prevPlayerStats.totalScore = ps.totalScore;

    return m_prevPlayerStats;
}

float OsuDatabase::getWeightForIndex(int i) { return std::pow(0.95, (double)i); }

float OsuDatabase::getBonusPPForNumScores(int numScores) {
    // TODO: rework to take size_t numScores instead of useless early int conversion
    // TODO: not actually sure if the new version is even correct, not on any official osu github repositories yet, so
    // still waiting for confirmation see
    // https://github.com/ppy/osu-queue-score-statistics/blob/185ad3c00423f6209bcfd8f7177fbe915179055a/osu.Server.Queues.ScoreStatisticsProcessor/Processors/UserTotalPerformanceProcessor.cs#L72

    // old
    return (416.6667 * (1.0 - std::pow(0.9994, (double)numScores)));

    // new
    // return ((417.0 - 1.0 / 3.0) * (1.0 - std::pow(0.995, (double)std::min(1000, numScores))));
}

unsigned long long OsuDatabase::getRequiredScoreForLevel(int level) {
    // https://zxq.co/ripple/ocl/src/branch/master/level.go
    if(level <= 100) {
        if(level > 1)
            return (uint64_t)std::floor(5000 / 3 * (4 * std::pow(level, 3) - 3 * std::pow(level, 2) - level) +
                                        std::floor(1.25 * std::pow(1.8, (double)(level - 60))));

        return 1;
    }

    return (uint64_t)26931190829 + (uint64_t)100000000000 * (uint64_t)(level - 100);
}

int OsuDatabase::getLevelForScore(unsigned long long score, int maxLevel) {
    // https://zxq.co/ripple/ocl/src/branch/master/level.go
    int i = 0;
    while(true) {
        if(maxLevel > 0 && i >= maxLevel) return i;

        const unsigned long long lScore = getRequiredScoreForLevel(i);

        if(score < lScore) return (i - 1);

        i++;
    }
}

OsuDatabaseBeatmap *OsuDatabase::getBeatmap(const MD5Hash &md5hash) {
    for(size_t i = 0; i < m_databaseBeatmaps.size(); i++) {
        OsuDatabaseBeatmap *beatmap = m_databaseBeatmaps[i];
        const std::vector<OsuDatabaseBeatmap *> &diffs = beatmap->getDifficulties();
        for(size_t d = 0; d < diffs.size(); d++) {
            const OsuDatabaseBeatmap *diff = diffs[d];
            if(diff->getMD5Hash() == md5hash) {
                return beatmap;
            }
        }
    }

    return NULL;
}

OsuDatabaseBeatmap *OsuDatabase::getBeatmapDifficulty(const MD5Hash &md5hash) {
    // TODO: optimize db accesses by caching a hashmap from md5hash -> OsuBeatmap*, currently it just does a loop over
    // all diffs of all beatmaps (for every call)
    for(size_t i = 0; i < m_databaseBeatmaps.size(); i++) {
        OsuDatabaseBeatmap *beatmap = m_databaseBeatmaps[i];
        const std::vector<OsuDatabaseBeatmap *> &diffs = beatmap->getDifficulties();
        for(size_t d = 0; d < diffs.size(); d++) {
            OsuDatabaseBeatmap *diff = diffs[d];
            if(diff->getMD5Hash() == md5hash) {
                return diff;
            }
        }
    }

    return NULL;
}

std::string OsuDatabase::parseLegacyCfgBeatmapDirectoryParameter() {
    // get BeatmapDirectory parameter from osu!.<OS_USERNAME>.cfg
    debugLog("OsuDatabase::parseLegacyCfgBeatmapDirectoryParameter() : username = %s\n", env->getUsername().toUtf8());
    if(env->getUsername().length() > 0) {
        std::string osuUserConfigFilePath = osu_folder.getString().toUtf8();
        osuUserConfigFilePath.append("osu!.");
        osuUserConfigFilePath.append(env->getUsername());
        osuUserConfigFilePath.append(".cfg");

        File file(osuUserConfigFilePath);
        char stringBuffer[1024];
        while(file.canRead()) {
            std::string curLine = file.readLine();
            memset(stringBuffer, '\0', 1024);
            if(sscanf(curLine.c_str(), " BeatmapDirectory = %1023[^\n]", stringBuffer) == 1) {
                std::string beatmapDirectory = stringBuffer;
                trim(&beatmapDirectory);

                if(beatmapDirectory.length() > 2) {
                    // if we have an absolute path, use it in its entirety.
                    // otherwise, append the beatmapDirectory to the songFolder (which uses the osu_folder as the
                    // starting point)
                    std::string songsFolder = osu_folder.getString().toUtf8();

                    if(beatmapDirectory.find(':') != -1)
                        songsFolder = beatmapDirectory;
                    else {
                        // ensure that beatmapDirectory doesn't start with a slash
                        if(beatmapDirectory[0] == '/' || beatmapDirectory[0] == '\\') beatmapDirectory.erase(0, 1);

                        songsFolder.append(beatmapDirectory);
                    }

                    // ensure that the songFolder ends with a slash
                    if(songsFolder.length() > 0) {
                        if(songsFolder[songsFolder.length() - 1] != '/' &&
                           songsFolder[songsFolder.length() - 1] != '\\')
                            songsFolder.append("/");
                    }

                    return songsFolder;
                }

                break;
            }
        }
    }

    return "";
}

std::string OsuDatabase::getOsuSongsFolder() {
    auto song_folder = osu_folder.getString();
    {
        const std::string customBeatmapDirectory = parseLegacyCfgBeatmapDirectoryParameter();
        if(customBeatmapDirectory.length() < 1)
            song_folder.append(osu_folder_sub_songs.getString());
        else
            song_folder = customBeatmapDirectory.c_str();
    }

    std::string out(song_folder.toUtf8());
    return out;
}

void OsuDatabase::scheduleLoadRaw() {
    auto song_folder = getOsuSongsFolder();
    debugLog("Database: song_folder = %s\n", song_folder.c_str());

    m_rawLoadBeatmapFolders = env->getFoldersInFolder(song_folder);
    m_iNumBeatmapsToLoad = m_rawLoadBeatmapFolders.size();

    // if this isn't the first load, only load the differences
    if(!m_bIsFirstLoad) {
        std::vector<std::string> toLoad;
        for(int i = 0; i < m_iNumBeatmapsToLoad; i++) {
            bool alreadyLoaded = false;
            for(int j = 0; j < m_rawBeatmapFolders.size(); j++) {
                if(m_rawLoadBeatmapFolders[i] == m_rawBeatmapFolders[j]) {
                    alreadyLoaded = true;
                    break;
                }
            }

            if(!alreadyLoaded) toLoad.push_back(m_rawLoadBeatmapFolders[i]);
        }

        // only load differences
        m_rawLoadBeatmapFolders = toLoad;
        m_iNumBeatmapsToLoad = m_rawLoadBeatmapFolders.size();

        debugLog("Database: Found %i new/changed beatmaps.\n", m_iNumBeatmapsToLoad);

        m_bFoundChanges = m_iNumBeatmapsToLoad > 0;
        if(m_bFoundChanges)
            m_osu->getNotificationOverlay()->addNotification(
                UString::format(m_iNumBeatmapsToLoad == 1 ? "Adding %i new beatmap." : "Adding %i new beatmaps.",
                                m_iNumBeatmapsToLoad),
                0xff00ff00);
        else
            m_osu->getNotificationOverlay()->addNotification(
                UString::format("No new beatmaps detected.", m_iNumBeatmapsToLoad), 0xff00ff00);
    }

    debugLog("Database: Building beatmap database ...\n");
    debugLog("Database: Found %i folders to load.\n", m_rawLoadBeatmapFolders.size());

    // only start loading if we have something to load
    if(m_rawLoadBeatmapFolders.size() > 0) {
        m_fLoadingProgress = 0.0f;
        m_iCurRawBeatmapLoadIndex = 0;

        m_bRawBeatmapLoadScheduled = true;
        m_importTimer->start();

        if(m_bIsFirstLoad) {
            // reset
            m_rawHashToDiff2.clear();
            m_rawHashToBeatmap.clear();
        }
    } else
        m_fLoadingProgress = 1.0f;

    m_bIsFirstLoad = false;
}

void OsuDatabase::loadDB(Packet *db, bool &fallbackToRawLoad) {
    if(m_databaseBeatmaps.size() > 0) debugLog("WARNING: OsuDatabase::loadDB() called without cleared m_beatmaps!!!\n");

    m_databaseBeatmaps.clear();

    // get BeatmapDirectory parameter from osu!.<OS_USERNAME>.cfg
    // fallback to /Songs/ if it doesn't exist
    std::string songFolder = osu_folder.getString().toUtf8();
    {
        const std::string customBeatmapDirectory = parseLegacyCfgBeatmapDirectoryParameter();
        if(customBeatmapDirectory.length() < 1)
            songFolder.append(osu_folder_sub_songs.getString().toUtf8());
        else
            songFolder = customBeatmapDirectory;
    }

    debugLog("Database: songFolder = %s\n", songFolder.c_str());

    m_importTimer->start();

    // read header
    m_iVersion = read_int32(db);
    m_iFolderCount = read_int32(db);
    read_byte(db);
    read_int64(db) /* timestamp */;
    auto playerName = read_stdstring(db);
    m_iNumBeatmapsToLoad = read_int32(db);

    debugLog("Database: version = %i, folderCount = %i, playerName = %s, numDiffs = %i\n", m_iVersion, m_iFolderCount,
             playerName.c_str(), m_iNumBeatmapsToLoad);

    if(m_iVersion < 20170222) {
        debugLog("Database: Version is quite old, below 20170222 ...\n");
        m_osu->getNotificationOverlay()->addNotification("osu!.db version too old, update osu! and try again!",
                                                         0xffff0000);
        m_fLoadingProgress = 1.0f;
        return;
    }

    if(!osu_database_ignore_version_warnings.getBool()) {
        if(m_iVersion < 20190207)  // xexxar angles star recalc
        {
            m_osu->getNotificationOverlay()->addNotification(
                "osu!.db version is old,  let osu! update when convenient.", 0xffffff00, false, 3.0f);
        }
    }

    // hard cap upper db version
    if(m_iVersion > osu_database_version.getInt() && !osu_database_ignore_version.getBool()) {
        m_osu->getNotificationOverlay()->addNotification(
            UString::format("osu!.db version unknown (%i),  using fallback loader.", m_iVersion), 0xffffff00, false,
            5.0f);

        fallbackToRawLoad = true;
        m_fLoadingProgress = 0.0f;

        return;
    }

    // read beatmapInfos, and also build two hashmaps (diff hash -> OsuBeatmapDifficulty, diff hash -> OsuBeatmap)
    struct BeatmapSet {
        int setID;
        std::string path;
        std::vector<OsuDatabaseBeatmap *> diffs2;
    };
    std::vector<BeatmapSet> beatmapSets;
    std::unordered_map<int, size_t> setIDToIndex;
    std::unordered_map<MD5Hash, OsuDatabaseBeatmap *> hashToDiff2;
    std::unordered_map<MD5Hash, OsuDatabaseBeatmap *> hashToBeatmap;
    for(int i = 0; i < m_iNumBeatmapsToLoad; i++) {
        if(m_bInterruptLoad.load()) break;  // cancellation point

        if(Osu::debug->getBool()) debugLog("Database: Reading beatmap %i/%i ...\n", (i + 1), m_iNumBeatmapsToLoad);

        m_fLoadingProgress = 0.24f + 0.5f * ((float)(i + 1) / (float)m_iNumBeatmapsToLoad);

        if(m_iVersion < 20191107)  // see https://osu.ppy.sh/home/changelog/stable40/20191107.2
        {
            // also see https://github.com/ppy/osu-wiki/commit/b90f312e06b4f86e509b397565f1fe014bb15943
            // no idea why peppy decided to change the wiki version from 20191107 to 20191106, because that's not what
            // stable is doing. the correct version is still 20191107

            /*unsigned int size = */ read_int32(db);  // size in bytes of the beatmap entry
        }

        UString artistName = read_string(db).trim();
        UString artistNameUnicode = read_string(db);
        UString songTitle = read_string(db).trim();
        UString songTitleUnicode = read_string(db);
        UString creatorName = read_string(db).trim();
        UString difficultyName = read_string(db).trim();
        std::string audioFileName = read_stdstring(db);
        auto hash_str = read_stdstring(db);
        MD5Hash md5hash = hash_str.c_str();
        std::string osuFileName = read_stdstring(db);
        /*unsigned char rankedStatus = */ read_byte(db);
        unsigned short numCircles = read_short(db);
        unsigned short numSliders = read_short(db);
        unsigned short numSpinners = read_short(db);
        long long lastModificationTime = read_int64(db);
        float AR = read_float32(db);
        float CS = read_float32(db);
        float HP = read_float32(db);
        float OD = read_float32(db);
        double sliderMultiplier = read_float64(db);

        unsigned int numOsuStandardStarRatings = read_int32(db);
        // debugLog("%i star ratings for osu!standard\n", numOsuStandardStarRatings);
        float numOsuStandardStars = 0.0f;
        for(int s = 0; s < numOsuStandardStarRatings; s++) {
            read_byte(db);  // ObjType
            unsigned int mods = read_int32(db);
            read_byte(db);  // ObjType
            double starRating = read_float64(db);
            // debugLog("%f stars for %u\n", starRating, mods);

            if(mods == 0) numOsuStandardStars = starRating;
        }
        // NOTE: if we have our own stars cached then prefer that
        {
            if(osu_database_stars_cache_enabled.getBool())
                numOsuStandardStars = 0.0f;  // NOTE: force don't use stable stars

            const auto result = m_starsCache.find(md5hash);
            if(result != m_starsCache.end()) numOsuStandardStars = result->second.starsNomod;
        }

        unsigned int numTaikoStarRatings = read_int32(db);
        // debugLog("%i star ratings for taiko\n", numTaikoStarRatings);
        for(int s = 0; s < numTaikoStarRatings; s++) {
            read_byte(db);  // ObjType
            read_int32(db);
            read_byte(db);  // ObjType
            read_float64(db);
        }

        unsigned int numCtbStarRatings = read_int32(db);
        // debugLog("%i star ratings for ctb\n", numCtbStarRatings);
        for(int s = 0; s < numCtbStarRatings; s++) {
            read_byte(db);  // ObjType
            read_int32(db);
            read_byte(db);  // ObjType
            read_float64(db);
        }

        unsigned int numManiaStarRatings = read_int32(db);
        // debugLog("%i star ratings for mania\n", numManiaStarRatings);
        for(int s = 0; s < numManiaStarRatings; s++) {
            read_byte(db);  // ObjType
            read_int32(db);
            read_byte(db);  // ObjType
            read_float64(db);
        }

        /*unsigned int drainTime = */ read_int32(db);  // seconds
        int duration = read_int32(db);                 // milliseconds
        duration = duration >= 0 ? duration : 0;       // sanity clamp
        int previewTime = read_int32(db);

        // debugLog("drainTime = %i sec, duration = %i ms, previewTime = %i ms\n", drainTime, duration, previewTime);

        unsigned int numTimingPoints = read_int32(db);
        // debugLog("%i timingpoints\n", numTimingPoints);
        std::vector<TIMINGPOINT> timingPoints;
        for(int t = 0; t < numTimingPoints; t++) {
            timingPoints.push_back(read_timing_point(db));
        }

        int beatmapID = read_int32(db);     // fucking bullshit, this is NOT an unsigned integer as is described on the
                                            // wiki, it can and is -1 sometimes
        int beatmapSetID = read_int32(db);  // same here
        /*unsigned int threadID = */ read_int32(db);

        /*unsigned char osuStandardGrade = */ read_byte(db);
        /*unsigned char taikoGrade = */ read_byte(db);
        /*unsigned char ctbGrade = */ read_byte(db);
        /*unsigned char maniaGrade = */ read_byte(db);
        // debugLog("beatmapID = %i, beatmapSetID = %i, threadID = %i, osuStandardGrade = %i, taikoGrade = %i, ctbGrade
        // = %i, maniaGrade = %i\n", beatmapID, beatmapSetID, threadID, osuStandardGrade, taikoGrade, ctbGrade,
        // maniaGrade);

        short localOffset = read_short(db);
        float stackLeniency = read_float32(db);
        unsigned char mode = read_byte(db);
        // debugLog("localOffset = %i, stackLeniency = %f, mode = %i\n", localOffset, stackLeniency, mode);

        auto songSource = read_stdstring(db);
        auto songTags = read_stdstring(db);
        trim(&songSource);
        trim(&songTags);
        // debugLog("songSource = %s, songTags = %s\n", songSource.toUtf8(), songTags.toUtf8());

        short onlineOffset = read_short(db);
        skip_string(db);  // song title font
        /*bool unplayed = */ read_byte(db);
        /*long long lastTimePlayed = */ read_int64(db);
        /*bool isOsz2 = */ read_byte(db);

        // somehow, some beatmaps may have spaces at the start/end of their
        // path, breaking the Windows API (e.g. https://osu.ppy.sh/s/215347)
        auto path = read_stdstring(db);
        trim(&path);

        /*long long lastOnlineCheck = */ read_int64(db);
        // debugLog("onlineOffset = %i, songTitleFont = %s, unplayed = %i, lastTimePlayed = %lu, isOsz2 = %i, path = %s,
        // lastOnlineCheck = %lu\n", onlineOffset, songTitleFont.toUtf8(), (int)unplayed, lastTimePlayed, (int)isOsz2,
        // path.c_str(), lastOnlineCheck);

        /*bool ignoreBeatmapSounds = */ read_byte(db);
        /*bool ignoreBeatmapSkin = */ read_byte(db);
        /*bool disableStoryboard = */ read_byte(db);
        /*bool disableVideo = */ read_byte(db);
        /*bool visualOverride = */ read_byte(db);
        /*int lastEditTime = */ read_int32(db);
        /*unsigned char maniaScrollSpeed = */ read_byte(db);
        // debugLog("ignoreBeatmapSounds = %i, ignoreBeatmapSkin = %i, disableStoryboard = %i, disableVideo = %i,
        // visualOverride = %i, maniaScrollSpeed = %i\n", (int)ignoreBeatmapSounds, (int)ignoreBeatmapSkin,
        // (int)disableStoryboard, (int)disableVideo, (int)visualOverride, maniaScrollSpeed);

        // HACKHACK: workaround for linux and macos: it can happen that nested beatmaps are stored in the database, and
        // that osu! stores that filepath with a backslash (because windows)
        if(env->getOS() == Environment::OS::OS_LINUX || env->getOS() == Environment::OS::OS_MACOS) {
            for(int c = 0; c < path.length(); c++) {
                if(path[c] == '\\') {
                    path[c] = '/';
                }
            }
        }

        // build beatmap & diffs from all the data
        std::string beatmapPath = songFolder;
        beatmapPath.append(path.c_str());
        beatmapPath.append("/");
        std::string fullFilePath = beatmapPath;
        fullFilePath.append(osuFileName);

        // skip invalid/corrupt entries
        // the good way would be to check if the .osu file actually exists on disk, but that is slow af, ain't nobody
        // got time for that so, since I've seen some concrete examples of what happens in such cases, we just exclude
        // those
        if(artistName.length() < 1 && songTitle.length() < 1 && creatorName.length() < 1 &&
           difficultyName.length() < 1 && md5hash.hash[0] == 0)
            continue;

        // fill diff with data
        if(mode == 0) {
            OsuDatabaseBeatmap *diff2 = new OsuDatabaseBeatmap(m_osu, fullFilePath, beatmapPath);
            {
                diff2->m_sTitle = songTitle;
                diff2->m_sAudioFileName = audioFileName;
                diff2->m_iLengthMS = duration;

                diff2->m_fStackLeniency = stackLeniency;

                diff2->m_sArtist = artistName;
                diff2->m_sCreator = creatorName;
                diff2->m_sDifficultyName = difficultyName;
                diff2->m_sSource = songSource;
                diff2->m_sTags = songTags;
                diff2->m_sMD5Hash = md5hash;
                diff2->m_iID = beatmapID;
                diff2->m_iSetID = beatmapSetID;

                diff2->m_fAR = AR;
                diff2->m_fCS = CS;
                diff2->m_fHP = HP;
                diff2->m_fOD = OD;
                diff2->m_fSliderMultiplier = sliderMultiplier;

                // diff2->m_sBackgroundImageFileName = "";

                diff2->m_iPreviewTime = previewTime;
                diff2->m_iLastModificationTime = lastModificationTime;

                diff2->m_sFullSoundFilePath = beatmapPath;
                diff2->m_sFullSoundFilePath.append(diff2->m_sAudioFileName);
                diff2->m_iLocalOffset = localOffset;
                diff2->m_iOnlineOffset = (long)onlineOffset;
                diff2->m_iNumObjects = numCircles + numSliders + numSpinners;
                diff2->m_iNumCircles = numCircles;
                diff2->m_iNumSliders = numSliders;
                diff2->m_iNumSpinners = numSpinners;
                diff2->m_fStarsNomod = numOsuStandardStars;

                // calculate bpm range
                float minBeatLength = 0;
                float maxBeatLength = std::numeric_limits<float>::max();
                std::vector<TIMINGPOINT> uninheritedTimingpoints;
                for(int j = 0; j < timingPoints.size(); j++) {
                    const TIMINGPOINT &t = timingPoints[j];

                    if(t.msPerBeat >= 0)  // NOT inherited
                    {
                        uninheritedTimingpoints.push_back(t);

                        if(t.msPerBeat > minBeatLength) minBeatLength = t.msPerBeat;
                        if(t.msPerBeat < maxBeatLength) maxBeatLength = t.msPerBeat;
                    }
                }

                // convert from msPerBeat to BPM
                const float msPerMinute = 1 * 60 * 1000;
                if(minBeatLength != 0) minBeatLength = msPerMinute / minBeatLength;
                if(maxBeatLength != 0) maxBeatLength = msPerMinute / maxBeatLength;

                diff2->m_iMinBPM = (int)std::round(minBeatLength);
                diff2->m_iMaxBPM = (int)std::round(maxBeatLength);

                struct MostCommonBPMHelper {
                    static int calculateMostCommonBPM(const std::vector<TIMINGPOINT> &uninheritedTimingpoints,
                                                      long lastTime) {
                        if(uninheritedTimingpoints.size() < 1) return 0;

                        struct Tuple {
                            float beatLength;
                            long duration;

                            size_t sortHack;
                        };

                        // "Construct a set of (beatLength, duration) tuples for each individual timing point."
                        std::vector<Tuple> tuples;
                        tuples.reserve(uninheritedTimingpoints.size());
                        for(size_t i = 0; i < uninheritedTimingpoints.size(); i++) {
                            const TIMINGPOINT &t = uninheritedTimingpoints[i];

                            Tuple tuple;
                            {
                                if(t.offset > lastTime) {
                                    tuple.beatLength = std::round(t.msPerBeat * 1000.0f) / 1000.0f;
                                    tuple.duration = 0;
                                } else {
                                    // "osu-stable forced the first control point to start at 0."
                                    // "This is reproduced here to maintain compatibility around osu!mania scroll speed
                                    // and song select display."
                                    const long currentTime = (i == 0 ? 0 : t.offset);
                                    const long nextTime = (i >= uninheritedTimingpoints.size() - 1
                                                               ? lastTime
                                                               : uninheritedTimingpoints[i + 1].offset);

                                    tuple.beatLength = std::round(t.msPerBeat * 1000.0f) / 1000.0f;
                                    tuple.duration = std::max(nextTime - currentTime, (long)0);
                                }

                                tuple.sortHack = i;
                            }
                            tuples.push_back(tuple);
                        }

                        // "Aggregate durations into a set of (beatLength, duration) tuples for each beat length"
                        std::vector<Tuple> aggregations;
                        aggregations.reserve(tuples.size());
                        for(size_t i = 0; i < tuples.size(); i++) {
                            const Tuple &t = tuples[i];

                            bool foundExistingAggregation = false;
                            size_t aggregationIndex = 0;
                            for(size_t j = 0; j < aggregations.size(); j++) {
                                if(aggregations[j].beatLength == t.beatLength) {
                                    foundExistingAggregation = true;
                                    aggregationIndex = j;
                                    break;
                                }
                            }

                            if(!foundExistingAggregation)
                                aggregations.push_back(t);
                            else
                                aggregations[aggregationIndex].duration += t.duration;
                        }

                        // "Get the most common one, or 0 as a suitable default"
                        struct SortByDuration {
                            bool operator()(Tuple const &a, Tuple const &b) const {
                                // first condition: duration
                                // second condition: if duration is the same, higher BPM goes before lower BPM

                                // strict weak ordering!
                                if(a.duration == b.duration && a.beatLength == b.beatLength)
                                    return a.sortHack > b.sortHack;
                                else if(a.duration == b.duration)
                                    return (a.beatLength < b.beatLength);
                                else
                                    return (a.duration > b.duration);
                            }
                        };
                        std::sort(aggregations.begin(), aggregations.end(), SortByDuration());

                        float mostCommonBPM = aggregations[0].beatLength;
                        {
                            // convert from msPerBeat to BPM
                            const float msPerMinute = 1.0f * 60.0f * 1000.0f;
                            if(mostCommonBPM != 0.0f) mostCommonBPM = msPerMinute / mostCommonBPM;
                        }
                        return (int)std::round(mostCommonBPM);
                    }
                };
                diff2->m_iMostCommonBPM = MostCommonBPMHelper::calculateMostCommonBPM(
                    uninheritedTimingpoints,
                    (timingPoints.size() > 0 ? timingPoints[timingPoints.size() - 1].offset : 0));

                // build temp partial timingpoints, only used for menu animations
                for(int t = 0; t < timingPoints.size(); t++) {
                    OsuDatabaseBeatmap::TIMINGPOINT tp;
                    {
                        tp.offset = timingPoints[t].offset;
                        tp.msPerBeat = timingPoints[t].msPerBeat;
                        tp.timingChange = timingPoints[t].timingChange;
                        tp.kiai = false;
                    }
                    diff2->m_timingpoints.push_back(tp);
                }
            }

            // special case: legacy fallback behavior for invalid beatmapSetID, try to parse the ID from the path
            if(beatmapSetID < 1 && path.length() > 0) {
                auto upath = UString(path.c_str());
                const std::vector<UString> pathTokens =
                    upath.split("\\");  // NOTE: this is hardcoded to backslash since osu is windows only
                if(pathTokens.size() > 0 && pathTokens[0].length() > 0) {
                    const std::vector<UString> spaceTokens = pathTokens[0].split(" ");
                    if(spaceTokens.size() > 0 && spaceTokens[0].length() > 0) {
                        try {
                            beatmapSetID = spaceTokens[0].toInt();
                        } catch(...) {
                            beatmapSetID = -1;
                        }
                    }
                }
            }

            // (the diff is now fully built)

            // now, search if the current set (to which this diff would belong) already exists and add it there, or if
            // it doesn't exist then create the set
            const auto result = setIDToIndex.find(beatmapSetID);
            const bool beatmapSetExists = (result != setIDToIndex.end());
            if(beatmapSetExists)
                beatmapSets[result->second].diffs2.push_back(diff2);
            else {
                setIDToIndex[beatmapSetID] = beatmapSets.size();

                BeatmapSet s;

                s.setID = beatmapSetID;
                s.path = beatmapPath;
                s.diffs2.push_back(diff2);

                beatmapSets.push_back(s);
            }

            // and add an entry in our hashmap
            hashToDiff2[diff2->getMD5Hash()] = diff2;
        }
    }

    // we now have a collection of BeatmapSets (where one set is equal to one beatmap and all of its diffs), build the
    // actual OsuBeatmap objects first, build all beatmaps which have a valid setID (trusting the values from the osu
    // database)
    std::unordered_map<UString, OsuDatabaseBeatmap *> titleArtistToBeatmap;
    for(int i = 0; i < beatmapSets.size(); i++) {
        if(m_bInterruptLoad.load()) break;  // cancellation point

        if(beatmapSets[i].diffs2.size() > 0)  // sanity check
        {
            if(beatmapSets[i].setID > 0) {
                OsuDatabaseBeatmap *bm = new OsuDatabaseBeatmap(m_osu, beatmapSets[i].diffs2);

                m_databaseBeatmaps.push_back(bm);

                // and add an entry in our hashmap
                for(int d = 0; d < beatmapSets[i].diffs2.size(); d++) {
                    const MD5Hash &md5hash = beatmapSets[i].diffs2[d]->getMD5Hash();
                    hashToBeatmap[md5hash] = bm;
                }

                // and in the other hashmap
                UString titleArtist = bm->getTitle();
                titleArtist.append(bm->getArtist());
                if(titleArtist.length() > 0) titleArtistToBeatmap[UString(titleArtist.toUtf8())] = bm;
            }
        }
    }

    // second, handle all diffs which have an invalid setID, and group them exclusively by artist and title and creator
    // (diffs with the same artist and title and creator will end up in the same beatmap object) this goes through every
    // individual diff in a "set" (not really a set because its ID is either 0 or -1) instead of trusting the ID values
    // from the osu database
    for(int i = 0; i < beatmapSets.size(); i++) {
        if(m_bInterruptLoad.load()) break;  // cancellation point

        if(beatmapSets[i].diffs2.size() > 0)  // sanity check
        {
            if(beatmapSets[i].setID < 1) {
                for(int b = 0; b < beatmapSets[i].diffs2.size(); b++) {
                    if(m_bInterruptLoad.load()) break;  // cancellation point

                    OsuDatabaseBeatmap *diff2 = beatmapSets[i].diffs2[b];

                    // try finding an already existing beatmap with matching artist and title and creator (into which we
                    // could inject this lone diff)
                    bool existsAlready = false;

                    // new: use hashmap
                    UString titleArtistCreator = diff2->getTitle();
                    titleArtistCreator.append(diff2->getArtist());
                    titleArtistCreator.append(diff2->getCreator());
                    if(titleArtistCreator.length() > 0) {
                        const auto result = titleArtistToBeatmap.find(UString(titleArtistCreator.toUtf8()));
                        if(result != titleArtistToBeatmap.end()) {
                            existsAlready = true;

                            // we have found a matching beatmap, add ourself to its diffs
                            const_cast<std::vector<OsuDatabaseBeatmap *> &>(result->second->getDifficulties())
                                .push_back(diff2);

                            // and add an entry in our hashmap
                            hashToBeatmap[diff2->getMD5Hash()] = result->second;
                        }
                    }

                    // if we couldn't find any beatmap with our title and artist, create a new one
                    if(!existsAlready) {
                        std::vector<OsuDatabaseBeatmap *> diffs2;
                        diffs2.push_back(beatmapSets[i].diffs2[b]);

                        OsuDatabaseBeatmap *bm = new OsuDatabaseBeatmap(m_osu, diffs2);

                        m_databaseBeatmaps.push_back(bm);

                        // and add an entry in our hashmap
                        for(int d = 0; d < diffs2.size(); d++) {
                            const MD5Hash &md5hash = diffs2[d]->getMD5Hash();
                            hashToBeatmap[md5hash] = bm;
                        }
                    }
                }
            }
        }
    }

    m_importTimer->update();
    debugLog("Refresh finished, added %i beatmaps in %f seconds.\n", m_databaseBeatmaps.size(),
             m_importTimer->getElapsedTime());

    // signal that we are almost done
    m_fLoadingProgress = 0.75f;

    load_collections();

    // signal that we are done
    m_fLoadingProgress = 1.0f;
}

void OsuDatabase::loadStars() {
    if(!osu_database_stars_cache_enabled.getBool()) return;

    debugLog("OsuDatabase::loadStars()\n");

    const int starsCacheVersion = 20221108;

    Packet cache = load_db("stars.cache");
    if(cache.size > 0) {
        m_starsCache.clear();

        const int cacheVersion = read_int32(&cache);

        if(cacheVersion <= starsCacheVersion) {
            skip_string(&cache);  // ignore md5
            const int64_t numStarsCacheEntries = read_int64(&cache);

            debugLog("Stars cache: version = %i, numStarsCacheEntries = %i\n", cacheVersion, numStarsCacheEntries);

            for(int64_t i = 0; i < numStarsCacheEntries; i++) {
                auto hash_str = read_stdstring(&cache);
                const MD5Hash beatmapMD5Hash = hash_str.c_str();
                const float starsNomod = read_float32(&cache);

                STARS_CACHE_ENTRY entry;
                { entry.starsNomod = starsNomod; }
                m_starsCache[beatmapMD5Hash] = entry;
            }
        } else
            debugLog("Invalid stars cache version, ignoring.\n");
    } else
        debugLog("No stars cache found.\n");
}

void OsuDatabase::saveStars() {
    if(!osu_database_stars_cache_enabled.getBool()) return;

    debugLog("Osu: Saving stars ...\n");

    const int starsCacheVersion = 20221108;

    // count
    int64_t numStarsCacheEntries = 0;
    for(OsuDatabaseBeatmap *beatmap : m_databaseBeatmaps) {
        for(OsuDatabaseBeatmap *diff2 : beatmap->getDifficulties()) {
            if(diff2->getStarsNomod() > 0.0f && diff2->getStarsNomod() != 0.0001f) numStarsCacheEntries++;
        }
    }

    if(numStarsCacheEntries < 1) {
        debugLog("No stars cached, nothing to write.\n");
        return;
    }

    // write
    Packet cache;
    write_int32(&cache, starsCacheVersion);
    write_string(&cache, "00000000000000000000000000000000");
    write_int64(&cache, numStarsCacheEntries);
    for(OsuDatabaseBeatmap *beatmap : m_databaseBeatmaps) {
        for(OsuDatabaseBeatmap *diff2 : beatmap->getDifficulties()) {
            if(diff2->getStarsNomod() > 0.0f && diff2->getStarsNomod() != 0.0001f) {
                write_string(&cache, diff2->getMD5Hash().hash);
                write_float32(&cache, diff2->getStarsNomod());
            }
        }
    }

    if(!save_db(&cache, "stars.cache")) {
        debugLog("Couldn't write stars.cache!\n");
    }
}

void OsuDatabase::loadScores() {
    if(m_bScoresLoaded) return;

    debugLog("OsuDatabase::loadScores()\n");

    // reset
    m_scores.clear();

    // load custom scores
    // NOTE: custom scores are loaded before legacy scores because we want to be able to skip loading legacy scores
    // which were already previously imported at some point
    int nb_neosu_scores = 0;
    size_t customScoresFileSize = 0;
    if(osu_scores_custom_enabled.getBool()) {
        const unsigned char hackIsImportedLegacyScoreFlag =
            0xA9;  // TODO: remove this once all builds on steam (even previous-version) have loading version cap logic

        Packet db = load_db("scores.db");
        if(db.size > 0) {
            customScoresFileSize = db.size;

            const uint32_t dbVersion = read_int32(&db);
            const uint32_t numBeatmaps = read_int32(&db);
            debugLog("Custom scores: version = %u, numBeatmaps = %u\n", dbVersion, numBeatmaps);

            if(dbVersion <= OsuScore::VERSION) {
                for(int b = 0; b < numBeatmaps; b++) {
                    auto hash_str = read_stdstring(&db);
                    const int numScores = read_int32(&db);

                    if(hash_str.size() < 32) {
                        if(Osu::debug->getBool()) {
                            debugLog("WARNING: Invalid score with md5hash.length() = %i!\n", hash_str.size());
                        }
                        continue;
                    } else if(hash_str.size() > 32) {
                        debugLog("ERROR: Corrupt score database/entry detected, stopping.\n");
                        break;
                    }

                    if(Osu::debug->getBool()) {
                        debugLog("Beatmap[%i]: md5hash = %s, numScores = %i\n", b, hash_str.c_str(), numScores);
                    }

                    const MD5Hash md5hash = hash_str.c_str();
                    for(int s = 0; s < numScores; s++) {
                        Score sc;
                        sc.isLegacyScore = false;
                        sc.isImportedLegacyScore = false;
                        sc.maxPossibleCombo = -1;
                        sc.numHitObjects = -1;
                        sc.numCircles = -1;
                        sc.perfect = false;

                        const unsigned char gamemode = read_byte(&db);  // NOTE: abused as isImportedLegacyScore flag
                        sc.version = read_int32(&db);

                        if(dbVersion == 20210103 && sc.version > 20190103) {
                            sc.isImportedLegacyScore = read_byte(&db);
                        } else if(dbVersion > 20210103 && sc.version > 20190103) {
                            // HACKHACK: for explanation see hackIsImportedLegacyScoreFlag
                            sc.isImportedLegacyScore = (gamemode & hackIsImportedLegacyScoreFlag);
                        }

                        sc.unixTimestamp = read_int64(&db);

                        // default
                        sc.playerName = read_stdstring(&db);

                        sc.num300s = read_short(&db);
                        sc.num100s = read_short(&db);
                        sc.num50s = read_short(&db);
                        sc.numGekis = read_short(&db);
                        sc.numKatus = read_short(&db);
                        sc.numMisses = read_short(&db);

                        sc.score = read_int64(&db);
                        sc.comboMax = read_short(&db);
                        sc.modsLegacy = read_int32(&db);

                        // custom
                        sc.numSliderBreaks = read_short(&db);
                        sc.pp = read_float32(&db);
                        sc.unstableRate = read_float32(&db);
                        sc.hitErrorAvgMin = read_float32(&db);
                        sc.hitErrorAvgMax = read_float32(&db);
                        sc.starsTomTotal = read_float32(&db);
                        sc.starsTomAim = read_float32(&db);
                        sc.starsTomSpeed = read_float32(&db);
                        sc.speedMultiplier = read_float32(&db);
                        sc.CS = read_float32(&db);
                        sc.AR = read_float32(&db);
                        sc.OD = read_float32(&db);
                        sc.HP = read_float32(&db);

                        if(sc.version > 20180722) {
                            sc.maxPossibleCombo = read_int32(&db);
                            sc.numHitObjects = read_int32(&db);
                            sc.numCircles = read_int32(&db);
                            sc.perfect = sc.comboMax >= sc.maxPossibleCombo;
                        }

                        if(sc.version >= 20240412) {
                            sc.has_replay = true;
                            sc.online_score_id = read_int32(&db);
                            sc.server = read_stdstring(&db);
                        }

                        sc.experimentalModsConVars = read_stdstring(&db);

                        if(gamemode == 0x0 || (dbVersion > 20210103 && sc.version > 20190103)) {
                            // runtime
                            sc.sortHack = m_iSortHackCounter++;
                            sc.md5hash = md5hash;

                            addScoreRaw(md5hash, sc);
                            nb_neosu_scores++;
                        }
                    }
                }
            } else {
                debugLog("Newer scores.db version is not backwards compatible with old clients.\n");
            }
        }

        free(db.memory);
    }

    // load legacy osu scores
    int nb_peppy_scores = 0;
    if(osu_scores_legacy_enabled.getBool()) {
        std::string scoresPath = osu_folder.getString().toUtf8();
        // XXX: name it something else than "scores.db" to prevent conflict with osu!stable/steam mcosu
        scoresPath.append("scores.db");

        Packet db = load_db(scoresPath);

        // HACKHACK: heuristic sanity check (some people have their osu!folder
        // point directly to neosu, which would break legacy score db loading
        // here since there is no magic number)
        if(db.size > 0 && db.size != customScoresFileSize) {
            const int dbVersion = read_int32(&db);
            const int numBeatmaps = read_int32(&db);

            debugLog("Legacy scores: version = %i, numBeatmaps = %i\n", dbVersion, numBeatmaps);

            for(int b = 0; b < numBeatmaps; b++) {
                auto hash_str = read_stdstring(&db);

                if(hash_str.size() < 32) {
                    if(Osu::debug->getBool()) {
                        debugLog("WARNING: Invalid score with md5hash.length() = %i!\n", hash_str.size());
                    }
                    continue;
                } else if(hash_str.size() > 32) {
                    debugLog("ERROR: Corrupt score database/entry detected, stopping.\n");
                    break;
                }

                const MD5Hash md5hash = hash_str.c_str();
                const int numScores = read_int32(&db);

                if(Osu::debug->getBool())
                    debugLog("Beatmap[%i]: md5hash = %s, numScores = %i\n", b, md5hash.toUtf8(), numScores);

                for(int s = 0; s < numScores; s++) {
                    Score sc;
                    sc.server = "ppy.sh";
                    sc.online_score_id = 0;
                    sc.isLegacyScore = true;
                    sc.isImportedLegacyScore = false;
                    sc.numSliderBreaks = 0;
                    sc.pp = 0.0f;
                    sc.unstableRate = 0.0f;
                    sc.hitErrorAvgMin = 0.0f;
                    sc.hitErrorAvgMax = 0.0f;
                    sc.starsTomTotal = 0.0f;
                    sc.starsTomAim = 0.0f;
                    sc.starsTomSpeed = 0.0f;
                    sc.CS = 0.0f;
                    sc.AR = 0.0f;
                    sc.OD = 0.0f;
                    sc.HP = 0.0f;
                    sc.maxPossibleCombo = -1;
                    sc.numHitObjects = -1;
                    sc.numCircles = -1;

                    const unsigned char gamemode = read_byte(&db);
                    sc.version = read_int32(&db);
                    skip_string(&db);  // beatmap hash (already have it)

                    sc.playerName = read_stdstring(&db);
                    skip_string(&db);  // replay hash (don't use it)

                    sc.num300s = read_short(&db);
                    sc.num100s = read_short(&db);
                    sc.num50s = read_short(&db);
                    sc.numGekis = read_short(&db);
                    sc.numKatus = read_short(&db);
                    sc.numMisses = read_short(&db);

                    int32_t score = read_int32(&db);
                    sc.score = (score < 0 ? 0 : score);

                    sc.comboMax = read_short(&db);
                    sc.perfect = read_byte(&db);

                    sc.modsLegacy = read_int32(&db);
                    sc.speedMultiplier = 1.0f;
                    if(sc.modsLegacy & ModFlags::HalfTime)
                        sc.speedMultiplier = 0.75f;
                    else if((sc.modsLegacy & ModFlags::DoubleTime) || (sc.modsLegacy & ModFlags::Nightcore))
                        sc.speedMultiplier = 1.5f;

                    skip_string(&db);  // hp graph

                    uint64_t full_tms = read_int64(&db);
                    sc.unixTimestamp = (full_tms - 621355968000000000) / 10000000;
                    sc.legacyReplayTimestamp = full_tms - 504911232000000000;

                    // Always -1, but let's skip it properly just in case
                    int32_t old_replay_size = read_int32(&db);
                    if(old_replay_size > 0) {
                        db.pos += old_replay_size;
                    }

                    // Just assume we have the replay ¯\_(ツ)_/¯
                    sc.has_replay = true;

                    if(sc.version >= 20140721)
                        sc.online_score_id = read_int64(&db);
                    else if(sc.version >= 20121008)
                        sc.online_score_id = read_int32(&db);

                    if(sc.modsLegacy & ModFlags::Target) /*double totalAccuracy = */
                        read_float64(&db);

                    if(gamemode == 0) {  // gamemode filter (osu!standard)
                        // runtime
                        sc.sortHack = m_iSortHackCounter++;
                        sc.md5hash = md5hash;

                        nb_peppy_scores++;

                        // NOTE: avoid adding an already imported legacy score (since that just spams the
                        // scorebrowser with useless information)
                        bool isScoreAlreadyImported = false;
                        {
                            const std::vector<Score> &otherScores = m_scores[sc.md5hash];

                            for(size_t s = 0; s < otherScores.size(); s++) {
                                if(sc.isLegacyScoreEqualToImportedLegacyScore(otherScores[s])) {
                                    isScoreAlreadyImported = true;
                                    break;
                                }
                            }
                        }

                        if(!isScoreAlreadyImported) addScoreRaw(md5hash, sc);
                    }
                }
            }
        }

        free(db.memory);
    }

    // XXX: Also load steam mcosu scores?
    debugLog("Loaded %i scores from neosu and %i scores from osu!stable.\n", nb_neosu_scores, nb_peppy_scores);

    if(m_scores.size() > 0) m_bScoresLoaded = true;
}

void OsuDatabase::saveScores() {
    if(!m_bDidScoresChangeForSave) return;
    m_bDidScoresChangeForSave = false;

    if(m_scores.empty()) return;
    const unsigned char hackIsImportedLegacyScoreFlag =
        0xA9;  // TODO: remove this once all builds on steam (even previous-version) have loading version cap logic

    debugLog("Osu: Saving scores ...\n");
    const double startTime = engine->getTimeReal();

    // count number of beatmaps with valid scores
    int numBeatmaps = 0;
    for(std::unordered_map<MD5Hash, std::vector<Score>>::iterator it = m_scores.begin(); it != m_scores.end(); ++it) {
        for(int i = 0; i < it->second.size(); i++) {
            if(!it->second[i].isLegacyScore) {
                numBeatmaps++;
                break;
            }
        }
    }

    // write header
    Packet db;
    write_int32(&db, OsuScore::VERSION);
    write_int32(&db, numBeatmaps);

    // write scores for each beatmap
    for(auto &beatmap : m_scores) {
        int numNonLegacyScores = 0;
        for(int i = 0; i < beatmap.second.size(); i++) {
            if(!beatmap.second[i].isLegacyScore) numNonLegacyScores++;
        }

        if(numNonLegacyScores == 0) continue;

        write_string(&db, beatmap.first.hash);  // beatmap md5 hash
        write_int32(&db, numNonLegacyScores);   // numScores

        for(auto &score : beatmap.second) {
            if(score.isLegacyScore) continue;

            uint8_t gamemode = 0;
            if(score.version > 20190103 && score.isImportedLegacyScore) gamemode = hackIsImportedLegacyScoreFlag;
            write_byte(&db, gamemode);

            write_int32(&db, score.version);
            write_int64(&db, score.unixTimestamp);

            // default
            write_string(&db, score.playerName.c_str());

            write_short(&db, score.num300s);
            write_short(&db, score.num100s);
            write_short(&db, score.num50s);
            write_short(&db, score.numGekis);
            write_short(&db, score.numKatus);
            write_short(&db, score.numMisses);

            write_int64(&db, score.score);
            write_short(&db, score.comboMax);
            write_int32(&db, score.modsLegacy);

            // custom
            write_short(&db, score.numSliderBreaks);
            write_float32(&db, score.pp);
            write_float32(&db, score.unstableRate);
            write_float32(&db, score.hitErrorAvgMin);
            write_float32(&db, score.hitErrorAvgMax);
            write_float32(&db, score.starsTomTotal);
            write_float32(&db, score.starsTomAim);
            write_float32(&db, score.starsTomSpeed);
            write_float32(&db, score.speedMultiplier);
            write_float32(&db, score.CS);
            write_float32(&db, score.AR);
            write_float32(&db, score.OD);
            write_float32(&db, score.HP);

            if(score.version > 20180722) {
                write_int32(&db, score.maxPossibleCombo);
                write_int32(&db, score.numHitObjects);
                write_int32(&db, score.numCircles);
            }

            if(score.version >= 20240412) {
                write_int32(&db, score.online_score_id);
                write_string(&db, score.server.c_str());
            }

            write_string(&db, score.experimentalModsConVars.c_str());
        }
    }

    if(!save_db(&db, ("scores.db"))) {
        debugLog("Couldn't write scores.db!\n");
    }

    debugLog("Took %f seconds.\n", (engine->getTimeReal() - startTime));
}

OsuDatabaseBeatmap *OsuDatabase::loadRawBeatmap(std::string beatmapPath) {
    if(Osu::debug->getBool()) debugLog("OsuBeatmapDatabase::loadRawBeatmap() : %s\n", beatmapPath.c_str());

    // try loading all diffs
    std::vector<OsuDatabaseBeatmap *> diffs2;
    {
        std::vector<std::string> beatmapFiles = env->getFilesInFolder(beatmapPath);
        for(int i = 0; i < beatmapFiles.size(); i++) {
            std::string ext = env->getFileExtensionFromFilePath(beatmapFiles[i]);

            std::string fullFilePath = beatmapPath;
            fullFilePath.append(beatmapFiles[i]);

            // load diffs
            if(ext.compare("osu") == 0) {
                OsuDatabaseBeatmap *diff2 = new OsuDatabaseBeatmap(m_osu, fullFilePath, beatmapPath);

                // try to load it. if successful save it, else cleanup and continue to the next osu file
                if(!OsuDatabaseBeatmap::loadMetadata(diff2)) {
                    if(Osu::debug->getBool()) {
                        debugLog("OsuBeatmapDatabase::loadRawBeatmap() : Couldn't loadMetadata(), deleting object.\n");
                        if(diff2->getGameMode() == 0)
                            engine->showMessageWarning("OsuBeatmapDatabase::loadRawBeatmap()",
                                                       "Couldn't loadMetadata()\n");
                    }
                    SAFE_DELETE(diff2);
                    continue;
                }

                // (metadata loaded successfully)

                // NOTE: if we have our own stars cached then use that
                {
                    const auto result = m_starsCache.find(diff2->getMD5Hash());
                    if(result != m_starsCache.end()) diff2->m_fStarsNomod = result->second.starsNomod;
                }

                diffs2.push_back(diff2);
            }
        }
    }

    // build beatmap from diffs
    OsuDatabaseBeatmap *beatmap = NULL;
    {
        if(diffs2.size() > 0) {
            beatmap = new OsuDatabaseBeatmap(m_osu, diffs2);

            // and add entries in our hashmaps
            for(size_t i = 0; i < diffs2.size(); i++) {
                OsuDatabaseBeatmap *diff2 = diffs2[i];
                m_rawHashToDiff2[diff2->getMD5Hash()] = diff2;
                m_rawHashToBeatmap[diff2->getMD5Hash()] = beatmap;
            }
        }
    }

    return beatmap;
}

void OsuDatabase::onScoresRename(UString args) {
    if(args.length() < 2) {
        m_osu->getNotificationOverlay()->addNotification(
            UString::format("Usage: %s MyNewName", osu_scores_rename.getName().toUtf8()));
        return;
    }

    auto playerName = m_name_ref->getString();

    debugLog("Renaming scores \"%s\" to \"%s\"\n", playerName.toUtf8(), args.toUtf8());

    int numRenamedScores = 0;
    for(auto &kv : m_scores) {
        for(size_t i = 0; i < kv.second.size(); i++) {
            Score &score = kv.second[i];

            if(!score.isLegacyScore && UString(score.playerName.c_str()) == playerName) {
                numRenamedScores++;
                score.playerName = args.toUtf8();
            }
        }
    }

    if(numRenamedScores < 1)
        m_osu->getNotificationOverlay()->addNotification("No (pp) scores for active user.");
    else {
        m_osu->getNotificationOverlay()->addNotification(UString::format("Renamed %i scores.", numRenamedScores));

        m_bDidScoresChangeForSave = true;
        m_bDidScoresChangeForStats = true;
    }
}

void OsuDatabase::onScoresExport() {
    const std::string exportFilePath = "scores.csv";

    debugLog("Exporting currently loaded scores to \"%s\" (overwriting existing file) ...\n", exportFilePath.c_str());

    std::ofstream out(exportFilePath.c_str());
    if(!out.good()) {
        debugLog("ERROR: Couldn't write %s\n", exportFilePath.c_str());
        return;
    }

    out << "#beatmapMD5hash,beatmapID,beatmapSetID,isImportedLegacyScore,version,unixTimestamp,playerName,num300s,"
           "num100s,num50s,numGekis,numKatus,numMisses,score,comboMax,perfect,modsLegacy,numSliderBreaks,pp,"
           "unstableRate,hitErrorAvgMin,hitErrorAvgMax,starsTomTotal,starsTomAim,starsTomSpeed,speedMultiplier,CS,AR,"
           "OD,HP,maxPossibleCombo,numHitObjects,numCircles,experimentalModsConVars\n";

    for(auto beatmapScores : m_scores) {
        bool triedGettingDatabaseBeatmapOnceForThisBeatmap = false;
        OsuDatabaseBeatmap *beatmap = NULL;

        for(const Score &score : beatmapScores.second) {
            if(!score.isLegacyScore) {
                long id = -1;
                long setId = -1;
                {
                    if(beatmap == NULL && !triedGettingDatabaseBeatmapOnceForThisBeatmap) {
                        triedGettingDatabaseBeatmapOnceForThisBeatmap = true;
                        beatmap = getBeatmapDifficulty(beatmapScores.first);
                    }

                    if(beatmap != NULL) {
                        id = beatmap->getID();
                        setId = beatmap->getSetID();
                    }
                }

                out << beatmapScores.first.toUtf8();  // md5 hash
                out << ",";
                out << id;
                out << ",";
                out << setId;
                out << ",";

                out << score.isImportedLegacyScore;
                out << ",";
                out << score.version;
                out << ",";
                out << score.unixTimestamp;
                out << ",";

                out << score.playerName;
                out << ",";

                out << score.num300s;
                out << ",";
                out << score.num100s;
                out << ",";
                out << score.num50s;
                out << ",";
                out << score.numGekis;
                out << ",";
                out << score.numKatus;
                out << ",";
                out << score.numMisses;
                out << ",";

                out << score.score;
                out << ",";
                out << score.comboMax;
                out << ",";
                out << score.perfect;
                out << ",";
                out << score.modsLegacy;
                out << ",";

                out << score.numSliderBreaks;
                out << ",";
                out << score.pp;
                out << ",";
                out << score.unstableRate;
                out << ",";
                out << score.hitErrorAvgMin;
                out << ",";
                out << score.hitErrorAvgMax;
                out << ",";
                out << score.starsTomTotal;
                out << ",";
                out << score.starsTomAim;
                out << ",";
                out << score.starsTomSpeed;
                out << ",";
                out << score.speedMultiplier;
                out << ",";
                out << score.CS;
                out << ",";
                out << score.AR;
                out << ",";
                out << score.OD;
                out << ",";
                out << score.HP;
                out << ",";
                out << score.maxPossibleCombo;
                out << ",";
                out << score.numHitObjects;
                out << ",";
                out << score.numCircles;
                out << ",";
                out << score.experimentalModsConVars.c_str();

                out << "\n";
            }
        }

        triedGettingDatabaseBeatmapOnceForThisBeatmap = false;
        beatmap = NULL;
    }

    out.close();

    debugLog("Done.\n");
}
