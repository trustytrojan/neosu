#ifdef _WIN32
#include <windows.h>
#include <wbemidl.h>
#include <stdio.h>
#include <comutil.h>
#else
#include <blkid/blkid.h>
#include <linux/limits.h>
#endif

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
#include "OsuUIAvatar.h"
#include "OsuUIButton.h"
#include "OsuUISongBrowserUserButton.h"


Bancho bancho;
std::unordered_map<std::string, Channel*> chat_channels;
bool print_new_channels = true;

void update_channel(UString name, UString topic, int32_t nb_members) {
  Channel* chan;
  auto name_str = std::string(name.toUtf8());
  auto it = chat_channels.find(name_str);
  if(it == chat_channels.end()) {
    chan = new Channel();
    chan->name = name;
    chat_channels[name_str] = chan;

    if(print_new_channels) {
      bancho.osu->m_chat->addMessage("#osu", ChatMessage{
        .tms = time(NULL),
        .author_id = 0,
        .author_name = UString(""),
        .text = UString::format("%s (%d): %s", name.toUtf8(), nb_members, topic.toUtf8()),
      });
    }
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

UString get_disk_uuid() {
#ifdef _WIN32
  // ChatGPT'd, this looks absolutely insane but might just be regular Windows API...
  CoInitializeEx(0, COINIT_MULTITHREADED);
  CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

  IWbemLocator* pLoc = NULL;
  auto hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
  if (FAILED(hres)) {
    debugLog("Failed to create IWbemLocator object. Error code = 0x%x\n", hres);
    return UString("error getting disk uuid");
  }

  IWbemServices* pSvc = NULL;
  BSTR bstr_root = SysAllocString(L"ROOT\\CIMV2");
  hres = pLoc->ConnectServer(bstr_root, NULL, NULL, 0, 0, 0, 0, &pSvc);
  if (FAILED(hres)) {
    debugLog("Could not connect. Error code = 0x%x\n", hres);
    pLoc->Release();
    SysFreeString(bstr_root);
    return UString("error getting disk uuid");
  }
  SysFreeString(bstr_root);

  hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
  if (FAILED(hres)) {
    debugLog("Could not set proxy blanket. Error code = 0x%x\n", hres);
    pSvc->Release();
    pLoc->Release();
    return UString("error getting disk uuid");
  }

  UString uuid = "";
  IEnumWbemClassObject* pEnumerator = NULL;
  BSTR bstr_wql = SysAllocString(L"WQL");
  BSTR bstr_sql = SysAllocString(L"SELECT * FROM Win32_DiskDrive");
  hres = pSvc->ExecQuery(bstr_wql, bstr_sql, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
  if (FAILED(hres)) {
    debugLog("Query for hard drive UUID failed. Error code = 0x%x\n", hres);
    pSvc->Release();
    pLoc->Release();
    SysFreeString(bstr_wql);
    SysFreeString(bstr_sql);
    return UString("error getting disk uuid");
  }
  SysFreeString(bstr_wql);
  SysFreeString(bstr_sql);

  IWbemClassObject* pclsObj = NULL;
  ULONG uReturn = 0;
  while (pEnumerator && uuid.length() == 0) {
    HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
    if (0 == uReturn) {
      break;
    }

    VARIANT vtProp;
    hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
    if (SUCCEEDED(hr)) {
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
  while (!blkid_dev_next(iter, &device)) {
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

void handle_packet(Packet *packet) {
  if (packet->id == USER_ID) {
    bancho.user_id = read_int(packet);
    if(bancho.user_id > 0) {
      debugLog("Logged in as user #%d.\n", bancho.user_id);
      bancho.osu->m_optionsMenu->logInButton->setText("Disconnect");
      bancho.osu->m_optionsMenu->logInButton->setColor(0xffff0000);
      bancho.osu->m_optionsMenu->logInButton->is_loading = false;
      convar->getConVarByName("mp_autologin")->setValue(true);
      ConVars::sv_cheats.setValue(false);
      print_new_channels = true;

      auto avatar_dir = UString::format(MCENGINE_DATA_DIR "avatars/%s", bancho.endpoint.toUtf8());
      if(!env->directoryExists(avatar_dir)) {
        env->createDirectory(avatar_dir);
      }

      // close your eyes
      SAFE_DELETE(bancho.osu->m_songBrowser2->m_userButton->m_avatar);
      bancho.osu->m_songBrowser2->m_userButton->m_avatar = new OsuUIAvatar(bancho.user_id, 0.f, 0.f, 0.f, 0.f);
      bancho.osu->m_songBrowser2->m_userButton->m_avatar->on_screen = true;

      // XXX: We should toggle between "offline" sorting options and "online" ones
      //      Online ones would be "Local scores", "Global", "Country", "Selected mods" etc
      //      While offline ones would be "By score", "By pp", etc
      bancho.osu->m_songBrowser2->onSortScoresChange(UString("Online Leaderboard"), 0);
    } else {
      convar->getConVarByName("mp_autologin")->setValue(false);
      bancho.osu->m_optionsMenu->logInButton->setText("Log in");
      bancho.osu->m_optionsMenu->logInButton->setColor(0xff00ff00);
      bancho.osu->m_optionsMenu->logInButton->is_loading = false;

      debugLog("Failed to log in, server returned code %d.\n", bancho.user_id);
      UString errmsg = UString::format("Failed to log in: %s (code %d)\n", cho_token.toUtf8(), bancho.user_id);
      if(bancho.user_id == -2) {
        errmsg = "Client version is too old to connect to this server.";
      } else if(bancho.user_id == -3 || bancho.user_id == -4) {
        errmsg = "You are banned from this server.";
      } else if(bancho.user_id == -6) {
        errmsg = "You need to buy supporter to connect to this server.";
      } else if(bancho.user_id == -7) {
        errmsg = "You need to reset your password to connect to this server.";
      } else if(bancho.user_id == -8) {
        errmsg = "Open the verification link sent to your email, then log in again.";
      } else {
        if(cho_token == UString("user-already-logged-in")) {
          errmsg = "Already logged in on another client.";
        } else if(cho_token == UString("unknown-username")) {
          errmsg = UString::format("No account by the username '%s' exists.", bancho.username.toUtf8());
        } else if(cho_token == UString("incorrect-password")) {
          errmsg = "Incorrect password.";
        } else if(cho_token == UString("contact-staff")) {
          errmsg = "Please contact an administrator of the server.";
        }
      }
      bancho.osu->getNotificationOverlay()->addNotification(errmsg);
    }
  } else if (packet->id == RECV_MESSAGE) {
    UString sender = read_string(packet);
    UString text = read_string(packet);
    UString recipient = read_string(packet);
    int32_t sender_id = read_int(packet);

    bancho.osu->m_chat->addMessage(recipient, ChatMessage{
      .tms = time(NULL),
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
    user->mods = read_int(packet);
    user->mode = (GameMode)read_byte(packet);
    user->map_id = read_int(packet);
    user->ranked_score = read_int64(packet);
    user->accuracy = read_float32(packet);
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
  } else if (packet->id == SPECTATOR_LEFT) {
    int32_t spectator_id = read_int(packet);
    debugLog("Spectator left: user id %d\n", spectator_id);
  } else if (packet->id == VERSION_UPDATE) {
    disconnect();
    bancho.osu->getNotificationOverlay()->addNotification(
        "Server uses an unsupported protocol version.");
  } else if (packet->id == SPECTATOR_CANT_SPECTATE) {
    int32_t spectator_id = read_int(packet);
    debugLog("Spectator can't spectate: user id %d\n", spectator_id);
  } else if (packet->id == GET_ATTENTION) {
    // (nothing to do)
  } else if (packet->id == NOTIFICATION) {
    UString notification = read_string(packet);
    bancho.osu->getNotificationOverlay()->addNotification(notification);
    // XXX: don't do McOsu style notifications, since:
    // 1) they can't do multiline text
    // 2) they don't stack, if the server sends >1 you only see the latest
    // Maybe log them in the chat + display in bottom right like ppy client?
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
    // (nothing to do)
  } else if (packet->id == MATCH_ALL_PLAYERS_LOADED) {
    bancho.osu->m_room->on_all_players_loaded();
  } else if (packet->id == MATCH_PLAYER_FAILED) {
    int32_t slot_id = read_int(packet);
    bancho.osu->m_room->on_player_failed(slot_id);
  } else if (packet->id == MATCH_FINISHED) {
    bancho.osu->m_room->on_match_finished();
  } else if (packet->id == MATCH_SKIP) {
    bancho.osu->m_room->on_all_players_skipped();
  } else if (packet->id == CHANNEL_JOIN_SUCCESS) {
    UString name = read_string(packet);
    bancho.osu->m_chat->addChannel(name);
    bancho.osu->m_chat->addMessage(name, ChatMessage{
      .tms = time(NULL),
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
  } else if (packet->id == FRIENDS_LIST) {
    uint16_t nb_friends = read_short(packet);
    for(int i = 0; i < nb_friends; i++) {
      auto user = get_user_info(read_int(packet));
      user->is_friend = true;
    }
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
  } else if (packet->id == MATCH_PLAYER_SKIPPED) {
    int32_t user_id = read_int(packet);
    bancho.osu->m_room->on_player_skip(user_id);

    // I'm not sure the server ever sends MATCH_SKIP... So, double-checking here.
    bool all_players_skipped = true;
    for(int i = 0; i < 16; i++) {
      if(bancho.room.slots[i].is_player_playing()) {
        if(!bancho.room.slots[i].skipped) {
          all_players_skipped = false;
        }
      }
    }
    if(all_players_skipped) {
      bancho.osu->m_room->on_all_players_skipped();
    }
  } else if (packet->id == USER_PRESENCE) {
    int32_t presence_user_id = read_int(packet);
    UString presence_username = read_string(packet);

    UserInfo *user = get_user_info(presence_user_id);
    user->name = presence_username;
    user->utc_offset = read_byte(packet);
    user->country = read_byte(packet);
    user->privileges = read_byte(packet);
    user->longitude = read_float32(packet);
    user->latitude = read_float32(packet);
    user->global_rank = read_int(packet);
  } else if (packet->id == RESTART) {
    // XXX: wait 'ms' milliseconds before reconnecting
    int32_t ms = read_int(packet);
    (void)ms;

    // Some servers send "restart" packets when password is incorrect
    // So, don't retry unless actually logged in
    if(bancho.is_online()) {
      reconnect();
    }
  } else if (packet->id == MATCH_INVITE) {
    UString sender = read_string(packet);
    UString text = read_string(packet);
    UString recipient = read_string(packet);
    (void)recipient;
    int32_t sender_id = read_int(packet);
    bancho.osu->m_chat->addMessage(recipient, ChatMessage{
      .tms = time(NULL),
      .author_id = sender_id,
      .author_name = sender,
      .text = text,
    });
  } else if (packet->id == CHANNEL_INFO_END) {
    print_new_channels = false;
    bancho.osu->m_chat->join("#announce");
    bancho.osu->m_chat->join("#osu");
  } else if (packet->id == ROOM_PASSWORD_CHANGED) {
    UString new_password = read_string(packet);
    debugLog("Room changed password to %s\n", new_password.toUtf8());
    bancho.room.password = new_password;
  } else if (packet->id == SILENCE_END) {
    int32_t delta = read_int(packet);
    debugLog("Silence ends in %d seconds.\n", delta);
    // XXX: Prevent user from sending messages while silenced
  } else if (packet->id == USER_SILENCED) {
    int32_t user_id = read_int(packet);
    debugLog("User #%d silenced.\n", user_id);
  } else if (packet->id == USER_DM_BLOCKED) {
    read_string(packet);
    read_string(packet);
    UString blocked = read_string(packet);
    read_int(packet);
    debugLog("Blocked %s.\n", blocked.toUtf8());
  } else if (packet->id == TARGET_IS_SILENCED) {
    read_string(packet);
    read_string(packet);
    UString blocked = read_string(packet);
    read_int(packet);
    debugLog("Silenced %s.\n", blocked.toUtf8());
  } else if (packet->id == VERSION_UPDATE_FORCED) {
    disconnect();
    bancho.osu->getNotificationOverlay()->addNotification(
        "Server uses an unsupported protocol version.");
  } else if (packet->id == ACCOUNT_RESTRICTED) {
    bancho.osu->getNotificationOverlay()->addNotification("Account restricted.");
    disconnect();
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

  write_bytes(&packet, (uint8_t *)bancho.username.toUtf8(), bancho.username.lengthUtf8());
  write_byte(&packet, '\n');

  write_bytes(&packet, (uint8_t *)bancho.pw_md5.toUtf8(), bancho.pw_md5.lengthUtf8());
  write_byte(&packet, '\n');

  write_bytes(&packet, (uint8_t *)OSU_VERSION, strlen(OSU_VERSION));
  write_byte(&packet, '|');

  // UTC offset
  time_t now = time(NULL);
  auto gmt = gmtime(&now);
  auto local_time = localtime(&now);
  int utc_offset = difftime(mktime(local_time), mktime(gmt)) / 3600;
  if(utc_offset < 0) {
    write_byte(&packet, '-');
    utc_offset *= -1;
  }
  write_byte(&packet, '0' + utc_offset);
  write_byte(&packet, '|');

  // Don't dox the user's city
  write_byte(&packet, '0');
  write_byte(&packet, '|');

  char osu_path[PATH_MAX] = {0};
#ifdef _WIN32
  GetModuleFileName(NULL, osu_path, PATH_MAX);
#else
  readlink("/proc/self/exe", osu_path, PATH_MAX - 1);
#endif

  UString osu_path_md5 = md5((uint8_t *)osu_path, strlen(osu_path));

  // XXX: Should get MAC addresses from network adapters
  // NOTE: Not sure how the MD5 is computed - does it include final "." ?
  const char *adapters = "runningunderwine";
  UString adapters_md5 = md5((uint8_t *)adapters, strlen(adapters));

  // XXX: Should remove '|' from the disk UUID just to be safe
  bancho.disk_uuid = get_disk_uuid();
  UString disk_md5 = md5((uint8_t *)bancho.disk_uuid.toUtf8(), bancho.disk_uuid.lengthUtf8());

  // XXX: Not implemented, I'm lazy so just reusing disk signature
  bancho.install_id = bancho.disk_uuid;
  UString install_md5 = md5((uint8_t *)bancho.install_id.toUtf8(), bancho.install_id.lengthUtf8());;

  bancho.client_hashes = UString::format(
    "%s:%s.:%s:%s:%s:",
    osu_path_md5.toUtf8(),
    adapters,
    adapters_md5.toUtf8(),
    install_md5.toUtf8(),
    disk_md5.toUtf8()
  );
  write_bytes(&packet, (uint8_t *)bancho.client_hashes.toUtf8(), bancho.client_hashes.lengthUtf8());

  // Allow PMs from strangers
  write_byte(&packet, '|');
  write_byte(&packet, '0');

  write_byte(&packet, '\n');
  return packet;
}
