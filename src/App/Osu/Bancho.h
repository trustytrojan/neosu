#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "templates.h"
#include "BanchoProtocol.h"
#include "UString.h"

class Image;

enum class ServerPolicy : uint8_t {
    NO,
    YES,
    NO_PREFERENCE,
};

struct Bancho {
    Bancho() = default;
    ~Bancho() = default;

    Bancho &operator=(const Bancho &) = delete;
    Bancho &operator=(Bancho &&) = delete;
    Bancho(const Bancho &) = delete;
    Bancho(Bancho &&) = delete;

    UString neosu_version;

    UString endpoint;
    i32 user_id{0};
    UString username;
    MD5Hash pw_md5;
    u8 oauth_challenge[32]{};
    u8 oauth_verifier[32]{};
    Room room;

    bool spectating{false};
    i32 spectated_player_id{0};
    std::vector<u32> spectators;
    std::vector<u32> fellow_spectators;

    UString server_icon_url;
    Image *server_icon{nullptr};

    ServerPolicy score_submission_policy{ServerPolicy::NO_PREFERENCE};
    bool submit_scores();

    UString user_agent;
    UString client_hashes;
    UString disk_uuid;
    UString install_id;

    bool match_started{false};
    Slot last_scores[16];

    [[nodiscard]] inline bool is_online() const { return this->user_id > 0; }

    // Room ID can be 0 on private servers! So we check if the room has players instead.
    [[nodiscard]] inline bool is_in_a_multi_room() const {
        return this->room.nb_players > 0;
    }

    [[nodiscard]] inline bool is_playing_a_multi_map() const {
        return this->match_started;
    }

    struct Channel {
        UString name;
        UString topic;
        u32 nb_members;
    };

    // static helpers
    static MD5Hash md5(u8 *msg, size_t msg_len);
    static void handle_packet(Packet *packet);
    static Packet build_login_packet();
    static std::unordered_map<std::string, Bancho::Channel *> chat_channels;
};

// initialized by NetworkHandler
// declared here for convenience
extern mcatomic_ref<Bancho *> bancho;
