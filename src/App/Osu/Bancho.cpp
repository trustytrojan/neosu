#include <blkid/blkid.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Bancho.h"
#include "BanchoProtocol.h"
#include "Engine.h"
#include "MD5.h"
#include "OsuNotificationOverlay.h"

Bancho bancho;

std::string md5(uint8_t *msg, size_t msg_len) {
  MD5 hasher;
  hasher.update(msg, msg_len);
  hasher.finalize();

  std::string hash = "";
  uint8_t *digest = hasher.getDigest();
  for (int i = 0; i < 16; i++) {
    hash += "0123456789abcdef"[digest[i] >> 4];
    hash += "0123456789abcdef"[digest[i] & 0xf];
  }

  return hash;
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
  if (packet->id == 5) {
    // USER_ID
    bancho.user_id = read_int(packet);
    debugLog("Received user ID %d.\n", bancho.user_id);
  } else if (packet->id == 7) {
    // SEND_MESSAGE
    char *sender = read_string(packet);
    char *text = read_string(packet);
    char *recipient = read_string(packet);
    int32_t sender_id = read_int(packet);
    debugLog("%s (%s): %s\n", sender, recipient, text);
    free(sender);
    free(text);
    free(recipient);
  } else if (packet->id == 8) {
    // PONG
    // (nothing to do)
  } else if (packet->id == 11) {
    // USER_STATS
    int32_t stats_user_id = read_int(packet);
    uint8_t action = read_byte(packet);
    char *info_text = read_string(packet);
    char *map_md5 = read_string(packet);
    uint8_t mode = read_byte(packet);
    int32_t map_id = read_int(packet);
    int64_t ranked_score = read_int64(packet);
    float accuracy = read_float(packet);
    int32_t plays = read_int(packet);
    int64_t total_score = read_int64(packet);
    int32_t global_rank = read_int(packet);
    uint16_t pp = read_short(packet);
    debugLog("Received stats for user %d (ranked #%d) [%s]\n", stats_user_id,
             global_rank, info_text);
    free(info_text);
    free(map_md5);
  } else if (packet->id == 12) {
    // USER_LOGOUT
    int32_t logged_out_id = read_int(packet);
    int8_t zero = read_byte(packet);
    debugLog("Logged out.\n");
  } else if (packet->id == 13) {
    // SPECTATOR_JOINED
    int32_t spectator_id = read_int(packet);
    debugLog("Spectator joined: user id %d\n", spectator_id);
  } else if (packet->id == 14) {
    // SPECTATOR_LEFT
    int32_t spectator_id = read_int(packet);
    debugLog("Spectator left: user id %d\n", spectator_id);
  } else if (packet->id == 19) {
    // VERSION_UPDATE
    // (nothing to do?)
  } else if (packet->id == 22) {
    // SPECTATOR_CANT_SPECTATE
    int32_t spectator_id = read_int(packet);
    debugLog("Spectator can't spectate: user id %d\n", spectator_id);
  } else if (packet->id == 23) {
    // GET_ATTENTION
    debugLog("ATTENTION!!!! (is this for flashing taskbar?)\n");
  } else if (packet->id == 24) {
    // NOTIFICATION
    char *notification = read_string(packet);
    bancho.osu->getNotificationOverlay()->addNotification(notification);
    free(notification);
  } else if (packet->id == 26) {
    // ROOM UPDATED
    Room *room = read_room(packet);
    debugLog("UPDATED Room %s: %d players\n", room->name, room->nb_players);
    free_room(room);
  } else if (packet->id == 27) {
    // ROOM CREATED
    Room *room = read_room(packet);
    debugLog("CREATED Room %s: %d players\n", room->name, room->nb_players);
    free_room(room);
  } else if (packet->id == 28) {
    // ROOM CLOSED
    int32_t room_id = read_int(packet);
    debugLog("CLOSED Room #%d\n", room_id);
  } else if (packet->id == 36) {
    // MATCH_JOIN_SUCCESS
    Room *room = read_room(packet);
    debugLog("JOINED Room %s: %d players\n", room->name, room->nb_players);
    free_room(room);
  } else if (packet->id == 37) {
    // MATCH_JOIN_FAIL
    debugLog("FAILED to join room!\n");
  } else if (packet->id == 42) {
    // FELLOW_SPECTATOR_JOINED
    uint32_t spectator_id = read_int(packet);
    debugLog("Fellow spectator joined with user id %d\n", spectator_id);
  } else if (packet->id == 43) {
    // FELLOW_SPECTATOR_LEFT
    uint32_t spectator_id = read_int(packet);
    debugLog("Fellow spectator left with user id %d\n", spectator_id);
  } else if (packet->id == 46) {
    // MATCH_START
    Room *room = read_room(packet);
    debugLog("MATCH STARTED IN Room %s: %d players\n", room->name,
             room->nb_players);
    free_room(room);
  } else if (packet->id == 48) {
    // MATCH_SCORE_UPDATE
    int32_t time = read_int(packet);
    uint8_t id = read_byte(packet);
    uint16_t num300 = read_short(packet);
    uint16_t num100 = read_short(packet);
    uint16_t num50 = read_short(packet);
    uint16_t num_geki = read_short(packet);
    uint16_t num_katu = read_short(packet);
    uint16_t num_miss = read_short(packet);
    int32_t total_score = read_int(packet);
    uint16_t current_combo = read_short(packet);
    uint16_t max_combo = read_short(packet);
    uint8_t is_perfect = read_byte(packet);
    uint8_t current_hp = read_byte(packet);
    uint8_t tag = read_byte(packet);
    uint8_t is_scorev2 = read_byte(packet);
    // NOTE: where is voteskip here?
  } else if (packet->id == 50) {
    // MATCH_TRANSFER_HOST
    debugLog("Host transferred!\n");
  } else if (packet->id == 53) {
    // MATCH_ALL_PLAYERS_LOADED
    debugLog("All players loaded!\n");
  } else if (packet->id == 57) {
    // MATCH_PLAYER_FAILED
    int32_t slot_id = read_int(packet);
    debugLog("Player in slot %d has failed.\n", slot_id);
  } else if (packet->id == 58) {
    // MATCH_COMPLETE
    debugLog("Match complete!\n");
  } else if (packet->id == 61) {
    // MATCH_SKIP
    debugLog("Skipping!\n");
  } else if (packet->id == 64) {
    // CHANNEL_JOIN_SUCCESS
    char *name = read_string(packet);
    debugLog("Success in joining channel %s.\n", name);
    free(name);
  } else if (packet->id == 65) {
    // CHANNEL_INFO
    char *channel_name = read_string(packet);
    char *channel_topic = read_string(packet);
    int32_t nb_members = read_int(packet);
    debugLog("Channel '%s' (%d members) has topic: %s\n", channel_name,
             nb_members, channel_topic);
    free(channel_name);
    free(channel_topic);
  } else if (packet->id == 66) {
    // CHANNEL_KICK
    char *name = read_string(packet);
    debugLog("Kicked from channel %s.\n", name);
    free(name);
  } else if (packet->id == 67) {
    // CHANNEL_AUTO_JOIN
    char *channel_name = read_string(packet);
    char *channel_topic = read_string(packet);
    int32_t nb_members = read_int(packet);
    debugLog("Auto-joined channel '%s' (%d members) with topic: %s\n",
             channel_name, nb_members, channel_topic);
    free(channel_name);
    free(channel_topic);
  } else if (packet->id == 71) {
    // PRIVILEGES
    int privileges = read_int(packet);
    debugLog("Privileges: %d\n", privileges);
  } else if (packet->id == 75) {
    // PROTOCOL_VERSION
    int protocol_version = read_int(packet);
    if (protocol_version != 19) {
      bancho.user_id = 0;
      bancho.osu->getNotificationOverlay()->addNotification(
          "Server uses an unsupported protocol version.");
    }
  } else if (packet->id == 76) {
    // MAIN_MENU_ICON
    char *icon = read_string(packet);
    debugLog("Main menu icon: %s\n", icon);
    free(icon);
  } else if (packet->id == 81) {
    // MATCH_PLAYER_SKIPPED
    int32_t user_id = read_int(packet);
    debugLog("User %d has voted to skip.\n", user_id);
  } else if (packet->id == 83) {
    // USER_PRESENCE
    int32_t presence_user_id = read_int(packet);
    char *presence_username = read_string(packet);
    uint8_t presence_utc_offset = read_byte(packet);
    uint8_t presence_country_code = read_byte(packet);
    uint8_t presence_privileges = read_byte(packet);
    float presence_longitude = read_float(packet);
    float presence_latitude = read_float(packet);
    int32_t presence_global_rank = read_int(packet);
    debugLog("%s (user %d) is rank #%d, coords %f:%f\n", presence_username,
             presence_user_id, presence_global_rank, presence_longitude,
             presence_latitude);
    free(presence_username);
  } else if (packet->id == 86) {
    // RESTART
    int32_t ms = read_int(packet);
    debugLog("Restart: %d ms\n", ms);
  } else if (packet->id == 88) {
    // MATCH_INVITE
    char *sender = read_string(packet);
    char *text = read_string(packet);
    char *recipient = read_string(packet);
    int32_t sender_id = read_int(packet);
    debugLog("(match invite) %s (%s): %s\n", sender, recipient, text);
    free(sender);
    free(text);
    free(recipient);
  } else if (packet->id == 89) {
    // XXX: handle CHANNEL_INFO_END
  } else if (packet->id == 91) {
    // MATCH_CHANGE_PASSWORD
    char *new_password = read_string(packet);
    debugLog("Room changed password to %s\n", new_password);
    free(new_password);
  } else if (packet->id == 92) {
    // SILENCE_END
    int32_t delta = read_int(packet);
    debugLog("Silence ends in %d seconds.\n", delta);
  } else if (packet->id == 94) {
    // USER_SILENCED
    int32_t user_id = read_int(packet);
    debugLog("User #%d silenced.\n", user_id);
  } else if (packet->id == 100) {
    // USER_DM_BLOCKED
    char *_1 = read_string(packet);
    char *_2 = read_string(packet);
    char *blocked = read_string(packet);
    int32_t _3 = read_int(packet);
    debugLog("Blocked %s.\n", blocked);
    free(_1);
    free(_2);
    free(blocked);
  } else if (packet->id == 101) {
    // TARGET_IS_SILENCED
    char *_1 = read_string(packet);
    char *_2 = read_string(packet);
    char *blocked = read_string(packet);
    int32_t _3 = read_int(packet);
    debugLog("Silenced %s.\n", blocked);
    free(_1);
    free(_2);
    free(blocked);
  } else if (packet->id == 102) {
    // VERSION_UPDATE_FORCED
    bancho.user_id = 0;
    bancho.osu->getNotificationOverlay()->addNotification(
        "Server uses an unsupported protocol version.");
  } else if (packet->id == 103) {
    // SWITCH_SERVER
    int32_t idle_time = read_int(packet);
    // XXX: handle this
  } else if (packet->id == 104) {
    // ACCOUNT_RESTRICTED
    debugLog("ACCOUNT RESTRICTED.\n");
  } else if (packet->id == 106) {
    // MATCH_ABORT
    debugLog("Match aborted!\n");
  } else {
    debugLog("Unknown packet ID %d (%d bytes)!\n", packet->id, packet->size);
  }
}

