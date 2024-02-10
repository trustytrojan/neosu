#include <blkid/blkid.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Engine.h"
#include "MD5.h"

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoProtocol.h"
#include "BanchoUsers.h"
#include "ConVar.h"
#include "Osu.h"
#include "OsuChat.h"
#include "OsuLobby.h"
#include "OsuNotificationOverlay.h"
#include "OsuOptionsMenu.h"
#include "OsuRoom.h"
#include "OsuSongBrowser2.h"
#include "OsuUIButton.h"
#include "OsuUISongBrowserUserButton.h"


// Let's commit the full TODO list, why not?
// TODO @kiwec: logout on quit. needs a rework of the networking logic. disconnect != logout
// TODO @kiwec: add "personal best" in online score list
// TODO @kiwec: fetch avatars and display in leaderboards, score browser, lobby list, etc
// TODO @kiwec: comb over every single option, and every single convar and make sure no cheats are possible in multiplayer
// TODO @kiwec: make webpage for https://mcosu.kiwec.net/
// TODO @kiwec: mark DMs as read when they're visible on screen
// TODO @kiwec: PLEASE polish lobby UI
// TODO @kiwec: handle freemod / allow user to pick mods in multi room
// TODO @kiwec: fix textboxes sharing focus


Bancho bancho;
std::unordered_map<std::string, Channel*> chat_channels;

void update_channel(UString name, UString topic, int32_t nb_members) {
  Channel* chan;
  auto name_str = std::string(name.toUtf8());
  auto it = chat_channels.find(name_str);
  if(it == chat_channels.end()) {
    chan = new Channel();
    chan->name = name;
    chat_channels[name_str] = chan;
  } else {
    chan = it->second;
  }

  chan->topic = topic;
  chan->nb_members = nb_members;
}

UString md5(uint8_t *msg, size_t msg_len) {
  MD5 hasher;
  hasher.update(msg, msg_len);
  hasher.finalize();

  uint8_t *digest = hasher.getDigest();

  char hash[33] = {0};
  for (int i = 0; i < 16; i++) {
    hash[i * 2] = "0123456789abcdef"[digest[i] >> 4];
    hash[i * 2 + 1] = "0123456789abcdef"[digest[i] & 0xf];
  }

  return UString(hash);
}

char *get_disk_uuid() {
  blkid_cache cache;
  blkid_get_cache(&cache, NULL);

  blkid_dev device;
  blkid_dev_iterate iter = blkid_dev_iterate_begin(cache);
  while (!blkid_dev_next(iter, &device)) {
    const char *devname = blkid_dev_devname(device);
    char *uuid = blkid_get_tag_value(cache, "UUID", devname);
    blkid_put_cache(cache);
    return uuid;
  }

  blkid_put_cache(cache);
  return NULL;
}

