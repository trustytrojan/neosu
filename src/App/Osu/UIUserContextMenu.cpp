#include "UIUserContextMenu.h"

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoUsers.h"
#include "Chat.h"
#include "Engine.h"
#include "Mouse.h"
#include "NotificationOverlay.h"
#include "Osu.h"
#include "UIContextMenu.h"

UIUserContextMenuScreen::UIUserContextMenuScreen(Osu *osu) : OsuScreen(osu) {
    m_bVisible = true;
    menu = new UIContextMenu(osu);
    addBaseUIElement(menu);
}

void UIUserContextMenuScreen::onResolutionChange(Vector2 newResolution) {
    setSize(newResolution);
    OsuScreen::onResolutionChange(newResolution);
}

void UIUserContextMenuScreen::stealFocus() {
    OsuScreen::stealFocus();
    close();
}

void UIUserContextMenuScreen::open(u32 user_id) {
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

        if(user_info->is_friend()) {
            menu->addButton("Revoke friendship", UA_REMOVE_FRIEND);
        } else {
            menu->addButton("Add as friend", UA_ADD_FRIEND);
        }

        // XXX: Not implemented
        // menu->addButton("Spectate", TOGGLE_SPECTATE);
    }

    menu->end(false, false);
    menu->setPos(engine->getMouse()->getPos());
    menu->setClickCallback(fastdelegate::MakeDelegate(this, &UIUserContextMenuScreen::on_action));
    menu->setVisible(true);
}

void UIUserContextMenuScreen::close() { menu->setVisible(false); }

void UIUserContextMenuScreen::on_action(UString text, int user_action) {
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
        Packet packet;
        packet.id = TRANSFER_HOST;
        write_u32(&packet, slot_number);
        send_packet(packet);
    } else if(user_action == KICK) {
        Packet packet;
        packet.id = MATCH_LOCK;
        write_u32(&packet, slot_number);
        send_packet(packet);  // kick by locking the slot
        send_packet(packet);  // unlock the slot
    } else if(user_action == START_CHAT) {
        m_osu->m_chat->addChannel(user_info->name, true);
    } else if(user_action == VIEW_PROFILE) {
        auto url = UString::format("https://%s/u/%d", bancho.endpoint.toUtf8(), m_user_id);
        m_osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
        env->openURLInDefaultBrowser(url.toUtf8());
    } else if(user_action == UA_ADD_FRIEND) {
        Packet packet;
        packet.id = FRIEND_ADD;
        write_u32(&packet, m_user_id);
        send_packet(packet);
        friends.push_back(m_user_id);
    } else if(user_action == UA_REMOVE_FRIEND) {
        Packet packet;
        packet.id = FRIEND_REMOVE;
        write_u32(&packet, m_user_id);
        send_packet(packet);

        auto it = std::find(friends.begin(), friends.end(), m_user_id);
        if(it != friends.end()) {
            friends.erase(it);
        }
    }

    menu->setVisible(false);
}

UIUserLabel::UIUserLabel(Osu *osu, u32 user_id, UString username) : CBaseUILabel() {
    m_user_id = user_id;
    setText(username);
    setDrawFrame(false);
    setDrawBackground(false);
}

void UIUserLabel::onMouseUpInside() { bancho.osu->m_user_actions->open(m_user_id); }
