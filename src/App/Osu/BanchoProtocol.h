#pragma once
#include <stddef.h>
#include <stdint.h>

enum Action {
  Idle = 0,
  Afk = 1,
  Playing = 2,
  Editing = 3,
  Modding = 4,
  Multiplayer = 5,
  Watching = 6,
  Unknown = 7,
  Testing = 8,
  Submitting = 9,
  Paused = 10,
  Lobby = 11,
  Multiplaying = 12,
  OsuDirect = 13,
};

enum IncomingPackets {
  USER_ID = 5,
  RECV_MESSAGE = 7,
  PONG = 8,
  USER_STATS = 11,
  USER_LOGOUT = 12,
  SPECTATOR_JOINED = 13,
  SPECTATOR_LEFT = 14,
  VERSION_UPDATE = 19,
  SPECTATOR_CANT_SPECTATE = 22,
  GET_ATTENTION = 23,
  NOTIFICATION = 24,
  ROOM_UPDATED = 26,
  ROOM_CREATED = 27,
  ROOM_CLOSED = 28,
  ROOM_JOIN_SUCCESS = 36,
  ROOM_JOIN_FAIL = 37,
  FELLOW_SPECTATOR_JOINED = 42,
  FELLOW_SPECTATOR_LEFT = 43,
  IN_MATCH_START = 46,
  IN_MATCH_SCORE_UPDATE = 48,
  IN_MATCH_TRANSFER_HOST = 50,
  MATCH_ALL_PLAYERS_LOADED = 53,
  MATCH_PLAYER_FAILED = 57,
  IN_MATCH_COMPLETE = 58,
  MATCH_SKIP = 61,
  CHANNEL_JOIN_SUCCESS = 64,
  CHANNEL_INFO = 65,
  CHANNEL_KICK = 66,
  CHANNEL_AUTO_JOIN = 67,
  PRIVILEGES = 71,
  FRIENDS_LIST = 72,
  PROTOCOL_VERSION = 75,
  MAIN_MENU_ICON = 76,
  MATCH_PLAYER_SKIPPED = 81,
  USER_PRESENCE = 83,
  RESTART = 86,
  ROOM_INVITE = 88,
  CHANNEL_INFO_END = 89,
  IN_MATCH_CHANGE_PASSWORD = 91,
  SILENCE_END = 92,
  USER_SILENCED = 94,
  USER_DM_BLOCKED = 100,
  TARGET_IS_SILENCED = 101,
  VERSION_UPDATE_FORCED = 102,
  SWITCH_SERVER = 103,
  ACCOUNT_RESTRICTED = 104,
  MATCH_ABORT = 106,
};

enum OutgoingPackets {
  SEND_PUBLIC_MESSAGE = 1,
  PING = 4,
  START_SPECTATING = 16,
  STOP_SPECTATING = 17,
  SPECTATE_FRAMES = 18,
  ERROR_REPORT = 20,
  CANT_SPECTATE = 21,
  SEND_PRIVATE_MESSAGE = 25,
  EXIT_ROOM_LIST = 29,
  JOIN_ROOM_LIST = 30,
  CREATE_ROOM = 31,
  JOIN_ROOM = 32,
  EXIT_ROOM = 33,
  CHANGE_SLOT = 38,
  MATCH_READY = 39,
  MATCH_LOCK = 40,
  MATCH_CHANGE_SETTINGS = 41,
  OUT_MATCH_START = 44,
  OUT_MATCH_SCORE_UPDATE = 47,
  OUT_MATCH_COMPLETE = 49,
  MATCH_CHANGE_MODS = 51,
  MATCH_LOAD_COMPLETE = 52,
  MATCH_NO_BEATMAP = 54,
  MATCH_NOT_READY = 55,
  MATCH_FAILED = 56,
  MATCH_HAS_BEATMAP = 59,
  MATCH_SKIP_REQUEST = 60,
  CHANNEL_JOIN = 63,
  BEATMAP_INFO_REQUEST = 68,
  OUT_MATCH_TRANSFER_HOST = 70,
  FRIEND_ADD = 73,
  FRIEND_REMOVE = 74,
  MATCH_CHANGE_TEAM = 77,
  CHANNEL_PART = 78,
  RECEIVE_UPDATES = 79,
  SET_AWAY_MESSAGE = 82,
  IRC_ONLY = 84,
  USER_STATS_REQUEST = 85,
  MATCH_INVITE = 87,
  OUT_MATCH_CHANGE_PASSWORD = 90,
  TOURNAMENT_MATCH_INFO_REQUEST = 93,
  USER_PRESENCE_REQUEST = 97,
  USER_PRESENCE_REQUEST_ALL = 98,
  TOGGLE_BLOCK_NON_FRIEND_DMS = 99,
  TOURNAMENT_JOIN_MATCH_CHANNEL = 108,
  TOURNAMENT_EXIT_MATCH_CHANNEL = 109,
};

typedef struct {
  uint16_t id;
  uint8_t *memory;
  size_t size;
  size_t pos;
  uint8_t *extra;
} Packet;

typedef struct {
  uint32_t status;
  uint32_t team;
} Slot;

typedef struct {
  uint16_t id;
  uint8_t in_progress;
  uint8_t match_type;
  uint32_t mods;
  uint32_t seed;

  char *name;
  char *password;

  char *map_name;
  char *map_md5;
  int32_t map_id;

  uint32_t mode;
  uint32_t win_condition;
  uint32_t team_type;
  uint32_t freemods;

  int32_t host_id;
  uint8_t nb_players;
  Slot slots[16];
  uint32_t slot_mods[16];

  // NOTE: player indexes don't match slot indexes
  uint32_t player_ids[16];
} Room;

void read_bytes(Packet *packet, uint8_t *bytes, size_t n);
uint8_t read_byte(Packet *packet);
uint16_t read_short(Packet *packet);
uint32_t read_int(Packet *packet);
uint64_t read_int64(Packet *packet);
uint32_t read_uleb128(Packet *packet);
float read_float(Packet *packet);
char *read_string(Packet *packet);

Room *read_room(Packet *packet);
void free_room(Room *room);

void write_bytes(Packet *packet, uint8_t *bytes, size_t n);
void write_byte(Packet *packet, uint8_t b);
void write_short(Packet *packet, uint16_t s);
void write_int(Packet *packet, uint32_t i);
void write_int64(Packet *packet, uint64_t i);
void write_uleb128(Packet *packet, uint32_t num);
void write_float(Packet *packet, float f);
void write_string(Packet *packet, char *str);
void write_md5_bytes_as_hex(Packet *packet, unsigned char *md5);