void handle_packet(Packet *packet) {
  if (packet->id == USER_ID) {
    bancho.user_id = read_int(packet);
    if(bancho.user_id > 0) {
      debugLog("Logged in as user #%d.\n", bancho.user_id);
      bancho.osu->m_optionsMenu->logInButton->setText("Disconnect");
      bancho.osu->m_optionsMenu->logInButton->setColor(0xffff0000);
      bancho.osu->m_optionsMenu->logInButton->is_loading = false;
      convar->getConVarByName("mp_autologin")->setValue(true);

      auto avatar_dir = UString::format(MCENGINE_DATA_DIR "avatars/%s", bancho.endpoint.toUtf8());
      if(!env->directoryExists(avatar_dir)) {
        env->createDirectory(avatar_dir);
      }
    } else {
      debugLog("Failed to log in, server returned code %d.\n", bancho.user_id);
      bancho.osu->m_optionsMenu->logInButton->setText("Log in");
      bancho.osu->m_optionsMenu->logInButton->setColor(0xff00ff00);
      bancho.osu->m_optionsMenu->logInButton->is_loading = false;
      // TODO @kiwec: also display error string returned in cho-token header
    }
  } else if (packet->id == RECV_MESSAGE) {
    UString sender = read_string(packet);
    UString text = read_string(packet);
    UString recipient = read_string(packet);
    int32_t sender_id = read_int(packet);

    bancho.osu->m_chat->addMessage(recipient, ChatMessage{
      .author_id = sender_id,
      .author_name = sender,
      .text = text,
    });
  } else if (packet->id == PONG) {
    // (nothing to do)
  } else if (packet->id == USER_STATS) {
    int32_t stats_user_id = read_int(packet);
    uint8_t action = read_byte(packet);

    UserInfo *user = get_user_info(stats_user_id);
    user->action = (Action)action;
    user->info_text = read_string(packet);
    user->map_md5 = read_string(packet);
    user->mode = (GameMode)read_byte(packet);
    user->map_id = read_int(packet);
    user->ranked_score = read_int64(packet);
    user->accuracy = read_float(packet);
    user->plays = read_int(packet);
    user->total_score = read_int64(packet);
    user->global_rank = read_int(packet);
    user->pp = read_short(packet);

    if(stats_user_id == bancho.user_id) {
      bancho.osu->m_songBrowser2->m_userButton->updateUserStats();
    }
  } else if (packet->id == USER_LOGOUT) {
    int32_t logged_out_id = read_int(packet);
    read_byte(packet);
    if(logged_out_id == bancho.user_id) {
      debugLog("Logged out.\n");
      disconnect();
    }
  } else if (packet->id == SPECTATOR_JOINED) {
    int32_t spectator_id = read_int(packet);
    debugLog("Spectator joined: user id %d\n", spectator_id);
    // TODO @kiwec: display spectators
    // TODO @kiwec: send packets so spectators can spectate
  } else if (packet->id == SPECTATOR_LEFT) {
    int32_t spectator_id = read_int(packet);
    debugLog("Spectator left: user id %d\n", spectator_id);
    // TODO @kiwec: update spectators
  } else if (packet->id == VERSION_UPDATE) {
    disconnect();
    bancho.osu->getNotificationOverlay()->addNotification(
        "Server uses an unsupported protocol version.");
  } else if (packet->id == SPECTATOR_CANT_SPECTATE) {
    int32_t spectator_id = read_int(packet);
    debugLog("Spectator can't spectate: user id %d\n", spectator_id);
    // TODO @kiwec: update spectators ("doesn't have map" list)
  } else if (packet->id == GET_ATTENTION) {
    // TODO @kiwec: force window focus, or flash icon in taskbar
  } else if (packet->id == NOTIFICATION) {
    UString notification = read_string(packet);
    bancho.osu->getNotificationOverlay()->addNotification(notification);
    // TODO @kiwec: don't do McOsu style notifications, since:
    // 1) they can't do multiline text
    // 2) they don't stack, if the server sends >1 you only see the latest
    // Maybe log them in the chat + display in top right like ppy client?
  } else if (packet->id == ROOM_UPDATED) {
    auto room = Room(packet);
    if(bancho.osu->m_lobby->isVisible()) {
      bancho.osu->m_lobby->updateRoom(room);
    } else if(room.id == bancho.room.id) {
      bancho.osu->m_room->on_room_updated(room);
    }
  } else if (packet->id == ROOM_CREATED) {
    auto room = new Room(packet);
    bancho.osu->m_lobby->addRoom(room);
  } else if (packet->id == ROOM_CLOSED) {
    int32_t room_id = read_int(packet);
    bancho.osu->m_lobby->removeRoom(room_id);
  } else if (packet->id == ROOM_JOIN_SUCCESS) {
    auto room = Room(packet);
    bancho.osu->m_room->on_room_joined(room);
  } else if (packet->id == ROOM_JOIN_FAIL) {
    bancho.osu->getNotificationOverlay()->addNotification("Failed to join room.");
    bancho.osu->m_lobby->on_room_join_failed();
  } else if (packet->id == FELLOW_SPECTATOR_JOINED) {
    uint32_t spectator_id = read_int(packet);
    (void)spectator_id; // (spectating not implemented; nothing to do)
  } else if (packet->id == FELLOW_SPECTATOR_LEFT) {
    uint32_t spectator_id = read_int(packet);
    (void)spectator_id; // (spectating not implemented; nothing to do)
  } else if (packet->id == MATCH_STARTED) {
    auto room = Room(packet);
    bancho.osu->m_room->on_match_started(room);
  } else if (packet->id == UPDATE_MATCH_SCORE || packet->id == MATCH_SCORE_UPDATED) {
    bancho.osu->m_room->on_match_score_updated(packet);
  } else if (packet->id == HOST_CHANGED) {
    debugLog("Host changed!\n");
    // TODO @kiwec: nothing to do?
  } else if (packet->id == MATCH_ALL_PLAYERS_LOADED) {
    bancho.osu->m_room->on_all_players_loaded();
  } else if (packet->id == MATCH_PLAYER_FAILED) {
    int32_t slot_id = read_int(packet);
    bancho.osu->m_room->on_player_failed(slot_id);
    // TODO @kiwec: isn't this player ragequit? not death?
  } else if (packet->id == MATCH_FINISHED) {
    bancho.osu->m_room->on_match_finished();
  } else if (packet->id == MATCH_SKIP) {
    bancho.osu->m_room->on_all_players_skipped();
  } else if (packet->id == CHANNEL_JOIN_SUCCESS) {
    UString name = read_string(packet);
    bancho.osu->m_chat->addChannel(name);
    bancho.osu->m_chat->addMessage(name, ChatMessage{
      .author_id = 0,
      .author_name = UString(""),
      .text = UString("Joined channel."),
    });
  } else if (packet->id == CHANNEL_INFO) {
    UString channel_name = read_string(packet);
    UString channel_topic = read_string(packet);
    int32_t nb_members = read_int(packet);
    update_channel(channel_name, channel_topic, nb_members);
  } else if (packet->id == LEFT_CHANNEL) {
    UString name = read_string(packet);
    bancho.osu->m_chat->removeChannel(name);
  } else if (packet->id == CHANNEL_AUTO_JOIN) {
    UString channel_name = read_string(packet);
    UString channel_topic = read_string(packet);
    int32_t nb_members = read_int(packet);
    update_channel(channel_name, channel_topic, nb_members);
  } else if (packet->id == PRIVILEGES) {
    read_int(packet);
    // (nothing to do)
  } else if (packet->id == PROTOCOL_VERSION) {
    int protocol_version = read_int(packet);
    if (protocol_version != 19) {
      disconnect();
      bancho.osu->getNotificationOverlay()->addNotification(
          "Server uses an unsupported protocol version.");
    }
  } else if (packet->id == MAIN_MENU_ICON) {
    UString icon = read_string(packet);
    debugLog("Main menu icon: %s\n", icon.toUtf8());
    // TODO @kiwec: handle this
    // for that, we need to download & cache images somewhere. would be useful for avatars too
  } else if (packet->id == MATCH_PLAYER_SKIPPED) {
    int32_t user_id = read_int(packet);
    bancho.osu->m_room->on_player_skip(user_id);
  } else if (packet->id == USER_PRESENCE) {
    int32_t presence_user_id = read_int(packet);
    UString presence_username = read_string(packet);

    UserInfo *user = get_user_info(presence_user_id);
    user->name = presence_username;
    user->utc_offset = read_byte(packet);
    user->country = read_byte(packet);
    user->privileges = read_byte(packet);
    user->longitude = read_float(packet);
    user->latitude = read_float(packet);
    user->global_rank = read_int(packet);

    // TODO @kiwec: update UIs that display this info?
  } else if (packet->id == RESTART) {
    int32_t ms = read_int(packet);
    debugLog("Restart: %d ms\n", ms);
    reconnect();
    // TODO @kiwec: wait 'ms' milliseconds before reconnecting
    // TODO @kiwec: if login failure, don't reconnect
  } else if (packet->id == MATCH_INVITE) {
    UString sender = read_string(packet);
    UString text = read_string(packet);
    UString recipient = read_string(packet);
    int32_t sender_id = read_int(packet);
    (void)sender_id;
    debugLog("(match invite) %s (%s): %s\n", sender, recipient, text);
    // TODO @kiwec: make a clickable link in chat?
  } else if (packet->id == CHANNEL_INFO_END) {
    // (nothing to do)
  } else if (packet->id == ROOM_PASSWORD_CHANGED) {
    UString new_password = read_string(packet);
    debugLog("Room changed password to %s\n", new_password);
    // TODO @kiwec
  } else if (packet->id == SILENCE_END) {
    int32_t delta = read_int(packet);
    debugLog("Silence ends in %d seconds.\n", delta);
    // TODO @kiwec
  } else if (packet->id == USER_SILENCED) {
    int32_t user_id = read_int(packet);
    debugLog("User #%d silenced.\n", user_id);
    // TODO @kiwec
  } else if (packet->id == USER_DM_BLOCKED) {
    read_string(packet);
    read_string(packet);
    UString blocked = read_string(packet);
    read_int(packet);
    debugLog("Blocked %s.\n", blocked);
    // TODO @kiwec
  } else if (packet->id == TARGET_IS_SILENCED) {
    read_string(packet);
    read_string(packet);
    UString blocked = read_string(packet);
    read_int(packet);
    debugLog("Silenced %s.\n", blocked);
    // TODO @kiwec
  } else if (packet->id == VERSION_UPDATE_FORCED) {
    disconnect();
    bancho.osu->getNotificationOverlay()->addNotification(
        "Server uses an unsupported protocol version.");
  } else if (packet->id == ACCOUNT_RESTRICTED) {
    debugLog("ACCOUNT RESTRICTED.\n");
    // TODO @kiwec
  } else if (packet->id == MATCH_ABORT) {
    bancho.osu->m_room->on_match_aborted();
  } else {
    debugLog("Unknown packet ID %d (%d bytes)!\n", packet->id, packet->size);
  }
}

