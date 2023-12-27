#include <stdlib.h>
#include <string.h>

#include "BanchoProtocol.h"

void read_bytes(Packet *packet, uint8_t *bytes, size_t n) {
  if (packet->pos + n > packet->size) {
    packet->pos = packet->size + 1;
    return;
  }
  memcpy(bytes, packet->memory + packet->pos, n);
  packet->pos += n;
}

uint8_t read_byte(Packet *packet) {
  uint8_t byte = 0;
  read_bytes(packet, &byte, 1);
  return byte;
}

uint16_t read_short(Packet *packet) {
  uint16_t s = 0;
  read_bytes(packet, (uint8_t *)&s, 2);
  return s;
}

uint32_t read_int(Packet *packet) {
  uint32_t i = 0;
  read_bytes(packet, (uint8_t *)&i, 4);
  return i;
}

uint64_t read_int64(Packet *packet) {
  uint64_t i = 0;
  read_bytes(packet, (uint8_t *)&i, 8);
  return i;
}

uint32_t read_uleb128(Packet *packet) {
  uint32_t result = 0;
  uint32_t shift = 0;
  uint8_t byte = 0;

  do {
    byte = read_byte(packet);
    result |= (byte & 0x7f) << shift;
    shift += 7;
  } while (byte & 0x80);

  return result;
}

float read_float(Packet *packet) {
  float f = 0;
  read_bytes(packet, (uint8_t *)&f, 4);
  return f;
}

char *read_string(Packet *packet) {
  uint8_t empty_check = read_byte(packet);
  if (empty_check == 0)
    return strdup("");

  // XXX: Don't return null-terminated string
  uint32_t len = read_uleb128(packet);
  uint8_t *str = (uint8_t *)malloc(len + 1);
  read_bytes(packet, str, len);
  str[len] = '\0';
  return (char *)str;
}

Room *read_room(Packet *packet) {
  Room *room = (Room *)malloc(sizeof(Room));
  room->id = read_short(packet);
  room->in_progress = read_byte(packet);
  room->match_type = read_byte(packet);
  room->mods = read_int(packet);
  room->name = read_string(packet);
  room->password = read_string(packet);
  room->map_name = read_string(packet);
  room->map_id = read_int(packet);
  room->map_md5 = read_string(packet);

  room->nb_players = 0;
  for (int i = 0; i < 16; i++) {
    room->slots[i].status = read_int(packet);
    // open = 1
    // locked = 2
    // not_ready = 4
    // ready = 8
    // no_map = 16
    // playing = 32
    // complete = 64
    // quit = 128
    // slot_has_player = not_ready | ready | no_map | playing | complete
    int slot_has_player = (room->slots[i].status & 0b01111100) != 0;
    if (slot_has_player) {
      room->nb_players++;
    }
  }
  for (int i = 0; i < 16; i++) {
    room->slots[i].team = read_int(packet); // XXX: test this
  }
  for (int i = 0; i < room->nb_players; i++) {
    room->player_ids[i] = read_int(packet);
  }

  room->host_id = read_int(packet);

  // XXX: test these
  room->mode = read_int(packet);
  room->win_condition = read_int(packet);
  room->team_type = read_int(packet);
  room->freemods = read_int(packet);
  if (room->freemods) {
    for (int i = 0; i < 16; i++) {
      room->slot_mods[i] = read_int(packet);
    }
  }

  room->seed = read_int(packet);

  return room;
}

void free_room(Room *room) {
  if (!room)
    return;

  free(room->name);
  free(room->password);
  free(room->map_name);
  free(room->map_md5);
  free(room);
}

void write_bytes(Packet *packet, uint8_t *bytes, size_t n) {
  if (packet->pos + n > packet->size) {
    packet->memory =
        (unsigned char *)realloc(packet->memory, packet->size + n + 128);
    packet->size += n + 128;
    if (!packet->memory)
      return;
  }

  memcpy(packet->memory + packet->pos, bytes, n);
  packet->pos += n;
}

void write_byte(Packet *packet, uint8_t b) { write_bytes(packet, &b, 1); }

void write_short(Packet *packet, uint16_t s) {
  write_bytes(packet, (uint8_t *)&s, 2);
}

void write_int(Packet *packet, uint32_t i) {
  write_bytes(packet, (uint8_t *)&i, 4);
}

void write_int64(Packet *packet, uint64_t i) {
  write_bytes(packet, (uint8_t *)&i, 8);
}

void write_uleb128(Packet *packet, uint32_t num) {
  if (num == 0) {
    uint8_t zero = 0;
    write_byte(packet, zero);
    return;
  }

  while (num != 0) {
    uint8_t next = num & 0x7F;
    num >>= 7;
    if (num != 0) {
      next |= 0x80;
    }
    write_byte(packet, next);
  }
}

void write_float(Packet *packet, float f) {
  write_bytes(packet, (uint8_t *)&f, 4);
}

void write_string(Packet *packet, char *str) {
  if (!str || str[0] == '\0') {
    uint8_t zero = 0;
    write_byte(packet, zero);
    return;
  }

  uint8_t empty_check = 11;
  write_byte(packet, empty_check);

  uint32_t len = strlen(str);
  write_uleb128(packet, len);
  write_bytes(packet, (uint8_t *)str, len);
}

void write_md5_bytes_as_hex(Packet *packet, unsigned char *md5) {
  for (int i = 0; i < 16; i++) {
    write_byte(packet, "0123456789abcdef"[md5[i] >> 4]);
    write_byte(packet, "0123456789abcdef"[md5[i] & 0xf]);
  }
}

void write_header(Packet *packet, uint16_t packet_id) {
  write_short(packet, packet_id);
  write_byte(packet, 0);
  write_int(packet, 0);
}
