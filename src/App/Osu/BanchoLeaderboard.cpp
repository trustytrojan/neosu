#include "BanchoLeaderboard.h"

#include <stdlib.h>
#include <string.h>

#include <sstream>
#include <vector>

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoUsers.h"
#include "ConVar.h"
#include "Engine.h"
#include "OsuDatabase.h"
#include "OsuModSelector.h"
#include "OsuSongBrowser2.h"

OsuDatabase::Score parse_score(char *score_line) {
    OsuDatabase::Score score;
    score.isLegacyScore = true;
    score.isImportedLegacyScore = true;
    score.speedMultiplier = 1.0;

    char *saveptr = NULL;
    char *str = strtok_r(score_line, "|", &saveptr);
    if(!str) return score;
    // Do nothing with score ID

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return score;
    score.playerName = UString(str);

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return score;
    score.score = strtoul(str, NULL, 10);

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return score;
    score.comboMax = strtoul(str, NULL, 10);

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return score;
    score.num50s = strtoul(str, NULL, 10);

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return score;
    score.num100s = strtoul(str, NULL, 10);

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return score;
    score.num300s = strtoul(str, NULL, 10);

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return score;
    score.numMisses = strtoul(str, NULL, 10);

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return score;
    score.numKatus = strtoul(str, NULL, 10);

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return score;
    score.numGekis = strtoul(str, NULL, 10);

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return score;
    score.perfect = strtoul(str, NULL, 10) == 1;

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return score;
    score.modsLegacy = strtoul(str, NULL, 10);

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return score;
    score.player_id = strtoul(str, NULL, 10);

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return score;
    // Do nothing with rank

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return score;
    score.unixTimestamp = strtoul(str, NULL, 10);

    // And we do nothing with has_replay

    // Set username for given user id, since we now know both
    auto user = get_user_info(score.player_id);
    user->name = score.playerName;

    // Mark as a player. Setting this also makes the has_user_info check pass,
    // which unlocks context menu actions such as sending private messages.
    user->privileges |= 1;

    return score;
}

void fetch_online_scores(OsuDatabaseBeatmap *beatmap) {
    UString path = "/web/osu-osz2-getscores.php?s=0&vv=4&v=1";
    path.append(UString::format("&c=%s", beatmap->getMD5Hash().hash));

    std::string osu_file_path = beatmap->getFilePath();
    auto path_end = osu_file_path.find_last_of('/');
    if(path_end != std::string::npos) {
        osu_file_path.erase(0, path_end + 1);
    }

    CURL *curl = curl_easy_init();
    if(!curl) {
        debugLog("Failed to initialize cURL in fetch_online_scores()!\n");
        return;
    }
    char *encoded_filename = curl_easy_escape(curl, osu_file_path.c_str(), 0);
    if(!encoded_filename) {
        debugLog("Failed to encode map filename!\n");
        curl_easy_cleanup(curl);
        return;
    }
    path.append(UString::format("&f=%s", encoded_filename));
    curl_free(encoded_filename);
    curl_easy_cleanup(curl);
    path.append(UString::format("&m=0&i=%d&mods=%d&h=&a=0&us=%s&ha=%s", beatmap->getSetID(),
                                bancho.osu->m_modSelector->getModFlags(), bancho.username.toUtf8(),
                                bancho.pw_md5.toUtf8())
                    .toUtf8());

    APIRequest request;
    request.type = GET_MAP_LEADERBOARD;
    request.path = path;
    request.mime = NULL;
    request.extra = (uint8_t *)strdup(beatmap->getMD5Hash().hash);

    send_api_request(request);
}

// Since strtok_r SUCKS I'll just make my own
// Returns the token start, and edits str to after the token end (unless '\0').
char *strtok_x(char d, char **str) {
    char *old = *str;
    while(**str != '\0' && **str != d) {
        (*str)++;
    }
    if(**str != '\0') {
        **str = '\0';
        (*str)++;
    }
    return old;
}

void process_leaderboard_response(Packet response) {
    // NOTE: We're not doing anything with the "info" struct.
    //       Server can return partial responses in some cases, so make sure
    //       you actually received the data if you plan on using it.
    OnlineMapInfo info = {0};
    MD5Hash beatmap_hash = (char *)response.extra;
    std::vector<OsuDatabase::Score> scores;
    char *body = (char *)response.memory;

    char *ranked_status = strtok_x('|', &body);
    info.ranked_status = strtol(ranked_status, NULL, 10);

    char *server_has_osz2 = strtok_x('|', &body);
    info.server_has_osz2 = !strcmp(server_has_osz2, "true");

    char *beatmap_id = strtok_x('|', &body);
    info.beatmap_id = strtoul(beatmap_id, NULL, 10);

    char *beatmap_set_id = strtok_x('|', &body);
    info.beatmap_set_id = strtoul(beatmap_set_id, NULL, 10);

    char *nb_scores = strtok_x('|', &body);
    info.nb_scores = strtoul(nb_scores, NULL, 10);

    char *fa_track_id = strtok_x('|', &body);
    (void)fa_track_id;

    char *fa_license_text = strtok_x('\n', &body);
    (void)fa_license_text;

    // I'm guessing this is online offset, but idk
    char *online_offset = strtok_x('\n', &body);
    (void)online_offset;

    char *map_name = strtok_x('\n', &body);
    (void)map_name;

    char *online_star_rating = strtok_x('\n', &body);
    (void)online_star_rating;

    char *pb_score = strtok_x('\n', &body);
    (void)pb_score;

    char *score_line = NULL;
    while((score_line = strtok_x('\n', &body))[0] != '\0') {
        scores.push_back(parse_score(score_line));
    }

    // XXX: We should also separately display either the "personal best" the server sent us,
    //      or the local best, depending on which score is better.
    debugLog("Received online leaderbord for Beatmap ID %d\n", info.beatmap_id);
    bancho.osu->getSongBrowser()->getDatabase()->m_online_scores[beatmap_hash] = scores;
    bancho.osu->getSongBrowser()->rebuildScoreButtons();
}
