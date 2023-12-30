#pragma once
#include "BanchoProtocol.h"
#include <string>

enum APIRequestType {
  GET_MAP_LEADERBOARD,
};

struct APIRequest {
  APIRequestType type;
  std::string path;
  uint8_t *extra;
};

void disconnect();
void reconnect();

// Send an API request.
void send_api_request(APIRequest request);

// Send a packet to Bancho. Do not free it after calling this.
void send_packet(Packet& packet);

// Poll for new packets. Should be called regularly from main thread.
void receive_api_responses();
void receive_bancho_packets();

// Initialize networking thread. Should be called once when starting McOsu.
void init_networking_thread();

// TODO @kiwec: map downloads on a separate thread
// see https://curl.se/libcurl/c/CURLOPT_XFERINFOFUNCTION.html
