#include "BanchoProtocol.h"

#include <stdlib.h>
#include <string.h>

#include "Bancho.h"
#include "Beatmap.h"
#include "Osu.h"

namespace BANCHO::Proto {
void read_bytes(Packet *packet, u8 *bytes, size_t n) {
    if(packet->pos + n > packet->size) {
        packet->pos = packet->size + 1;
    } else {
        memcpy(bytes, packet->memory + packet->pos, n);
        packet->pos += n;
    }
}

u32 read_uleb128(Packet *packet) {
    u32 result = 0;
    u32 shift = 0;
    u8 byte = 0;

    do {
        byte = read<u8>(packet);
        result |= (byte & 0x7f) << shift;
        shift += 7;
    } while(byte & 0x80);

    return result;
}

UString read_string(Packet *packet) {
    u8 empty_check = read<u8>(packet);
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
    u8 empty_check = read<u8>(packet);
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

    u8 empty_check = read<u8>(packet);
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
    u8 empty_check = read<u8>(packet);
    if(empty_check == 0) {
        return;
    }

    u32 len = read_uleb128(packet);
    packet->pos += len;
}

void write_bytes(Packet *packet, u8 *bytes, size_t n) {
    if(packet->pos + n > packet->size) {
        packet->memory = (unsigned char *)realloc(packet->memory, packet->size + n + 4096);
        packet->size += n + 4096;
        if(!packet->memory) return;
    }

    memcpy(packet->memory + packet->pos, bytes, n);
    packet->pos += n;
}

void write_uleb128(Packet *packet, u32 num) {
    if(num == 0) {
        u8 zero = 0;
        write<u8>(packet, zero);
        return;
    }

    while(num != 0) {
        u8 next = num & 0x7F;
        num >>= 7;
        if(num != 0) {
            next |= 0x80;
        }
        write<u8>(packet, next);
    }
}

void write_string(Packet *packet, const char *str) {
    if(!str || str[0] == '\0') {
        u8 zero = 0;
        write<u8>(packet, zero);
        return;
    }

    u8 empty_check = 11;
    write<u8>(packet, empty_check);

    u32 len = strlen(str);
    write_uleb128(packet, len);
    write_bytes(packet, (u8 *)str, len);
}

void write_hash(Packet *packet, MD5Hash hash) {
    write<u8>(packet, 0x0B);
    write<u8>(packet, 0x20);
    write_bytes(packet, (u8 *)hash.hash, 32);
}
}  // namespace BANCHO::Proto

using namespace BANCHO::Proto;

Room::Room() {
    // 0-initialized room means we're not in multiplayer at the moment
}

Room::Room(Packet *packet) {
    this->id = read<u16>(packet);
    this->in_progress = read<u8>(packet);
    this->match_type = read<u8>(packet);
    this->mods = read<u32>(packet);
    this->name = read_string(packet);

    this->has_password = read<u8>(packet) > 0;
    if(this->has_password) {
        // Discard password. It should be an empty string, but just in case, read it properly.
        packet->pos--;
        read_string(packet);
    }

    this->map_name = read_string(packet);
    this->map_id = read<u32>(packet);

    auto hash_str = read_string(packet);
    this->map_md5 = hash_str.toUtf8();

    this->nb_players = 0;
    for(int i = 0; i < 16; i++) {
        this->slots[i].status = read<u8>(packet);
    }
    for(int i = 0; i < 16; i++) {
        this->slots[i].team = read<u8>(packet);
    }
    for(int s = 0; s < 16; s++) {
        if(!this->slots[s].is_locked()) {
            this->nb_open_slots++;
        }

        if(this->slots[s].has_player()) {
            this->slots[s].player_id = read<u32>(packet);
            this->nb_players++;
        }
    }

    this->host_id = read<u32>(packet);
    this->mode = read<u8>(packet);
    this->win_condition = read<u8>(packet);
    this->team_type = read<u8>(packet);
    this->freemods = read<u8>(packet);
    if(this->freemods) {
        for(int i = 0; i < 16; i++) {
            this->slots[i].mods = read<u32>(packet);
        }
    }

    this->seed = read<u32>(packet);
}

void Room::pack(Packet *packet) {
    write<u16>(packet, this->id);
    write<u8>(packet, this->in_progress);
    write<u8>(packet, this->match_type);
    write<u32>(packet, this->mods);
    write_string(packet, this->name.toUtf8());
    write_string(packet, this->password.toUtf8());
    write_string(packet, this->map_name.toUtf8());
    write<u32>(packet, this->map_id);
    write_string(packet, this->map_md5.toUtf8());
    for(int i = 0; i < 16; i++) {
        write<u8>(packet, this->slots[i].status);
    }
    for(int i = 0; i < 16; i++) {
        write<u8>(packet, this->slots[i].team);
    }
    for(int s = 0; s < 16; s++) {
        if(this->slots[s].has_player()) {
            write<u32>(packet, this->slots[s].player_id);
        }
    }

    write<u32>(packet, this->host_id);
    write<u8>(packet, this->mode);
    write<u8>(packet, this->win_condition);
    write<u8>(packet, this->team_type);
    write<u8>(packet, this->freemods);
    if(this->freemods) {
        for(int i = 0; i < 16; i++) {
            write<u32>(packet, this->slots[i].mods);
        }
    }

    write<u32>(packet, this->seed);
}

bool Room::is_host() { return this->host_id == bancho->user_id; }

ScoreFrame ScoreFrame::get() {
    u8 slot_id = 0;
    for(u8 i = 0; i < 16; i++) {
        if(bancho->room.slots[i].player_id == bancho->user_id) {
            slot_id = i;
            break;
        }
    }

    auto score = osu->getScore();
    auto perfect = (score->getNumSliderBreaks() == 0 && score->getNumMisses() == 0 && score->getNum50s() == 0 &&
                    score->getNum100s() == 0);

    return ScoreFrame{
        .time = (i32)osu->getSelectedBeatmap()->getCurMusicPos(),  // NOTE: might be incorrect
        .slot_id = slot_id,
        .num300 = (u16)score->getNum300s(),
        .num100 = (u16)score->getNum100s(),
        .num50 = (u16)score->getNum50s(),
        .num_geki = (u16)score->getNum300gs(),
        .num_katu = (u16)score->getNum100ks(),
        .num_miss = (u16)score->getNumMisses(),
        .total_score = (i32)score->getScore(),
        .max_combo = (u16)score->getComboMax(),
        .current_combo = (u16)score->getCombo(),
        .is_perfect = perfect,
        .current_hp = (u8)(osu->getSelectedBeatmap()->getHealth() * 200.0),
        .tag = 0,         // tag gamemode currently not supported
        .is_scorev2 = 0,  // scorev2 currently not supported
    };
}
