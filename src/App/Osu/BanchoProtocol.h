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

typedef struct {
  uint8_t *memory;
  size_t size;
  size_t pos;
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
void write_header(Packet *packet, uint16_t packet_id);
