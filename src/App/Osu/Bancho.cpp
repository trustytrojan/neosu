#ifdef _WIN32
// clang-format off
#include "cbase.h"

#include <stdio.h>
#include <wbemidl.h>

#include <sstream>
// clang-format on
#else
#include <blkid/blkid.h>
#include <linux/limits.h>
#endif

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoProtocol.h"
#include "BanchoUsers.h"
#include "Beatmap.h"
#include "Chat.h"
#include "ConVar.h"
#include "Engine.h"
#include "Lobby.h"
#include "crypto.h"
#include "NotificationOverlay.h"
#include "OptionsMenu.h"
#include "Osu.h"
#include "RankingScreen.h"
#include "RoomScreen.h"
#include "SongBrowser/SongBrowser.h"
#include "SoundEngine.h"
#include "SpectatorScreen.h"
#include "UIAvatar.h"
#include "UIButton.h"
#include "UserCard.h"

namespace proto = BANCHO::Proto;

namespace {  // static
bool print_new_channels = true;

void update_channel(const UString &name, const UString &topic, i32 nb_members) {
    Bancho::Channel *chan;
    auto name_str = std::string(name.toUtf8());
    auto it = Bancho::chat_channels.find(name_str);
    if(it == Bancho::chat_channels.end()) {
        chan = new Bancho::Channel();
        chan->name = name;
        Bancho::chat_channels[name_str] = chan;

        if(print_new_channels) {
            auto msg = ChatMessage{
                .tms = time(NULL),
                .author_id = 0,
                .author_name = UString(""),
                .text = UString::format("%s: %s", name.toUtf8(), topic.toUtf8()),
            };
            osu->chat->addMessage("#osu", msg, false);
        }
    } else {
        chan = it->second;
    }

    chan->topic = topic;
    chan->nb_members = nb_members;
}

UString get_disk_uuid() {
#ifdef _WIN32
    // ChatGPT'd, this looks absolutely insane but might just be regular Windows API...
    CoInitializeEx(0, COINIT_MULTITHREADED);
    CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE,
                         NULL);

    IWbemLocator *pLoc = NULL;
    auto hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);
    if(FAILED(hres)) {
        debugLog("Failed to create IWbemLocator object. Error code = 0x%x\n", hres);
        return UString("error getting disk uuid");
    }

    IWbemServices *pSvc = NULL;
    BSTR bstr_root = SysAllocString(L"ROOT\\CIMV2");
    hres = pLoc->ConnectServer(bstr_root, NULL, NULL, 0, 0, 0, 0, &pSvc);
    if(FAILED(hres)) {
        debugLog("Could not connect. Error code = 0x%x\n", hres);
        pLoc->Release();
        SysFreeString(bstr_root);
        return UString("error getting disk uuid");
    }
    SysFreeString(bstr_root);

    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL,
                             RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if(FAILED(hres)) {
        debugLog("Could not set proxy blanket. Error code = 0x%x\n", hres);
        pSvc->Release();
        pLoc->Release();
        return UString("error getting disk uuid");
    }

    UString uuid = "";
    IEnumWbemClassObject *pEnumerator = NULL;
    BSTR bstr_wql = SysAllocString(L"WQL");
    BSTR bstr_sql = SysAllocString(L"SELECT * FROM Win32_DiskDrive");
    hres =
        pSvc->ExecQuery(bstr_wql, bstr_sql, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if(FAILED(hres)) {
        debugLog("Query for hard drive UUID failed. Error code = 0x%x\n", hres);
        pSvc->Release();
        pLoc->Release();
        SysFreeString(bstr_wql);
        SysFreeString(bstr_sql);
        return UString("error getting disk uuid");
    }
    SysFreeString(bstr_wql);
    SysFreeString(bstr_sql);

    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
    while(pEnumerator && uuid.length() == 0) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if(0 == uReturn) {
            break;
        }

        VARIANT vtProp;
        hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
        if(SUCCEEDED(hr)) {
            uuid = vtProp.bstrVal;
            VariantClear(&vtProp);
        }

        pclsObj->Release();
    }

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();

    return uuid;