Packet build_login_packet(char *username, char *password) {
  // Request format:
  // username\npasswd_md5\nosu_version|utc_offset|display_city|client_hashes|pm_private\n
  Packet packet = {0};

  write_bytes(&packet, (uint8_t *)username, strlen(username));
  write_byte(&packet, '\n');

  std::string password_md5 = md5((uint8_t *)password, strlen(password));
  write_bytes(&packet, (uint8_t *)password_md5.c_str(), password_md5.size());
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
  std::string osu_path_md5 = md5((uint8_t *)osu_path, strlen(osu_path));
  write_bytes(&packet, (uint8_t *)osu_path_md5.c_str(), osu_path_md5.size());
  write_byte(&packet, ':');

  // XXX: Should get MAC addresses from network adapters
  const char *adapters = "runningunderwine";
  std::string adapters_md5 = md5((uint8_t *)adapters, strlen(adapters));
  write_bytes(&packet, (uint8_t *)adapters, strlen(adapters));
  write_byte(&packet, ':');
  write_bytes(&packet, (uint8_t *)adapters_md5.c_str(), adapters_md5.size());
  write_byte(&packet, ':');

  char *uuid = get_disk_uuid();
  std::string disk_md5 = "00000000000000000000000000000000";
  if (uuid) {
    disk_md5 = md5((uint8_t *)uuid, strlen(uuid));
    free(uuid);
  }

  // XXX: Should be per-install unique ID.
  // I'm lazy so just sending disk signature twice.
  write_bytes(&packet, (uint8_t *)disk_md5.c_str(), disk_md5.size());
  write_byte(&packet, ':');

  write_bytes(&packet, (uint8_t *)disk_md5.c_str(), disk_md5.size());
  write_byte(&packet, ':');

  // Allow PMs from strangers
  write_byte(&packet, '|');
  write_byte(&packet, '0');

  write_byte(&packet, '\n');
  return packet;
}

