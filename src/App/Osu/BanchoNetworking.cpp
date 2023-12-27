#include <time.h>
#include <unistd.h>

#include "Bancho.h"
#include "BanchoLeaderboard.h"
#include "BanchoNetworking.h"
#include "BanchoProtocol.h"
#include "Engine.h"

#ifdef MCENGINE_FEATURE_PTHREADS
#include <curl/curl.h>
#include <pthread.h>

pthread_t networking_thread;

// TODO: hardcoded for now
std::string endpoint = "cho.osudev.local";

// Bancho protocol
pthread_mutex_t outgoing_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t incoming_mutex = PTHREAD_MUTEX_INITIALIZER;
std::vector<Packet> outgoing_queue;
std::vector<Packet> incoming_queue;
std::string auth_header = "";

// osu! private API
pthread_mutex_t api_requests_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t api_responses_mutex = PTHREAD_MUTEX_INITIALIZER;
std::vector<APIRequest> api_request_queue;
std::vector<Packet> api_response_queue;
time_t last_api_call_tms = {0};

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

static void send_api_request(CURL *curl, APIRequest outgoing) {
  Packet response = {0};
  response.id = outgoing.type;
  response.extra = outgoing.extra;
  response.memory = (uint8_t *)malloc(2048);

  std::string query_url = "https://osu." + endpoint + outgoing.path;
  curl_easy_setopt(curl, CURLOPT_URL, query_url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

  last_api_call_tms = time(NULL);
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

    // For all packets except the LOGIN packet, we have a header that includes
    // packet length (which we need to update now).
    uint32_t final_pos = outgoing.pos;
    outgoing.pos = 3;
    write_int(&outgoing, final_pos - 7);
    outgoing.pos = final_pos;
  }

  std::string query_url = "https://c." + endpoint + "/";
  curl_easy_setopt(curl, CURLOPT_URL, query_url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, outgoing.memory);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, outgoing.pos);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "osu!");

  CURLcode res = curl_easy_perform(curl);
  CURLHcode hres;
  if (res != CURLE_OK)
    goto end;

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
        .memory = (uint8_t *)malloc(packet_len),
        .size = packet_len,
        .pos = 0,
    };
    memcpy(incoming.memory, response.memory + response.pos, packet_len);

    pthread_mutex_lock(&incoming_mutex);
    incoming_queue.push_back(incoming);
    pthread_mutex_unlock(&incoming_mutex);

    response.pos += packet_len;
  }

end:
  curl_easy_reset(curl);
  free(response.memory);
  curl_slist_free_all(chunk);
}

static void *do_networking(void *data) {
  (void)data;

  last_api_call_tms = time(NULL);

  CURL *curl = curl_easy_init();
  if (!curl) {
    debugLog("Failed to initialize cURL, online functionality disabled.\n");
    return NULL;
  }

  while (true) {
    if (difftime(time(NULL), last_api_call_tms) > 1.0) {
      pthread_mutex_lock(&api_requests_mutex);
      if (api_request_queue.empty()) {
        pthread_mutex_unlock(&api_requests_mutex);
      } else {
        APIRequest outgoing = api_request_queue.front();
        api_request_queue.erase(api_request_queue.begin());
        pthread_mutex_unlock(&api_requests_mutex);

        send_api_request(curl, outgoing);
      }
    }

    pthread_mutex_lock(&outgoing_mutex);
    if (outgoing_queue.empty()) {
      pthread_mutex_unlock(&outgoing_mutex);
      usleep(1000);
    } else {
      Packet outgoing = outgoing_queue.front();
      outgoing_queue.erase(outgoing_queue.begin());
      pthread_mutex_unlock(&outgoing_mutex);

      send_bancho_packet(curl, outgoing);
      free(outgoing.memory);
    }
  }

  // unreachable
  return NULL;
}
#endif

static void handle_api_response(Packet packet) {
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
    free(incoming.memory);
    free(incoming.extra);
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
    free(incoming.memory);
  }
  pthread_mutex_unlock(&incoming_mutex);
#endif
}

void send_api_request(APIRequest request) {
  if (bancho.user_id == 0) {
    debugLog("Cannot send API request of type %d since we are not logged in.\n", request.type);
    return;
  }

#ifdef MCENGINE_FEATURE_PTHREADS
  pthread_mutex_lock(&api_requests_mutex);
  api_request_queue.push_back(request);
  pthread_mutex_unlock(&api_requests_mutex);
#endif
}

void send_packet(Packet packet) {
#ifdef MCENGINE_FEATURE_PTHREADS
  pthread_mutex_lock(&outgoing_mutex);
  outgoing_queue.push_back(packet);
  pthread_mutex_unlock(&outgoing_mutex);
#else
  free(packet.memory);
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