#else
    blkid_cache cache;
    blkid_get_cache(&cache, NULL);

    blkid_dev device;
    blkid_dev_iterate iter = blkid_dev_iterate_begin(cache);
    while(!blkid_dev_next(iter, &device)) {
        const char *devname = blkid_dev_devname(device);
        char *uuid = blkid_get_tag_value(cache, "UUID", devname);
        blkid_put_cache(cache);

        UString w_uuid = UString(uuid);

        // Not sure if we own the string here or not, leak too small to matter anyway
        // free(uuid);

        return w_uuid;
    }

    blkid_put_cache(cache);
    return UString("error getting disk uuid");
#endif
}
}  // namespace

std::unordered_map<std::string, Bancho::Channel *> Bancho::chat_channels;

MD5Hash Bancho::md5(u8 *msg, size_t msg_len) {
    u8 digest[16];
    crypto::hash::md5(msg, msg_len, &digest[0]);

    MD5Hash out;
    for(int i = 0; i < 16; i++) {
        out.hash[i * 2] = "0123456789abcdef"[digest[i] >> 4];
        out.hash[i * 2 + 1] = "0123456789abcdef"[digest[i] & 0xf];
    }
    out.hash[32] = 0;
    return out;
}

void Bancho::handle_packet(Packet *packet) {
    // XXX: This is a bit of a mess, should at least group packets by type for readability
    if (cv::debug_network.getBool() && packet) {
        debugLogF("packet id: {}\n", packet->id);
    }
    if(packet->id == USER_ID) {
        i32 new_user_id = proto::read<i32>(packet);
        bancho->user_id = new_user_id;
        osu->optionsMenu->update_login_button();

        if(new_user_id > 0) {
            debugLog("Logged in as user #%d.\n", new_user_id);
            cv::mp_autologin.setValue(true);
            print_new_channels = true;

            std::string avatar_dir = fmt::format(MCENGINE_DATA_DIR "avatars/{:s}", bancho->endpoint.toUtf8());
            if(!env->directoryExists(avatar_dir)) {
                env->createDirectory(avatar_dir);
            }

            std::string replays_dir = fmt::format(MCENGINE_DATA_DIR "replays/{:s}", bancho->endpoint.toUtf8());
            if(!env->directoryExists(replays_dir)) {
                env->createDirectory(replays_dir);
            }

            osu->onUserCardChange(bancho->username);

            // XXX: We should toggle between "offline" sorting options and "online" ones
            //      Online ones would be "Local scores", "Global", "Country", "Selected mods" etc
            //      While offline ones would be "By score", "By pp", etc
            osu->songBrowser2->onSortScoresChange(UString("Online Leaderboard"), 0);

            // If server sent a score submission policy, update options menu to hide the checkbox
            osu->optionsMenu->scheduleLayoutUpdate();
        } else {
            cv::mp_autologin.setValue(false);

            debugLog("Failed to log in, server returned code %d.\n", bancho->user_id);
            UString errmsg = UString::format("Failed to log in: %s (code %d)\n", BANCHO::Net::cho_token.toUtf8(),
                                             bancho->user_id);
            if(new_user_id == -2) {
                errmsg = "Client version is too old to connect to this server.";
            } else if(new_user_id == -3 || new_user_id == -4) {
                errmsg = "You are banned from this server.";
            } else if(new_user_id == -6) {
                errmsg = "You need to buy supporter to connect to this server.";
            } else if(new_user_id == -7) {
                errmsg = "You need to reset your password to connect to this server.";
            } else if(new_user_id == -8) {
                errmsg = "Open the verification link sent to your email, then log in again.";
            } else {
                if(BANCHO::Net::cho_token == UString("user-already-logged-in")) {
                    errmsg = "Already logged in on another client.";
                } else if(BANCHO::Net::cho_token == UString("unknown-username")) {
                    errmsg = UString::format("No account by the username '%s' exists.", bancho->username.toUtf8());
                } else if(BANCHO::Net::cho_token == UString("incorrect-credentials")) {
                    errmsg = "This username is not registered.";
                } else if(BANCHO::Net::cho_token == UString("incorrect-password")) {
                    errmsg = "Incorrect password.";
                } else if(BANCHO::Net::cho_token == UString("contact-staff")) {
                    errmsg = "Please contact an administrator of the server.";
                }
            }
            osu->getNotificationOverlay()->addToast(errmsg);
        }
    } else if(packet->id == RECV_MESSAGE) {
        UString sender = proto::read_string(packet);
        UString text = proto::read_string(packet);
        UString recipient = proto::read_string(packet);
        i32 sender_id = proto::read<i32>(packet);

        auto msg = ChatMessage{
            .tms = time(NULL),
            .author_id = sender_id,
            .author_name = sender,
            .text = text,
        };
        osu->chat->addMessage(recipient, msg);
    } else if(packet->id == PONG) {
        // (nothing to do)
    } else if(packet->id == USER_STATS) {
        i32 raw_id = proto::read<i32>(packet);
        i32 stats_user_id = abs(raw_id);  // IRC clients are sent with negative IDs, hence the abs()
        u8 action = proto::read<u8>(packet);

        UserInfo *user = BANCHO::User::get_user_info(stats_user_id);
        user->irc_user = raw_id < 0;
        user->stats_tms = Timing::getTicksMS();
        user->action = (Action)action;
        user->info_text = proto::read_string(packet);
        user->map_md5 = proto::read_hash(packet);
        user->mods = proto::read<u32>(packet);
        user->mode = (GameMode)proto::read<u8>(packet);
        user->map_id = proto::read<i32>(packet);
        user->ranked_score = proto::read<i64>(packet);
        user->accuracy = proto::read<f32>(packet);
        user->plays = proto::read<i32>(packet);
        user->total_score = proto::read<i64>(packet);
        user->global_rank = proto::read<i32>(packet);
        user->pp = proto::read<u16>(packet);

        if(stats_user_id == bancho->user_id) {
            osu->userButton->updateUserStats();
        }
        if(stats_user_id == bancho->spectated_player_id) {
            osu->spectatorScreen->userCard->updateUserStats();
        }

        osu->chat->updateUserList();
    } else if(packet->id == USER_LOGOUT) {
        i32 logged_out_id = proto::read<i32>(packet);
        proto::read<u8>(packet);
        if(logged_out_id == bancho->user_id) {
            debugLog("Logged out.\n");
            BANCHO::Net::disconnect();
        } else {
            BANCHO::User::logout_user(logged_out_id);
        }
    } else if(packet->id == IN_SPECTATE_FRAMES) {
        i32 extra = proto::read<i32>(packet);
        (void)extra;  // this is mania seed or something we can't use

        if(bancho->spectating) {
            UserInfo *info = BANCHO::User::get_user_info(bancho->spectated_player_id, true);
            auto beatmap = osu->getSelectedBeatmap();

            u16 nb_frames = proto::read<u16>(packet);
            for(u16 i = 0; i < nb_frames; i++) {
                auto frame = proto::read<LiveReplayFrame>(packet);

                if(frame.mouse_x < 0 || frame.mouse_x > 512 || frame.mouse_y < 0 || frame.mouse_y > 384) {
                    debugLog("WEIRD FRAME: time %d, x %f, y %f\n", frame.time, frame.mouse_x, frame.mouse_y);
                }

                beatmap->spectated_replay.push_back(LegacyReplay::Frame{
                    .cur_music_pos = frame.time,
                    .milliseconds_since_last_frame = 0,  // fixed below
                    .x = frame.mouse_x,
                    .y = frame.mouse_y,
                    .key_flags = frame.key_flags,
                });
            }

            // NOTE: Server can send frames in the wrong order. So we're correcting it here.
            std::sort(beatmap->spectated_replay.begin(), beatmap->spectated_replay.end(),
                      [](LegacyReplay::Frame a, LegacyReplay::Frame b) { return a.cur_music_pos < b.cur_music_pos; });
            beatmap->last_frame_ms = 0;
            for(auto &frame : beatmap->spectated_replay) {
                frame.milliseconds_since_last_frame = frame.cur_music_pos - beatmap->last_frame_ms;
                beatmap->last_frame_ms = frame.cur_music_pos;
            }

            LiveReplayBundle::Action action = (LiveReplayBundle::Action)proto::read<u8>(packet);
            info->spec_action = action;

            if(osu->isInPlayMode()) {
                if(action == LiveReplayBundle::Action::SONG_SELECT) {
                    info->map_id = 0;
                    info->map_md5 = MD5Hash();
                    beatmap->stop(true);
                    osu->songBrowser2->bHasSelectedAndIsPlaying = false;
                }
                if(action == LiveReplayBundle::Action::UNPAUSE) {
                    beatmap->spectate_pause = false;
                }
                if(action == LiveReplayBundle::Action::PAUSE) {
                    beatmap->spectate_pause = true;
                }
                if(action == LiveReplayBundle::Action::SKIP) {
                    beatmap->skipEmptySection();
                }
                if(action == LiveReplayBundle::Action::FAIL) {
                    beatmap->fail(true);
                }
                if(action == LiveReplayBundle::Action::NEW_SONG) {
                    osu->rankingScreen->setVisible(false);
                    beatmap->restart(true);
                    beatmap->update();
                }
            }

            auto score_frame = proto::read<ScoreFrame>(packet);
            beatmap->score_frames.push_back(score_frame);

            auto sequence = proto::read<u16>(packet);
            (void)sequence;  // don't know how to use this
        }
    } else if(packet->id == SPECTATOR_JOINED) {
        i32 spectator_id = proto::read<i32>(packet);
        if(std::find(bancho->spectators.begin(), bancho->spectators.end(), spectator_id) == bancho->spectators.end()) {
            debugLog("Spectator joined: user id %d\n", spectator_id);
            bancho->spectators.push_back(spectator_id);
        }
    } else if(packet->id == SPECTATOR_LEFT) {
        i32 spectator_id = proto::read<i32>(packet);
        auto it = std::find(bancho->spectators.begin(), bancho->spectators.end(), spectator_id);
        if(it != bancho->spectators.end()) {
            debugLog("Spectator left: user id %d\n", spectator_id);
            bancho->spectators.erase(it);
        }
    } else if(packet->id == SPECTATOR_CANT_SPECTATE) {
        i32 spectator_id = proto::read<i32>(packet);
        debugLog("Spectator can't spectate: user id %d\n", spectator_id);
    } else if(packet->id == FELLOW_SPECTATOR_JOINED) {
        i32 spectator_id = proto::read<i32>(packet);
        if(std::find(bancho->fellow_spectators.begin(), bancho->fellow_spectators.end(), spectator_id) ==
           bancho->fellow_spectators.end()) {
            debugLog("Fellow spectator joined: user id %d\n", spectator_id);
            bancho->fellow_spectators.push_back(spectator_id);
        }
    } else if(packet->id == FELLOW_SPECTATOR_LEFT) {
        i32 spectator_id = proto::read<i32>(packet);
        auto it = std::find(bancho->fellow_spectators.begin(), bancho->fellow_spectators.end(), spectator_id);
        if(it != bancho->fellow_spectators.end()) {
            debugLog("Fellow spectator left: user id %d\n", spectator_id);
            bancho->fellow_spectators.erase(it);
        }
    } else if(packet->id == GET_ATTENTION) {
        // (nothing to do)
    } else if(packet->id == NOTIFICATION) {
        UString notification = proto::read_string(packet);
        osu->getNotificationOverlay()->addToast(notification, 0xffffdd00);
    } else if(packet->id == ROOM_UPDATED) {
        auto room = Room(packet);
        if(osu->lobby->isVisible()) {
            osu->lobby->updateRoom(room);
        } else if(room.id == bancho->room.id) {
            osu->room->on_room_updated(room);
        }
    } else if(packet->id == ROOM_CREATED) {
        auto room = new Room(packet);
        osu->lobby->addRoom(room);
    } else if(packet->id == ROOM_CLOSED) {
        i32 room_id = proto::read<i32>(packet);
        osu->lobby->removeRoom(room_id);
    } else if(packet->id == ROOM_JOIN_SUCCESS) {
        // Sanity, in case some trolley admins do funny business
        if(bancho->spectating) {
            stop_spectating();
        }
        if(osu->isInPlayMode()) {
            osu->getSelectedBeatmap()->stop(true);
            osu->songBrowser2->bHasSelectedAndIsPlaying = false;
        }

        auto room = Room(packet);
        osu->room->on_room_joined(room);
    } else if(packet->id == ROOM_JOIN_FAIL) {
        osu->getNotificationOverlay()->addToast("Failed to join room.");
        osu->lobby->on_room_join_failed();
    } else if(packet->id == MATCH_STARTED) {
        auto room = Room(packet);
        osu->room->on_match_started(room);
    } else if(packet->id == UPDATE_MATCH_SCORE || packet->id == MATCH_SCORE_UPDATED) {
        osu->room->on_match_score_updated(packet);
    } else if(packet->id == HOST_CHANGED) {
        // (nothing to do)
    } else if(packet->id == MATCH_ALL_PLAYERS_LOADED) {
        osu->room->on_all_players_loaded();
    } else if(packet->id == MATCH_PLAYER_FAILED) {
        i32 slot_id = proto::read<i32>(packet);
        osu->room->on_player_failed(slot_id);
    } else if(packet->id == MATCH_FINISHED) {
        osu->room->on_match_finished();
    } else if(packet->id == MATCH_SKIP) {
        osu->room->on_all_players_skipped();
    } else if(packet->id == CHANNEL_JOIN_SUCCESS) {
        UString name = proto::read_string(packet);
        auto msg = ChatMessage{
            .tms = time(NULL),
            .author_id = 0,
            .author_name = UString(""),
            .text = UString("Joined channel."),
        };
        osu->chat->addChannel(name);
        osu->chat->addMessage(name, msg, false);
    } else if(packet->id == CHANNEL_INFO) {
        UString channel_name = proto::read_string(packet);
        UString channel_topic = proto::read_string(packet);
        i32 nb_members = proto::read<i32>(packet);
        update_channel(channel_name, channel_topic, nb_members);
    } else if(packet->id == LEFT_CHANNEL) {
        UString name = proto::read_string(packet);
        osu->chat->removeChannel(name);
    } else if(packet->id == CHANNEL_AUTO_JOIN) {
        UString channel_name = proto::read_string(packet);
        UString channel_topic = proto::read_string(packet);
        i32 nb_members = proto::read<i32>(packet);
        update_channel(channel_name, channel_topic, nb_members);
    } else if(packet->id == PRIVILEGES) {
        proto::read<u32>(packet);
        // (nothing to do)
    } else if(packet->id == FRIENDS_LIST) {
        BANCHO::User::friends.clear();

        u16 nb_friends = proto::read<u16>(packet);
        for(u16 i = 0; i < nb_friends; i++) {
            i32 friend_id = proto::read<i32>(packet);
            BANCHO::User::friends.push_back(friend_id);
        }
    } else if(packet->id == PROTOCOL_VERSION) {
        int protocol_version = proto::read<i32>(packet);
        if(protocol_version != 19) {
            osu->getNotificationOverlay()->addToast("This server may use an unsupported protocol version.");
        }
    } else if(packet->id == MAIN_MENU_ICON) {
        UString icon = proto::read_string(packet);
        auto urls = icon.split("|");
        if(urls.size() == 2 && ((urls[0].find("http://") == 0) || urls[0].find("https://") == 0)) {
            bancho->server_icon_url = urls[0];
        }
    } else if(packet->id == MATCH_PLAYER_SKIPPED) {
        i32 user_id = proto::read<i32>(packet);
        osu->room->on_player_skip(user_id);
    } else if(packet->id == USER_PRESENCE) {
        i32 raw_id = proto::read<i32>(packet);
        i32 presence_user_id = abs(raw_id);  // IRC clients are sent with negative IDs, hence the abs()
        UString presence_username = proto::read_string(packet);

        UserInfo *user = BANCHO::User::get_user_info(presence_user_id);
        user->irc_user = raw_id < 0;
        user->has_presence = true;
        user->name = presence_username;
        user->utc_offset = proto::read<u8>(packet);
        user->country = proto::read<u8>(packet);
        user->privileges = proto::read<u8>(packet);
        user->longitude = proto::read<f32>(packet);
        user->latitude = proto::read<f32>(packet);
        user->global_rank = proto::read<i32>(packet);

        osu->chat->updateUserList();
    } else if(packet->id == USER_PRESENCE_SINGLE) {
        BANCHO::User::get_user_info(proto::read<i32>(packet));  // add to online_users list
    } else if(packet->id == USER_PRESENCE_BUNDLE) {
        u16 nb_users = proto::read<u16>(packet);
        for(u16 i = 0; i < nb_users; i++) {
            BANCHO::User::get_user_info(proto::read<i32>(packet));  // add to online_users list
        }
    } else if(packet->id == RESTART) {
        // XXX: wait 'ms' milliseconds before reconnecting
        i32 ms = proto::read<i32>(packet);
        (void)ms;

        // Some servers send "restart" packets when password is incorrect
        // So, don't retry unless actually logged in
        if(bancho->is_online()) {
            BANCHO::Net::reconnect();
        }
    } else if(packet->id == MATCH_INVITE) {
        UString sender = proto::read_string(packet);
        UString text = proto::read_string(packet);
        UString recipient = proto::read_string(packet);
        (void)recipient;
        i32 sender_id = proto::read<i32>(packet);
        auto msg = ChatMessage{
            .tms = time(NULL),
            .author_id = sender_id,
            .author_name = sender,
            .text = text,
        };
        osu->chat->addMessage(recipient, msg);
    } else if(packet->id == CHANNEL_INFO_END) {
        print_new_channels = false;
        osu->chat->join("#announce");
        osu->chat->join("#osu");
    } else if(packet->id == ROOM_PASSWORD_CHANGED) {
        UString new_password = proto::read_string(packet);
        debugLog("Room changed password to %s\n", new_password.toUtf8());
        bancho->room.password = new_password;
    } else if(packet->id == SILENCE_END) {
        i32 delta = proto::read<i32>(packet);
        debugLog("Silence ends in %d seconds.\n", delta);
        // XXX: Prevent user from sending messages while silenced
    } else if(packet->id == USER_SILENCED) {
        i32 user_id = proto::read<i32>(packet);
        debugLog("User #%d silenced.\n", user_id);
    } else if(packet->id == USER_DM_BLOCKED) {
        proto::read_string(packet);
        proto::read_string(packet);
        UString blocked = proto::read_string(packet);
        proto::read<u32>(packet);
        debugLog("Blocked %s.\n", blocked.toUtf8());
    } else if(packet->id == TARGET_IS_SILENCED) {
        proto::read_string(packet);
        proto::read_string(packet);
        UString blocked = proto::read_string(packet);
        proto::read<u32>(packet);
        debugLog("Silenced %s.\n", blocked.toUtf8());
    } else if(packet->id == VERSION_UPDATE) {
        // (nothing to do)
    } else if(packet->id == VERSION_UPDATE_FORCED) {
        BANCHO::Net::disconnect();
        osu->getNotificationOverlay()->addToast("This server requires a newer client version.");
    } else if(packet->id == ACCOUNT_RESTRICTED) {
        osu->getNotificationOverlay()->addToast("Account restricted.");
        BANCHO::Net::disconnect();
    } else if(packet->id == MATCH_ABORT) {
        osu->room->on_match_aborted();
    } else {
        debugLog("Unknown packet ID %d (%d bytes)!\n", packet->id, packet->size);
    }
}

