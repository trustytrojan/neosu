#include <time.h>
#include <unistd.h>
#include "miniz.h"

#include "Bancho.h"
#include "BanchoLeaderboard.h"
#include "BanchoNetworking.h"
#include "BanchoProtocol.h"
#include "ConVar.h"
#include "Engine.h"

#ifdef MCENGINE_FEATURE_PTHREADS
#include <curl/curl.h>
#include <pthread.h>
#include "File.h"

pthread_t networking_thread;

// TODO @kiwec: improve the way the client logs in and manages logged in state
// Right now if log in fails or expires, things explode

// Bancho protocol
pthread_mutex_t outgoing_mutex = PTHREAD_MUTEX_INITIALIZER;
bool try_logging_in = false;
Packet login_packet = {0};
Packet outgoing = {0};
pthread_mutex_t incoming_mutex = PTHREAD_MUTEX_INITIALIZER;
std::vector<Packet> incoming_queue;
time_t last_packet_tms = {0};
double seconds_between_pings = 1.0;

std::string auth_header = "";

// osu! private API
pthread_mutex_t api_requests_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t api_responses_mutex = PTHREAD_MUTEX_INITIALIZER;
std::vector<APIRequest> api_request_queue;
std::vector<Packet> api_response_queue;

void disconnect() {
  // TODO @kiwec: send a logout packet first?
  // TODO @kiwec: don't set auth_header here, enqueue a disconnect packet
  // instead?

  pthread_mutex_lock(&outgoing_mutex);
  try_logging_in = false;
  auth_header = "";
  delete outgoing.memory;
  outgoing = {0};
  pthread_mutex_unlock(&outgoing_mutex);

  bancho.user_id = 0;
}

void reconnect() {
  disconnect();

  UString user =
      convar->getConVarByName("name")
          ->getString(); // have to keep UString in scope to use toUtf8()
  UString pw =
      convar->getConVarByName("osu_password")
          ->getString(); // have to keep UString in scope to use toUtf8()

  // No password: don't try to log in
  if (pw.length() == 0)
    return;

  bancho.username = user;
  Packet new_login_packet = build_login_packet((char *)user.toUtf8(), (char *)pw.toUtf8());

  pthread_mutex_lock(&outgoing_mutex);
  delete login_packet.memory;
  login_packet = new_login_packet;
  try_logging_in = true;
  pthread_mutex_unlock(&outgoing_mutex);
}

