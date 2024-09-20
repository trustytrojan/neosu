#pragma once
#include <curl/curl.h>

#include <string>

#include "BanchoProtocol.h"

#ifdef _DEBUG
#define NEOSU_STREAM "dev"
#else
#define NEOSU_STREAM "release"
#endif

#define NEOSU_DOMAIN "neosu.net"

// NOTE: Full version can be something like "b20200201.2cuttingedge"
#define OSU_VERSION "b20240820.1"
#define OSU_VERSION_DATEONLY 20240820

enum APIRequestType {
    GET_BEATMAPSET_INFO,
    GET_MAP_LEADERBOARD,
    GET_REPLAY,
    MARK_AS_READ,
    SUBMIT_SCORE,
};

struct APIRequest {
    APIRequestType type;
    UString path = "";
    curl_mime *mime = NULL;
    u8 *extra = NULL;
    i32 extra_int = 0;  // lazy
};

struct ReplayExtraInfo {
    MD5Hash diff2_md5;
    i32 mod_flags;
    UString username;
    i32 player_id;
};

void disconnect();
void reconnect();

// Send an API request.
void send_api_request(APIRequest request);

// Send a packet to Bancho. Do not free it after calling this.
void send_packet(Packet &packet);

// Poll for new packets. Should be called regularly from main thread.
void receive_api_responses();
void receive_bancho_packets();

// Initialize networking thread. Should be called once when starting neosu.
void init_networking_thread();

// Stop networking thread. Should be called once when exiting neosu.
void kill_networking_thread();

size_t curl_write(void *contents, size_t size, size_t nmemb, void *userp);

extern UString cho_token;
