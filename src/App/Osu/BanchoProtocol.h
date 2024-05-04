#pragma once
#include "UString.h"
#include "types.h"

enum Action {
    IDLE = 0,
    AFK = 1,
    PLAYING = 2,
    EDITING = 3,
    MODDING = 4,
    MULTIPLAYER = 5,
    WATCHING = 6,
    UNKNOWN = 7,
    TESTING = 8,
    SUBMITTING = 9,
    PAUSED = 10,
    TESTING2 = 11,  // Was LOBBY but shows as "Testing" in-game
    MULTIPLAYING = 12,
    OSU_DIRECT = 13,
};

enum WinCondition {
    SCOREV1 = 0,
    ACCURACY = 1,
    COMBO = 2,
    SCOREV2 = 3,
};

enum GameMode {
    STANDARD = 0,
    TAIKO = 1,
    CATCH = 2,
    MANIA = 3,
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
    MATCH_STARTED = 46,
    MATCH_SCORE_UPDATED = 48,
    HOST_CHANGED = 50,
    MATCH_ALL_PLAYERS_LOADED = 53,
    MATCH_PLAYER_FAILED = 57,
    MATCH_FINISHED = 58,
    MATCH_SKIP = 61,
    CHANNEL_JOIN_SUCCESS = 64,
    CHANNEL_INFO = 65,
    LEFT_CHANNEL = 66,
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
    ROOM_PASSWORD_CHANGED = 91,
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
    CHANGE_ACTION = 0,
    SEND_PUBLIC_MESSAGE = 1,
    LOGOUT = 2,
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
    START_MATCH = 44,
    UPDATE_MATCH_SCORE = 47,
    FINISH_MATCH = 49,
    MATCH_CHANGE_MODS = 51,
    MATCH_LOAD_COMPLETE = 52,
    MATCH_NO_BEATMAP = 54,
    MATCH_NOT_READY = 55,
    MATCH_FAILED = 56,
    MATCH_HAS_BEATMAP = 59,
    MATCH_SKIP_REQUEST = 60,
    CHANNEL_JOIN = 63,
    BEATMAP_INFO_REQUEST = 68,
    TRANSFER_HOST = 70,
    FRIEND_ADD = 73,
    FRIEND_REMOVE = 74,
    MATCH_CHANGE_TEAM = 77,
    CHANNEL_PART = 78,
    RECEIVE_UPDATES = 79,
    SET_AWAY_MESSAGE = 82,
    IRC_ONLY = 84,
    USER_STATS_REQUEST = 85,
    MATCH_INVITE = 88,
    CHANGE_ROOM_PASSWORD = 90,
    TOURNAMENT_MATCH_INFO_REQUEST = 93,
    USER_PRESENCE_REQUEST = 97,
    USER_PRESENCE_REQUEST_ALL = 98,
    TOGGLE_BLOCK_NON_FRIEND_DMS = 99,
    TOURNAMENT_JOIN_MATCH_CHANNEL = 108,
    TOURNAMENT_EXIT_MATCH_CHANNEL = 109,
};

struct Packet {
    u16 id = 0;
    u8 *memory = nullptr;
    size_t size = 0;
    size_t pos = 0;
    u8 *extra = nullptr;
    u32 extra_int = 0;  // lazy

    void reserve(u32 newsize) {
        if(newsize <= size) return;
        memory = (u8 *)realloc(memory, newsize);
        size = newsize;
    }
};

struct Slot {
    // From ROOM_CREATED, ROOM_UPDATED
    u8 status = 0;  // bitfield of [quit, complete, playing, no_map, ready, not_ready, locked, open]
    u8 team = 0;
    u32 player_id = 0;
    u32 mods = 0;

    // From MATCH_PLAYER_SKIPPED
    bool skipped = false;

    // From MATCH_PLAYER_FAILED
    bool died = false;

    // From MATCH_SCORE_UPDATED
    i32 last_update_tms = 0;
    u16 num300 = 0;
    u16 num100 = 0;
    u16 num50 = 0;
    u16 num_geki = 0;
    u16 num_katu = 0;
    u16 num_miss = 0;
    i32 total_score = 0;
    u16 current_combo = 0;
    u16 max_combo = 0;
    u8 is_perfect = 0;
    u8 current_hp = 0;
    u8 tag = 0;
    double sv2_combo = 0.0;
    double sv2_bonus = 0.0;

    // locked
    bool is_locked() { return (status & 0b00000010); }

    // ready
    bool is_ready() { return (status & 0b00001000); }

    // no_map
    bool no_map() { return (status & 0b00010000); }

    // playing
    bool is_player_playing() { return (status & 0b00100000); }
    bool has_finished_playing() { return (status & 0b01000000); }

    // no_map
    bool is_missing_beatmap() { return (status & 0b00010000); }

    // quit
    bool has_quit() { return (status & 0b10000000); }

    // not_ready | ready | no_map | playing | complete
    bool has_player() { return (status & 0b01111100); }
};

class Room {
   public:
    Room();
    Room(Packet *packet);

    u16 id = 0;
    u8 in_progress = 0;
    u8 match_type = 0;
    u32 mods = 0;
    u32 seed = 0;
    bool all_players_loaded = false;
    bool all_players_skipped = false;
    bool player_loaded = false;

    UString name = "";
    UString password = "";
    bool has_password = false;

    UString map_name = "";
    MD5Hash map_md5;
    i32 map_id = 0;

    u8 mode = 0;
    u8 win_condition = 0;
    u8 team_type = 0;
    u8 freemods = 0;

    i32 host_id = 0;
    u8 nb_players = 0;
    u8 nb_open_slots = 0;
    Slot slots[16];

    bool all_players_ready() {
        for(int i = 0; i < 16; i++) {
            if(slots[i].has_player() && !slots[i].is_ready()) {
                return false;
            }
        }
        return true;
    }

    bool is_host();
    void pack(Packet *packet);
};

void read_bytes(Packet *packet, u8 *bytes, size_t n);
u32 read_uleb128(Packet *packet);
UString read_string(Packet *packet);
std::string read_stdstring(Packet *packet);
void skip_string(Packet *packet);
MD5Hash read_hash(Packet *packet);

// Null array for returning empty structures when trying to read more data out of a Packet than expected.
// See the read<> template below.
#define NULL_ARRAY_SIZE 128
extern u8 NULL_ARRAY[NULL_ARRAY_SIZE];

template <typename T>
T read(Packet *packet) {
    static_assert(sizeof(T) <= sizeof(NULL_ARRAY), "Please make NULL_ARRAY_SIZE bigger");

    if(packet->pos + sizeof(T) > packet->size) {
        packet->pos = packet->size + 1;
        return *(T *)NULL_ARRAY;
    } else {
        T out = *(T *)(packet->memory + packet->pos);
        packet->pos += sizeof(T);
        return out;
    }
}

void write_bytes(Packet *packet, u8 *bytes, size_t n);
void write_uleb128(Packet *packet, u32 num);
void write_string(Packet *packet, const char *str);
void write_hash(Packet *packet, MD5Hash hash);

template <typename T>
void write(Packet *packet, T t) {
    write_bytes(packet, (u8 *)&t, sizeof(T));
}