// void send_stuff() {
//   {
//     // CHANGE_ACTION
//     enum Action action = Testing;
//     char *status = "hmmmmmmmm...";
//     char *map_md5 = "real map md5 trust me";
//     int32_t map_id = 42;
//     uint32_t mods = 0;
//     uint8_t mode = 0;

//     Packet packet = {0};
//     write_header(&packet, 0);
//     write_byte(&packet, action);
//     write_string(&packet, status);
//     write_string(&packet, map_md5);
//     write_int(&packet, mods);
//     write_byte(&packet, mode);
//     write_int(&packet, map_id);

//     send_packet(&packet);
//   }

//   {
//     // CHANNEL_JOIN
//     Packet packet = {0};
//     write_header(&packet, 63);
//     write_string(&packet, "#osu");
//     send_packet(&packet);
//   }

//   {
//     // SEND_PUBLIC_MESSAGE
//     Packet packet = {0};
//     write_header(&packet, 1);
//     write_string(&packet, "kiwec");
//     write_string(&packet, "Hello world!");
//     write_string(&packet, "#osu");
//     write_int(&packet, 3);

//     send_packet(&packet);
//   }

//   {
//     // PING
//     Packet packet = {0};
//     write_header(&packet, 4);
//     send_packet(&packet);
//   }
// }

// int main() {
//   Packet login =
//       build_login_packet(getenv("OSU_USERNAME"), getenv("OSU_PASSWORD"));
//   send_packet(&login);

//   if (bancho.user_id == 0) {
//     debugLog(stderr, "Failed to login: %s\n", cho_token);
//   } else {
//     debugLog("Logged in successfully as user id %d!\n", bancho.user_id);
//     debugLog("Login token: %s\n", cho_token);
//   }

//   send_stuff();

//   return 0;
// }
