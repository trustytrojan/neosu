#include "BanchoProtocol.h"

#include <stdlib.h>
#include <string.h>

#include "Bancho.h"

Room::Room() {
    // 0-initialized room means we're not in multiplayer at the moment
}

Room::Room(Packet *packet) {
    id = read_u16(packet);
    in_progress = read_u8(packet);
    match_type = read_u8(packet);
    mods = read_u32(packet);
    name = read_string(packet);

    has_password = read_u8(packet) > 0;
    if(has_password) {
        // Discard password. It should be an empty string, but just in case, read it properly.
        packet->pos--;
        read_string(packet);
    }

    map_name = read_string(packet);
    map_id = read_u32(packet);

    auto hash_str = read_string(packet);
    map_md5 = hash_str.toUtf8();

    nb_players = 0;
    for(int i = 0; i < 16; i++) {
        slots[i].status = read_u8(packet);
    }
    for(int i = 0; i < 16; i++) {
        slots[i].team = read_u8(packet);
    }
    for(int s = 0; s < 16; s++) {
        if(!slots[s].is_locked()) {
            nb_open_slots++;
        }

        if(slots[s].has_player()) {
            slots[s].player_id = read_u32(packet);
            nb_players++;
        }
    }

    host_id = read_u32(packet);
    mode = read_u8(packet);
    win_condition = read_u8(packet);
    team_type = read_u8(packet);
    freemods = read_u8(packet);
    if(freemods) {
        for(int i = 0; i < 16; i++) {
            slots[i].mods = read_u32(packet);
        }
    }

    seed = read_u32(packet);
}

void Room::pack(Packet *packet) {
    write_u16(packet, id);
    write_u8(packet, in_progress);
    write_u8(packet, match_type);
    write_u32(packet, mods);
    write_string(packet, name.toUtf8());
    write_string(packet, password.toUtf8());
    write_string(packet, map_name.toUtf8());
    write_u32(packet, map_id);
    write_string(packet, map_md5.toUtf8());
    for(int i = 0; i < 16; i++) {
        write_u8(packet, slots[i].status);
    }
    for(int i = 0; i < 16; i++) {
        write_u8(packet, slots[i].team);
    }
    for(int s = 0; s < 16; s++) {
        if(slots[s].has_player()) {
            write_u32(packet, slots[s].player_id);
        }
    }

    write_u32(packet, host_id);
    write_u8(packet, mode);
    write_u8(packet, win_condition);
    write_u8(packet, team_type);
    write_u8(packet, freemods);
    if(freemods) {
        for(int i = 0; i < 16; i++) {
            write_u32(packet, slots[i].mods);
        }
    }

    write_u32(packet, seed);
}

bool Room::is_host() { return host_id == bancho.user_id; }

void read_bytes(Packet *packet, u8 *bytes, size_t n) {
    if(packet->pos + n > packet->size) {
        packet->pos = packet->size + 1;
        return;
    }
    memcpy(bytes, packet->memory + packet->pos, n);
    packet->pos += n;
}

u8 read_u8(Packet *packet) {
    u8 byte = 0;
    read_bytes(packet, &byte, 1);
    return byte;
}

u16 read_u16(Packet *packet) {
    u16 s = 0;
    read_bytes(packet, (u8 *)&s, 2);
    return s;
}

u32 read_u32(Packet *packet) {
    u32 i = 0;
    read_bytes(packet, (u8 *)&i, 4);
    return i;
}

u64 read_u64(Packet *packet) {
    u64 i = 0;
    read_bytes(packet, (u8 *)&i, 8);
    return i;
}

u32 read_uleb128(Packet *packet) {
    u32 result = 0;
    u32 shift = 0;
    u8 byte = 0;

    do {
        byte = read_u8(packet);
        result |= (byte & 0x7f) << shift;
        shift += 7;
    } while(byte & 0x80);

    return result;
}

float read_f32(Packet *packet) {
    float f = 0;
    read_bytes(packet, (u8 *)&f, 4);
    return f;
}

double read_f64(Packet *packet) {
    double f = 0;
    read_bytes(packet, (u8 *)&f, 8);
    return f;
}

UString read_string(Packet *packet) {
    u8 empty_check = read_u8(packet);
    if(empty_check == 0) return UString("");

    u32 len = read_uleb128(packet);
    u8 *str = new u8[len + 1];
    read_bytes(packet, str, len);
    str[len] = '\0';

    auto ustr = UString((const char *)str);
    delete[] str;

    return ustr;
}

std::string read_stdstring(Packet *packet) {
    u8 empty_check = read_u8(packet);
    if(empty_check == 0) return std::string();

    u32 len = read_uleb128(packet);
    u8 *str = new u8[len + 1];
    read_bytes(packet, str, len);

    std::string str_out((const char *)str, len);
    delete[] str;

    return str_out;
}

MD5Hash read_hash(Packet *packet) {
    MD5Hash hash;

    u8 empty_check = read_u8(packet);
    if(empty_check == 0) return hash;

    u32 len = read_uleb128(packet);
    if(len > 32) {
        len = 32;
    }

    read_bytes(packet, (u8 *)hash.hash, len);
    hash.hash[len] = '\0';
    return hash;
}

void skip_string(Packet *packet) {
    u8 empty_check = read_u8(packet);
    if(empty_check == 0) {
        return;
    }

    u32 len = read_uleb128(packet);
    packet->pos += len;
}

void write_bytes(Packet *packet, u8 *bytes, size_t n) {
    if(packet->pos + n > packet->size) {
        packet->memory = (unsigned char *)realloc(packet->memory, packet->size + n + 128);
        packet->size += n + 128;
        if(!packet->memory) return;
    }

    memcpy(packet->memory + packet->pos, bytes, n);
    packet->pos += n;
}

void write_u8(Packet *packet, u8 b) { write_bytes(packet, &b, 1); }

void write_u16(Packet *packet, u16 s) { write_bytes(packet, (u8 *)&s, 2); }

void write_u32(Packet *packet, u32 i) { write_bytes(packet, (u8 *)&i, 4); }

void write_u64(Packet *packet, u64 i) { write_bytes(packet, (u8 *)&i, 8); }

void write_uleb128(Packet *packet, u32 num) {
    if(num == 0) {
        u8 zero = 0;
        write_u8(packet, zero);
        return;
    }

    while(num != 0) {
        u8 next = num & 0x7F;
        num >>= 7;
        if(num != 0) {
            next |= 0x80;
        }
        write_u8(packet, next);
    }
}

void write_f32(Packet *packet, float f) { write_bytes(packet, (u8 *)&f, 4); }

void write_f64(Packet *packet, double f) { write_bytes(packet, (u8 *)&f, 8); }

void write_string(Packet *packet, const char *str) {
    if(!str || str[0] == '\0') {
        u8 zero = 0;
        write_u8(packet, zero);
        return;
    }

    u8 empty_check = 11;
    write_u8(packet, empty_check);

    u32 len = strlen(str);
    write_uleb128(packet, len);
    write_bytes(packet, (u8 *)str, len);
}
