#include "Lobby.h"

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoUsers.h"
#include "CBaseUIButton.h"
#include "CBaseUIContainer.h"
#include "CBaseUILabel.h"
#include "Chat.h"
#include "Engine.h"
#include "Keyboard.h"
#include "MainMenu.h"
#include "NotificationOverlay.h"
#include "Osu.h"
#include "PromptScreen.h"
#include "ResourceManager.h"
#include "RichPresence.h"
#include "UIButton.h"

RoomUIElement::RoomUIElement(Lobby* multi, Room* room, float x, float y, float width, float height)
    : CBaseUIScrollView(x, y, width, height, "") {
    // NOTE: We can't store the room pointer, since it might expire later
    m_multi = multi;
    room_id = room->id;
    has_password = room->has_password;

    setBlockScrolling(true);
    setDrawFrame(true);

    float title_width = multi->font->getStringWidth(room->name) + 20;
    auto title_ui = new CBaseUILabel(10, 5, title_width, 30, "", room->name);
    title_ui->setDrawFrame(false);
    title_ui->setDrawBackground(false);
    getContainer()->addBaseUIElement(title_ui);

    char player_count_str[256] = {0};
    snprintf(player_count_str, 255, "Players: %d/%d", room->nb_players, room->nb_open_slots);
    float player_count_width = multi->font->getStringWidth(player_count_str) + 20;
    auto slots_ui = new CBaseUILabel(10, 33, player_count_width, 30, "", UString(player_count_str));
    slots_ui->setDrawFrame(false);
    slots_ui->setDrawBackground(false);
    getContainer()->addBaseUIElement(slots_ui);

    join_btn = new UIButton(multi->m_osu, 10, 65, 120, 30, "", "Join room");
    join_btn->setUseDefaultSkin();
    join_btn->setColor(0xff00ff00);
    join_btn->setClickCallback(fastdelegate::MakeDelegate(this, &RoomUIElement::onRoomJoinButtonClick));
    getContainer()->addBaseUIElement(join_btn);

    if(room->has_password) {
        auto pwlabel = new CBaseUILabel(135, 64, 150, 30, "", "(password required)");
        pwlabel->setDrawFrame(false);
        pwlabel->setDrawBackground(false);
        getContainer()->addBaseUIElement(pwlabel);
    }
}

void RoomUIElement::onRoomJoinButtonClick(CBaseUIButton* btn) {
    if(has_password) {
        m_multi->room_to_join = room_id;
        m_multi->m_osu->m_prompt->prompt("Room password:",
                                         fastdelegate::MakeDelegate(m_multi, &Lobby::on_room_join_with_password));
    } else {
        m_multi->joinRoom(room_id, "");
    }
}

Lobby::Lobby(Osu* osu) : OsuScreen(osu) {
    font = engine->getResourceManager()->getFont("FONT_DEFAULT");

    auto heading = new CBaseUILabel(50, 30, 300, 40, "", "Multiplayer rooms");
    heading->setFont(m_osu->getTitleFont());
    heading->setSizeToContent(0, 0);
    heading->setDrawFrame(false);
    heading->setDrawBackground(false);
    addBaseUIElement(heading);

    m_create_room_btn = new UIButton(osu, 0, 0, 200, 50, "", "Create new room");
    m_create_room_btn->setUseDefaultSkin();
    m_create_room_btn->setColor(0xff00ff00);
    m_create_room_btn->setClickCallback(fastdelegate::MakeDelegate(this, &Lobby::on_create_room_clicked));
    addBaseUIElement(m_create_room_btn);

    m_list = new CBaseUIScrollView(0, 0, 0, 0, "");
    m_list->setDrawFrame(false);
    m_list->setDrawBackground(true);
    m_list->setBackgroundColor(0xdd000000);
    m_list->setHorizontalScrolling(false);
    addBaseUIElement(m_list);

    updateLayout(m_osu->getScreenSize());
}

void Lobby::onKeyDown(KeyboardEvent& key) {
    if(!m_bVisible) return;

    if(key.getKeyCode() == KEY_ESCAPE) {
        key.consume();
        setVisible(false);
        m_osu->m_mainMenu->setVisible(true);
        return;
    }

    // XXX: search bar
}

void Lobby::onKeyUp(KeyboardEvent& key) {
    if(!m_bVisible) return;

    // XXX: search bar
}

void Lobby::onChar(KeyboardEvent& key) {
    if(!m_bVisible) return;

    // XXX: search bar
}

void Lobby::onResolutionChange(Vector2 newResolution) { updateLayout(newResolution); }

