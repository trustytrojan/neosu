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
  OsuDatabase::Score score;

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

  const char *osu_file_path = beatmap->getFilePath().toUtf8();
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
  path += "&f=" + std::string(encoded_filename);
  curl_free(encoded_filename);
  curl_easy_cleanup(curl);

  path += "&m=0&i=" + std::to_string(beatmap->getSetID());
  path += "&mods=0&h=&a=0";

  auto cv_username = convar->getConVarByName("osu_username");
  path += "&us=" + std::string(cv_username->getString().toUtf8());

  auto cv_password = convar->getConVarByName("osu_password");
  const char *pw = cv_password->getString().toUtf8();
  std::string password_hash = md5((uint8_t *)pw, strlen(pw));
  path += "&ha=" + password_hash;

  APIRequest request = {
      .type = GET_MAP_LEADERBOARD,
      .path = path,
      .extra = (uint8_t*)strdup(beatmap_hash.c_str()),
  };

  send_api_request(request);
}

void process_leaderboard_response(Packet response) {
  // TODO @kiwec: strengthen this to handle error cases

  OnlineMapInfo info = {0};

  char *saveptr = NULL;
  char *str = strtok_r((char *)response.memory, "|", &saveptr);
  if (!str)
    return;
  info.ranked_status = strtoul(str, NULL, 10);

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return;
  if (!strcmp(str, "true"))
    info.server_has_osz2 = true;
  else if (strcmp(str, "false") != 0)
    return;

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return;
  info.beatmap_id = strtoul(str, NULL, 10);

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return;
  info.beatmap_set_id = strtoul(str, NULL, 10);

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return;
  info.nb_scores = strtoul(str, NULL, 10);

  str = strtok_r(NULL, "|", &saveptr);
  if (!str)
    return;
  // Do nothing with fa_track_id

  str = strtok_r(NULL, "\n", &saveptr);
  if (!str)
    return;
  // Do nothing with fa_license_text

  str = strtok_r(NULL, "\n", &saveptr);
  if (!str)
    return;
  // Ignore line

  str = strtok_r(NULL, "\n", &saveptr);
  if (!str)
    return;
  // Do nothing with map name

  str = strtok_r(NULL, "\n", &saveptr);
  if (!str)
    return;
  // Do nothing with star rating

  str = strtok_r(NULL, "\n", &saveptr);
  if (!str)
    return;
  // Do nothing with PB score

  std::vector<OsuDatabase::Score> scores;
  while ((str = strtok_r(NULL, "\n", &saveptr))) {
    scores.push_back(parse_score(str));
  }

  debugLog("Got response for beatmap %d\n", info.beatmap_id);

  std::string beatmap_hash = (char*)response.extra;
  bancho.osu->getSongBrowser()->getDatabase()->m_online_scores[beatmap_hash] = scores;
  bancho.osu->getSongBrowser()->rebuildScoreButtons();
}