Packet Bancho::build_login_packet() {
    // Request format:
    // username\npasswd_md5\nosu_version|utc_offset|display_city|client_hashes|pm_private\n
    Packet packet;

    proto::write_bytes(&packet, (u8 *)bancho->username.toUtf8(), bancho->username.lengthUtf8());
    proto::write<u8>(&packet, '\n');

    proto::write_bytes(&packet, (u8 *)bancho->pw_md5.hash.data(), 32);
    proto::write<u8>(&packet, '\n');

    proto::write_bytes(&packet, (u8 *)OSU_VERSION, sizeof(OSU_VERSION) - 1);
    proto::write<u8>(&packet, '|');

    // UTC offset
    time_t now = time(NULL);
    auto gmt = gmtime(&now);
    auto local_time = localtime(&now);
    int utc_offset = difftime(mktime(local_time), mktime(gmt)) / 3600;
    if(utc_offset < 0) {
        proto::write<u8>(&packet, '-');
        utc_offset *= -1;
    }
    proto::write<u8>(&packet, '0' + utc_offset);
    proto::write<u8>(&packet, '|');

    // Don't dox the user's city
    proto::write<u8>(&packet, '0');
    proto::write<u8>(&packet, '|');

    const char *osu_path = Environment::getPathToSelf().c_str();

    MD5Hash osu_path_md5 = Bancho::md5((u8 *)osu_path, strlen(osu_path));

    // XXX: Should get MAC addresses from network adapters
    // NOTE: Not sure how the MD5 is computed - does it include final "." ?
    const char *adapters = "runningunderwine";
    MD5Hash adapters_md5 = Bancho::md5((u8 *)adapters, strlen(adapters));

    // XXX: Should remove '|' from the disk UUID just to be safe
    bancho->disk_uuid = get_disk_uuid();
    MD5Hash disk_md5 = Bancho::md5((u8 *)bancho->disk_uuid.toUtf8(), bancho->disk_uuid.lengthUtf8());

    // XXX: Not implemented, I'm lazy so just reusing disk signature
    bancho->install_id = bancho->disk_uuid;
    MD5Hash install_md5 = Bancho::md5((u8 *)bancho->install_id.toUtf8(), bancho->install_id.lengthUtf8());

    bancho->client_hashes = UString::fmt("{:s}:{:s}:{:s}:{:s}:{:s}:", osu_path_md5.hash.data(), adapters,
                                         adapters_md5.hash.data(), install_md5.hash.data(), disk_md5.hash.data());
    proto::write_bytes(&packet, (u8 *)bancho->client_hashes.toUtf8(), bancho->client_hashes.lengthUtf8());

    // Allow PMs from strangers
    proto::write<u8>(&packet, '|');
    proto::write<u8>(&packet, '0');

    proto::write<u8>(&packet, '\n');

    return packet;
}

bool Bancho::submit_scores() {
    if(this->score_submission_policy == ServerPolicy::NO_PREFERENCE) {
        return cv::submit_scores.getBool();
    } else if(this->score_submission_policy == ServerPolicy::YES) {
        return true;
    } else {
        return false;
    }
}