CBaseUIContainer* Lobby::setVisible(bool visible) {
    if(visible == m_bVisible) return this;
    m_bVisible = visible;

    if(visible) {
        Packet packet;
        packet.id = JOIN_ROOM_LIST;
        send_packet(packet);

        packet = Packet();
        packet.id = CHANNEL_JOIN;
        write_string(&packet, "#lobby");
        send_packet(packet);

        // LOBBY presence is broken so we send MULTIPLAYER
        RichPresence::setBanchoStatus(m_osu, "Looking to play", MULTIPLAYER);

        // XXX: Could call refreshBeatmaps() here so we load them if not already done so.
        //      Would need to edit it a bit to work outside of songBrowser2, + display loading progress.
        //      Ideally, you'd do this in the background and still be able to browse rooms.
    } else {
        Packet packet;
        packet.id = EXIT_ROOM_LIST;
        send_packet(packet);

        packet = Packet();
        packet.id = CHANNEL_PART;
        write_string(&packet, "#lobby");
        send_packet(packet);

        for(auto room : rooms) {
            delete room;
        }
        rooms.clear();
    }

    m_osu->m_chat->updateVisibility();
    return this;
}

void Lobby::updateLayout(Vector2 newResolution) {
    setSize(newResolution);

    m_list->clear();
    m_list->setPos(round(newResolution.x * 0.6), 0);
    m_list->setSize(round(newResolution.x * 0.4), newResolution.y);

    if(rooms.empty()) {
        auto noRoomsOpenElement = new CBaseUILabel(0, 0, 0, 0, "", "There are no matches available.");
        noRoomsOpenElement->setTextJustification(CBaseUILabel::TEXT_JUSTIFICATION::TEXT_JUSTIFICATION_CENTERED);
        noRoomsOpenElement->setSizeToContent(20, 20);
        noRoomsOpenElement->setPos(m_list->getSize().x / 2 - noRoomsOpenElement->getSize().x / 2,
                                   m_list->getSize().y / 2 - noRoomsOpenElement->getSize().y / 2);
        m_list->getContainer()->addBaseUIElement(noRoomsOpenElement);
    }

    float heading_ratio = 70 / newResolution.y;
    float chat_ratio = 0.3;
    float free_ratio = 1.f - (heading_ratio + chat_ratio);
    m_create_room_btn->setPos(round(newResolution.x * 0.3) - m_create_room_btn->getSize().x / 2,
                              70 + round(newResolution.y * free_ratio / 2) - m_create_room_btn->getSize().y / 2);

    float y = 10;
    const float room_height = 105;
    for(auto room : rooms) {
        auto room_ui = new RoomUIElement(this, room, 10, y, m_list->getSize().x - 20, room_height);
        m_list->getContainer()->addBaseUIElement(room_ui);
        y += room_height + 20;
    }

    m_list->setScrollSizeToContent();
}

void Lobby::addRoom(Room* room) {
    rooms.push_back(room);
    updateLayout(getSize());
}

void Lobby::joinRoom(uint32_t id, UString password) {
    Packet packet;
    packet.id = JOIN_ROOM;
    write_int32(&packet, id);
    write_string(&packet, password.toUtf8());
    send_packet(packet);

    for(CBaseUIElement* elm : m_list->getContainer()->getElements()) {
        auto room = (RoomUIElement*)elm;
        if(room->room_id != id) continue;
        room->join_btn->is_loading = true;
        break;
    }

    m_osu->getNotificationOverlay()->addNotification("Joining room...");
}

void Lobby::updateRoom(Room room) {
    for(auto old_room : rooms) {
        if(old_room->id == room.id) {
            *old_room = room;
            updateLayout(getSize());
            return;
        }
    }

    // On bancho.py, if a player creates a room when we're already in the lobby,
    // we won't receive a ROOM_CREATED but only a ROOM_UPDATED packet.
    auto new_room = new Room();
    *new_room = room;
    addRoom(new_room);
}

void Lobby::removeRoom(uint32_t room_id) {
    for(auto room : rooms) {
        if(room->id == room_id) {
            auto it = std::find(rooms.begin(), rooms.end(), room);
            rooms.erase(it);
            delete room;
            break;
        }
    }

    updateLayout(getSize());
}

void Lobby::on_create_room_clicked() {
    bancho.room = Room();
    bancho.room.name = "New room";  // XXX: doesn't work
    bancho.room.host_id = bancho.user_id;
    for(int i = 0; i < 16; i++) {
        bancho.room.slots[i].status = 1;  // open slot
    }
    bancho.room.slots[0].status = 4;  // not ready
    bancho.room.slots[0].player_id = bancho.user_id;

    Packet packet = {0};
    packet.id = CREATE_ROOM;
    bancho.room.pack(&packet);
    send_packet(packet);

    m_osu->getNotificationOverlay()->addNotification("Creating room...");
}

void Lobby::on_room_join_with_password(UString password) { joinRoom(room_to_join, password); }

void Lobby::on_room_join_failed() {
    // Updating layout will reset is_loading to false
    updateLayout(getSize());
}