Packet build_login_packet() {
  // Request format:
  // username\npasswd_md5\nosu_version|utc_offset|display_city|client_hashes|pm_private\n
  Packet packet = {0};

  write_bytes(&packet, (uint8_t *)bancho.username.toUtf8(), bancho.username.length());
  write_byte(&packet, '\n');

  write_bytes(&packet, (uint8_t *)bancho.pw_md5.toUtf8(), bancho.pw_md5.length());
  write_byte(&packet, '\n');

  const char *osu_version = "b20231102.5";
  write_bytes(&packet, (uint8_t *)osu_version, strlen(osu_version));
  write_byte(&packet, '|');

  // XXX: Get actual UTC offset
  write_byte(&packet, '1');
  write_byte(&packet, '|');

  // Don't dox the user's city
  write_byte(&packet, '0');
  write_byte(&packet, '|');

  char osu_path[PATH_MAX] = {0};
  readlink("/proc/self/exe", osu_path, PATH_MAX - 1);
  UString osu_path_md5 = md5((uint8_t *)osu_path, strlen(osu_path));
  write_bytes(&packet, (uint8_t *)osu_path_md5.toUtf8(), osu_path_md5.length());
  write_byte(&packet, ':');

  // XXX: Should get MAC addresses from network adapters
  const char *adapters = "runningunderwine";
  UString adapters_md5 = md5((uint8_t *)adapters, strlen(adapters));
  write_bytes(&packet, (uint8_t *)adapters, strlen(adapters));
  write_byte(&packet, ':');
  write_bytes(&packet, (uint8_t *)adapters_md5.toUtf8(), adapters_md5.length());
  write_byte(&packet, ':');

  static char *uuid = get_disk_uuid();
  UString disk_md5 = UString("00000000000000000000000000000000");
  if (uuid) {
    disk_md5 = md5((uint8_t *)uuid, strlen(uuid));
  }

  // XXX: Should be per-install unique ID.
  // I'm lazy so just sending disk signature twice.
  write_bytes(&packet, (uint8_t *)disk_md5.toUtf8(), disk_md5.length());
  write_byte(&packet, ':');

  write_bytes(&packet, (uint8_t *)disk_md5.toUtf8(), disk_md5.length());
  write_byte(&packet, ':');

  // Allow PMs from strangers
  write_byte(&packet, '|');
  write_byte(&packet, '0');

  write_byte(&packet, '\n');
  return packet;
}
