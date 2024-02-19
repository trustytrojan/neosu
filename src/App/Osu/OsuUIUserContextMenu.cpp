#include "OsuUIUserContextMenu.h"

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoUsers.h"
#include "Osu.h"
#include "OsuChat.h"
#include "OsuNotificationOverlay.h"
#include "OsuUIContextMenu.h"

#include "Engine.h"
#include "Mouse.h"


OsuUIUserContextMenuScreen::OsuUIUserContextMenuScreen(Osu *osu) : OsuScreen(osu) {
    m_bVisible = true;
    menu = new OsuUIContextMenu(osu);
    addBaseUIElement(menu);
}

void OsuUIUserContextMenuScreen::onResolutionChange(Vector2 newResolution) {
    setSize(newResolution);
    OsuScreen::onResolutionChange(newResolution);
}

void OsuUIUserContextMenuScreen::stealFocus() {
    OsuScreen::stealFocus();
    close();
}

void OsuUIUserContextMenuScreen::open(uint32_t user_id) {
    if(!bancho.is_online()) return;

    close();
    m_user_id = user_id;

    auto user_info = get_user_info(user_id);
    bool has_user_info = user_info->privileges != 0;

    int slot_number = -1;
    if(bancho.is_in_a_multi_room()) {
        for(int i = 0; i < 16; i++) {
            if(bancho.room.slots[i].player_id == user_id) {
                slot_number = i;
                break;
            }
        }
    }

    menu->begin();

    menu->addButton("View profile", VIEW_PROFILE);

    if(user_id != bancho.user_id) {
        if(bancho.room.is_host() && slot_number != -1) {
            menu->addButton("Set as Host", UA_TRANSFER_HOST);
            menu->addButton("Kick", KICK);
        }

        if(has_user_info) {
            // Without user info, we don't have the username
            menu->addButton("Start Chat", START_CHAT);

            // XXX: Not implemented
            // menu->addButton("Invite to game", INVITE_TO_GAME);
        }

        if(user_info->is_friend) {
            menu->addButton("Revoke friendship", UA_REMOVE_FRIEND);
        } else {
            menu->addButton("Add as friend", UA_ADD_FRIEND);
        }

        // XXX: Not implemented
        // menu->addButton("Spectate", TOGGLE_SPECTATE);
    }

    menu->end(false, false);
    menu->setPos(engine->getMouse()->getPos());
    menu->setClickCallback(fastdelegate::MakeDelegate(this, &OsuUIUserContextMenuScreen::on_action));
    menu->setVisible(true);
}

void OsuUIUserContextMenuScreen::close() {
    menu->setVisible(false);
}

void OsuUIUserContextMenuScreen::on_action(UString text, int user_action) {
    auto user_info = get_user_info(m_user_id);
    int slot_number = -1;
    if(bancho.is_in_a_multi_room()) {
        for(int i = 0; i < 16; i++) {
            if(bancho.room.slots[i].player_id == m_user_id) {
                slot_number = i;
                break;
            }
        }
    }

    if(user_action == UA_TRANSFER_HOST) {
        Packet packet = {0};
        packet.id = TRANSFER_HOST;
        write_int32(&packet, slot_number);
        send_packet(packet);
    } else if(user_action == KICK) {
        Packet packet = {0};
        packet.id = MATCH_LOCK;
        write_int32(&packet, slot_number);
        send_packet(packet); // kick by locking the slot
        send_packet(packet); // unlock the slot
    } else if(user_action == START_CHAT) {
        m_osu->m_chat->addChannel(user_info->name, true);
    } else if(user_action == VIEW_PROFILE) {
        auto url = UString::format("https://%s/u/%d", bancho.endpoint.toUtf8(), m_user_id);
        if (env->getOS() == Environment::OS::OS_HORIZON) {
            m_osu->getNotificationOverlay()->addNotification(url, 0xffffffff, false, 0.75f);
        } else {
            m_osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
            env->openURLInDefaultBrowser(url.toUtf8());
        }
    } else if(user_action == UA_ADD_FRIEND) {
        Packet packet = {0};
        packet.id = FRIEND_ADD;
        write_int32(&packet, m_user_id);
        send_packet(packet);

        user_info->is_friend = true;
    } else if(user_action == UA_REMOVE_FRIEND) {
        Packet packet = {0};
        packet.id = FRIEND_REMOVE;
        write_int32(&packet, m_user_id);
        send_packet(packet);

        user_info->is_friend = false;
    }

    menu->setVisible(false);
}


OsuUIUserLabel::OsuUIUserLabel(Osu *osu, uint32_t user_id, UString username) : CBaseUILabel() {
    m_user_id = user_id;
    setText(username);
    setDrawFrame(false);
    setDrawBackground(false);
}

void OsuUIUserLabel::onMouseUpInside() {
    bancho.osu->m_user_actions->open(m_user_id);
}
