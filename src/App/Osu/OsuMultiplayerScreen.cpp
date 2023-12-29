#include "BanchoNetworking.h"
#include "BanchoUsers.h"
#include "Osu.h"
#include "OsuMainMenu.h"
#include "OsuMultiplayerScreen.h"

#include "CBaseUIContainer.h"
#include "Engine.h"
#include "Keyboard.h"
#include "ResourceManager.h"


OsuMultiplayerScreen::OsuMultiplayerScreen(Osu *osu) : OsuScreen(osu) {
    font = engine->getResourceManager()->getFont("FONT_DEFAULT");
    m_container = new CBaseUIContainer(0, 0, 0, 0, "");
    updateLayout(m_osu->getScreenSize());
}

void OsuMultiplayerScreen::draw(Graphics *g) {
    if (!m_bVisible) return;

    m_container->draw(g);
}

void OsuMultiplayerScreen::update() {
    if (!m_bVisible) return;
    m_container->update();
}

void OsuMultiplayerScreen::onKeyDown(KeyboardEvent &key) {
    if(!m_bVisible) return;

    if(key.getKeyCode() == KEY_ESCAPE) {
        key.consume();
        setVisible(false);
        return;
    }

    // XXX: search bar
}

void OsuMultiplayerScreen::onKeyUp(KeyboardEvent &key) {
    if(!m_bVisible) return;

    // XXX: search bar
}

void OsuMultiplayerScreen::onChar(KeyboardEvent &key) {
    if(!m_bVisible) return;

    // XXX: search bar
}

void OsuMultiplayerScreen::onResolutionChange(Vector2 newResolution) {
    updateLayout(newResolution);
}

void OsuMultiplayerScreen::setVisible(bool visible) {
    if(visible == m_bVisible) return;
    m_bVisible = visible;

    if(visible) {
        Packet packet = {0};
        packet.id = JOIN_ROOM_LIST;
        send_packet(packet);

        packet = {0};
        packet.id = CHANNEL_JOIN;
        write_string(&packet, "#lobby");
        send_packet(packet);
    } else {
        Packet packet = {0};
        packet.id = EXIT_ROOM_LIST;
        send_packet(packet);

        packet = {0};
        packet.id = CHANNEL_PART;
        write_string(&packet, "#lobby");
        send_packet(packet);

        for(auto room : rooms) {
            free(room);
        }
        rooms.clear();

        m_osu->m_mainMenu->setVisible(true);
    }
}

void OsuMultiplayerScreen::updateLayout(Vector2 newResolution) {
    // TODO @kiwec
}

void debug_print_room(Room* room) {
    debugLog("Room %d:\n", room->id);

    for(int i = 0; i < 16; i++) {
        const char* status = "(wtf)";
        if(room->slots[i].status & 0b00000001) {
            status = "(open)";
        } else if(room->slots[i].status & 0b00000010) {
            status = "(locked)";
        } else if(room->slots[i].status & 0b00000100) {
            status = "(not ready)";
        } else if(room->slots[i].status & 0b00001000) {
            status = "(ready)";
        } else if(room->slots[i].status & 0b00010000) {
            status = "(no map)";
        } else if(room->slots[i].status & 0b00100000) {
            status = "(playing)";
        } else if(room->slots[i].status & 0b01000000) {
            status = "(finished)";
        } else if(room->slots[i].status & 0b10000000) {
            status = "(quit)";
        }

        if(room->slots[i].player_id > 0) {
            UserInfo *user = get_user_info(room->slots[i].player_id);
            debugLog("- %s (rank #%d) %s\n", user->name.toUtf8(), user->global_rank, status);
        }
    }
}

void OsuMultiplayerScreen::addRoom(Room* room) {
    rooms.push_back(room);
    updateLayout(m_container->getSize());
    debug_print_room(room);
}

void OsuMultiplayerScreen::updateRoom(Room* room) {
    // Yes, we replace the room just like that. Don't hold pointers, they will expire.
    for(auto old_room : rooms) {
        if(old_room->id == room->id) {
            auto it = std::find(rooms.begin(), rooms.end(), old_room);
            rooms.erase(it);
            free_room(old_room);
            rooms.push_back(room);
            debug_print_room(room);
            return;
        }
    }

    // On bancho.py, if a player creates a room when we're already in the lobby,
    // we won't receive a ROOM_CREATED but only a ROOM_UPDATED packet.
    addRoom(room);
}

void OsuMultiplayerScreen::removeRoom(uint32_t room_id) {
    for(auto room : rooms) {
      if(room->id == room_id) {
        auto it = std::find(rooms.begin(), rooms.end(), room);
        rooms.erase(it);
        free_room(room);
        break;
      }
    }

    updateLayout(m_container->getSize());
}