static size_t curl_write(void *contents, size_t size, size_t nmemb,
                         void *userp) {
  size_t realsize = size * nmemb;
  Packet *mem = (Packet *)userp;

  uint8_t *ptr = (uint8_t *)realloc(mem->memory, mem->size + realsize + 1);
  if (!ptr) {
    /* out of memory! */
    debugLog("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

static void send_api_request(CURL *curl, APIRequest api_out) {
  Packet response = {0};
  response.id = api_out.type;
  response.extra = api_out.extra;
  response.memory = new uint8_t[2048];

  // TODO @kiwec: convar not thread safe
  UString cv_endpoint =
      convar->getConVarByName("osu_server")
          ->getString(); // have to keep UString in scope to use toUtf8()
  std::string query_url =
      "https://osu." + std::string(cv_endpoint.toUtf8()) + api_out.path;
  debugLog("Sending request: %s\n", query_url.c_str());

  curl_easy_setopt(curl, CURLOPT_URL, query_url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

  CURLcode res = curl_easy_perform(curl);
  if (res == CURLE_OK) {
    pthread_mutex_lock(&api_responses_mutex);
    api_response_queue.push_back(response);
    pthread_mutex_unlock(&api_responses_mutex);
  }

  curl_easy_reset(curl);
}

static void send_bancho_packet(CURL *curl, Packet outgoing) {
  Packet response = {0};
  response.memory = (uint8_t *)malloc(2048);

  struct curl_slist *chunk = NULL;
  if (!auth_header.empty()) {
    chunk = curl_slist_append(chunk, auth_header.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
  }

  // TODO @kiwec: convar not thread safe
  UString cv_endpoint =
      convar->getConVarByName("osu_server")
          ->getString(); // have to keep UString in scope to use toUtf8()
  std::string query_url =
      "https://c." + std::string(cv_endpoint.toUtf8()) + "/";
  curl_easy_setopt(curl, CURLOPT_URL, query_url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, outgoing.memory);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, outgoing.pos);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "osu!");

  last_packet_tms = time(NULL);
  CURLcode res = curl_easy_perform(curl);
  CURLHcode hres;
  if (res != CURLE_OK) {
    // TODO @kiwec: if log in packet, display error to user
    debugLog("Failed to send packet, cURL error %d\n%s\n", res,
             query_url.c_str());
    goto end;
  }

  // Update auth token if applicable
  struct curl_header *header;
  hres = curl_easy_header(curl, "cho-token", 0, CURLH_HEADER, -1, &header);
  if (hres == CURLHE_OK) {
    auth_header = "osu-token: " + std::string(header->value);
  }

  while (response.pos < response.size) {
    uint16_t packet_id = read_short(&response);
    response.pos++;
    uint32_t packet_len = read_int(&response);
    if (packet_len > 10485760) {
      debugLog("Received a packet over 10Mb! Dropping response.\n");
      goto end;
    }

    Packet incoming = {
        .id = packet_id,
        .memory = new uint8_t[packet_len],
        .size = packet_len,
        .pos = 0,
    };
    memcpy(incoming.memory, response.memory + response.pos, packet_len);

    pthread_mutex_lock(&incoming_mutex);
    incoming_queue.push_back(incoming);
    pthread_mutex_unlock(&incoming_mutex);

    seconds_between_pings = 1.0;
    response.pos += packet_len;
  }

end:
  curl_easy_reset(curl);
  delete response.memory;
  curl_slist_free_all(chunk);
}

static void *do_networking(void *data) {
  (void)data;

  last_packet_tms = time(NULL);

  curl_global_init(CURL_GLOBAL_ALL);
  CURL *curl = curl_easy_init();
  if (!curl) {
    debugLog("Failed to initialize cURL, online functionality disabled.\n");
    return NULL;
  }

  // download_beatmap(curl, 2050081);
  download_beatmap(curl, 18980); // it has subfolder, i wanna test

  while (true) {
    pthread_mutex_lock(&api_requests_mutex);
    if (api_request_queue.empty()) {
      pthread_mutex_unlock(&api_requests_mutex);
    } else {
      APIRequest api_out = api_request_queue.front();
      api_request_queue.erase(api_request_queue.begin());
      pthread_mutex_unlock(&api_requests_mutex);

      send_api_request(curl, api_out);
    }

    bool should_ping = difftime(time(NULL), last_packet_tms) > seconds_between_pings;
    // TODO @kiwec: if(is_in_lobby_list || is_multiplaying) should_ping = true;
    // needs proper mutex handling

    pthread_mutex_lock(&outgoing_mutex);
    if (try_logging_in) {
      Packet login = login_packet;
      login_packet = {0};
      try_logging_in = false;
      pthread_mutex_unlock(&outgoing_mutex);
      send_bancho_packet(curl, login);
      delete login.memory;
      pthread_mutex_lock(&outgoing_mutex);
    } else if(should_ping && outgoing.pos == 0) {
      write_short(&outgoing, PING);
      write_byte(&outgoing, 0);
      write_int(&outgoing, 0);

      // Polling gets slower over time, but resets when we receive new data
      if (seconds_between_pings < 30.0) {
        seconds_between_pings += 1.0;
      }
    }

    if(outgoing.pos > 0) {
      Packet out = outgoing;
      outgoing = {0};
      pthread_mutex_unlock(&outgoing_mutex);

      // DEBUG: If we're not sending the right amount of bytes, bancho.py just
      // chugs along! To try to detect it faster, we'll send two packets per request.
      write_short(&out, PING);
      write_byte(&out, 0);
      write_int(&out, 0);

      send_bancho_packet(curl, out);
      delete out.memory;
    } else {
      pthread_mutex_unlock(&outgoing_mutex);
    }

    // TODO @kiwec: this sucks. just because the ppy client does it that way,
    //   doesn't mean we also should... at least, we should send requests instantly
    //   if we haven't in the last second.

    // osu! doesn't need fast networking. In fact, we want to avoid spamming the
    // servers... So we batch requests and send them once a second at most.
    sleep(1);
  }

  // unreachable
  return NULL;
}
#endif

static void handle_api_response(Packet packet) {
  debugLog("Received API response of type %d\n", packet.id);

  if (packet.id == GET_MAP_LEADERBOARD) {
    process_leaderboard_response(packet);
  } else {
    // NOTE: API Response type is same as API Request type
    debugLog("No handler for API response type %d!\n", packet.id);
  }
}

void receive_api_responses() {
#ifdef MCENGINE_FEATURE_PTHREADS
  pthread_mutex_lock(&api_responses_mutex);
  while (!api_response_queue.empty()) {
    Packet incoming = api_response_queue.front();
    api_response_queue.erase(api_response_queue.begin());
    handle_api_response(incoming);
    delete incoming.memory;
    delete incoming.extra;
  }
  pthread_mutex_unlock(&api_responses_mutex);
#endif
}

void receive_bancho_packets() {
#ifdef MCENGINE_FEATURE_PTHREADS
  pthread_mutex_lock(&incoming_mutex);
  while (!incoming_queue.empty()) {
    Packet incoming = incoming_queue.front();
    incoming_queue.erase(incoming_queue.begin());
    handle_packet(&incoming);
    delete incoming.memory;
  }
  pthread_mutex_unlock(&incoming_mutex);
#endif
}

void send_api_request(APIRequest request) {
  if (bancho.user_id <= 0) {
    debugLog("Cannot send API request of type %d since we are not logged in.\n",
             request.type);
    return;
  }

#ifdef MCENGINE_FEATURE_PTHREADS
  pthread_mutex_lock(&api_requests_mutex);
  api_request_queue.push_back(request);
  pthread_mutex_unlock(&api_requests_mutex);
#endif
}

void send_packet(Packet& packet) {
#ifdef MCENGINE_FEATURE_PTHREADS
  if(bancho.user_id <= 0) {
    // Don't queue any packets until we're logged in
    return;
  }

  // debugLog("Sending packet of type %hu: ", packet.id);
  // for (int i = 0; i < packet.pos; i++) {
  //     debugLog("%02x ", packet.memory[i]);
  // }
  // debugLog("\n");

  pthread_mutex_lock(&outgoing_mutex);

  // We're not sending it immediately, instead we just add it to the pile of
  // packets to send
  write_short(&outgoing, packet.id);
  write_byte(&outgoing, 0);
  write_int(&outgoing, packet.pos);
  write_bytes(&outgoing, packet.memory, packet.pos);

  pthread_mutex_unlock(&outgoing_mutex);
#else
  delete packet.memory;
#endif
}

void init_networking_thread() {
#ifdef MCENGINE_FEATURE_PTHREADS
  int ret = pthread_create(&networking_thread, NULL, do_networking, NULL);
  if (ret) {
    debugLog("Failed to start networking thread: pthread_create() returned "
             "code %i\n",
             ret);
  }
#endif
}

void download_beatmap(CURL *curl, uint32_t set_id) {
  // TODO @kiwec: make multithreaded (lol)
  // TODO @kiwec: Support multiple beatmap mirrors, pick randomly, retry with another if it fails

  if(!env->directoryExists(MCENGINE_DATA_DIR "maps")) {
    env->createDirectory(MCENGINE_DATA_DIR "maps");
  }

  auto extract_to = UString::format(MCENGINE_DATA_DIR "maps/%d", set_id);
  if(env->directoryExists(extract_to)) {
    // TODO @kiwec: return immediately, we already downloaded the map!
    debugLog("%s already exists\n", extract_to.toUtf8());
  } else {
    env->createDirectory(extract_to);
    debugLog("Creating %s\n", extract_to.toUtf8());
  }

  Packet response = {0};
  response.memory = new uint8_t[2048];

  std::string query_url = "https://api.osu.direct/d/" + std::to_string(set_id);
  debugLog("Downloading beatmap %s\n", query_url.c_str());
  curl_easy_setopt(curl, CURLOPT_URL, query_url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERAGENT, MCOSU_USER_AGENT);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

  // TODO @kiwec: get progress information
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

  CURLcode res = curl_easy_perform(curl);
  if(res != CURLE_OK) {
    // TODO @kiwec: handle error
    debugLog("Failed to download beatmap: %s\n", curl_easy_strerror(res));
    return;
  }

  curl_easy_reset(curl);

  debugLog("Unzipping beatmapset %d (%d bytes)\n", set_id, response.size);
  mz_zip_archive zip = {0};
  if (!mz_zip_reader_init_mem(&zip, response.memory, response.size, 0)) {
    // TODO @kiwec: handle error
    debugLog("Failed to open .osz file\n");
    return;
  }

  mz_uint num_files = mz_zip_reader_get_num_files(&zip);
  if (num_files <= 0) {
    // TODO @kiwec: handle error
    debugLog("0 files in zip\n");
    return;
  }
  mz_zip_archive_file_stat file_stat;
  for(mz_uint i = 0; i < num_files; i++) {
    if(!mz_zip_reader_file_stat(&zip, i, &file_stat)) continue;
    if(!mz_zip_reader_is_file_a_directory(&zip, i)) continue;
    UString new_dir = UString::format("%s/%s", extract_to.toUtf8(), file_stat.m_filename);
    debugLog("Creating dir %s\n", file_stat.m_filename);
    env->createDirectory(new_dir);
  }
  for(mz_uint i = 0; i < num_files; i++) {
    if(!mz_zip_reader_file_stat(&zip, i, &file_stat)) continue;
    if(mz_zip_reader_is_file_a_directory(&zip, i)) continue;

    char* saveptr = NULL;
    char* folder = strtok_r(file_stat.m_filename, "/", &saveptr);
    UString file_path = extract_to;
    while(folder != NULL) {
      if(!strcmp(folder, "..")) {
        // Bro...
        goto skip_file;
      }

      file_path.append("/");
      file_path.append(folder);
      folder = strtok_r(NULL, "/", &saveptr);

      if(folder != NULL) {
        if(!env->directoryExists(file_path)) {
          env->createDirectory(file_path);
        }
      }
    }

    debugLog("Extracting %s\n", file_path.toUtf8());
    if(!mz_zip_reader_extract_to_file(&zip, i, file_path.toUtf8(), 0)) {
      // TODO @kiwec: handle error (or ignore, and handle error later)
      debugLog("Failed to extract %s\n", file_path.toUtf8());
    }

skip_file:;
  }

  mz_zip_reader_end(&zip);
  delete response.memory;
}
