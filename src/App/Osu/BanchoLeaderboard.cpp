#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "Bancho.h"
#include "BanchoLeaderboard.h"
#include "BanchoNetworking.h"
#include "ConVar.h"
#include "Engine.h"
#include "OsuDatabase.h"
#include "OsuSongBrowser2.h"

// TODO @kiwec: some sort of cache

OsuDatabase::Score parse_score(char *score_line) {
  OsuDatabase::Score score = {0};
  score.isLegacyScore = true;
  score.isImportedLegacyScore = true;
  score.speedMultiplier = 1.0;

  char *saveptr = NULL;
  char *str = strtok_r(score_line, "|", &saveptr);
  if (!str)
    return score;
  // Do nothing with score ID

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return score;
  score.playerName = UString(str);

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return score;
  score.score = strtoul(str, NULL, 10);

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return score;
  score.comboMax = strtoul(str, NULL, 10);

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return score;
  score.num50s = strtoul(str, NULL, 10);

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return score;
  score.num100s = strtoul(str, NULL, 10);

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return score;
  score.num300s = strtoul(str, NULL, 10);

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return score;
  score.numMisses = strtoul(str, NULL, 10);

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return score;
  score.numKatus = strtoul(str, NULL, 10);

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return score;
  score.numGekis = strtoul(str, NULL, 10);

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return score;
  score.perfect = strtoul(str, NULL, 10) == 1;

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return score;
  score.modsLegacy = strtoul(str, NULL, 10);

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return score;
  // Do nothing with user ID

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return score;
  // Do nothing with rank

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return score;
  score.unixTimestamp = strtoul(str, NULL, 10);

  // And we do nothing with has_replay

  return score;
}

void fetch_online_scores(OsuDatabaseBeatmap *beatmap) {
  std::string path = "/web/osu-osz2-getscores.php?s=0&vv=4&v=1";

  std::string beatmap_hash = beatmap->getMD5Hash();
  path += "&c=" + beatmap_hash;

  UString ustr_osu_file_path =
      beatmap->getFilePath(); // have to keep UString in scope to use toUtf8()
  const char *osu_file_path = ustr_osu_file_path.toUtf8();
  for (const char *p = osu_file_path; *p; p++) {
    if (*p == '/') {
      osu_file_path = p + 1;
    }
  }
  CURL *curl = curl_easy_init();
  if (!curl) {
    debugLog("Failed to initialize cURL in fetch_online_scores()!\n");
    return;
  }
  char *encoded_filename = curl_easy_escape(curl, osu_file_path, 0);
  if (!encoded_filename) {
    debugLog("Failed to encode map filename!\n");
    curl_easy_cleanup(curl);
    return;
  }
  path += "&f=";
  path += encoded_filename;
  curl_free(encoded_filename);
  curl_easy_cleanup(curl);

  path += "&m=0&i=" + std::to_string(beatmap->getSetID());
  path += "&mods=0&h=&a=0";

  UString cv_username =
      convar->getConVarByName("name")
          ->getString(); // have to keep UString in scope to use toUtf8()
  path += "&us=" + std::string(cv_username.toUtf8());

  UString cv_password =
      convar->getConVarByName("osu_password")
          ->getString(); // have to keep UString in scope to use toUtf8()
  const char *pw = cv_password.toUtf8();
  std::string password_hash = md5((uint8_t *)pw, strlen(pw));
  path += "&ha=" + password_hash;

  APIRequest request = {
      .type = GET_MAP_LEADERBOARD,
      .path = path,
      .extra = (uint8_t *)strdup(beatmap_hash.c_str()),
  };

  send_api_request(request);
}

// Since strtok_r SUCKS I'll just make my own
// Returns the token start, and edits str to after the token end (unless '\0').
char *strtok_x(char d, char **str) {
  char *old = *str;
  while (**str != '\0' && **str != d) {
    (*str)++;
  }
  if (**str != '\0') {
    **str = '\0';
    (*str)++;
  }
  return old;
}

void process_leaderboard_response(Packet response) {
  // TODO @kiwec: strengthen this to handle error cases

  debugLog("process_leaderboard_response: %s\n", response.memory);

  OnlineMapInfo info = {0};

  char *body = (char *)response.memory;

  char *ranked_status = strtok_x('|', &body);
  info.ranked_status = strtoul(ranked_status, NULL, 10);

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

  std::vector<OsuDatabase::Score> scores;
  char *score_line = NULL;
  while ((score_line = strtok_x('\n', &body))[0] != '\0') {
    scores.push_back(parse_score(score_line));
  }

  debugLog("Got response for beatmap %d\n", info.beatmap_id);

  std::string beatmap_hash = (char *)response.extra;
  bancho.osu->getSongBrowser()->getDatabase()->m_online_scores[beatmap_hash] =
      scores;
  bancho.osu->getSongBrowser()->rebuildScoreButtons();
}
