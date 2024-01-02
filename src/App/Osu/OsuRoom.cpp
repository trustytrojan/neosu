#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoUsers.h"
#include "Osu.h"
#include "OsuChat.h"
#include "OsuLobby.h"
#include "OsuMainMenu.h"
#include "OsuRoom.h"
#include "OsuRichPresence.h"
#include "OsuSongBrowser2.h"
#include "OsuUIButton.h"

#include "CBaseUIButton.h"
#include "CBaseUIContainer.h"
#include "CBaseUILabel.h"
#include "Engine.h"
#include "Keyboard.h"
#include "ResourceManager.h"


OsuRoom::OsuRoom(Osu *osu) : OsuScreen(osu) {
    font = engine->getResourceManager()->getFont("FONT_DEFAULT");

    m_container = new CBaseUIContainer(0, 0, 0, 0, "");

    m_slotlist = new CBaseUIScrollView(0, 0, 0, 0, "");
    m_slotlist->setDrawFrame(false);
    m_slotlist->setDrawBackground(true);
    m_slotlist->setBackgroundColor(0xdd000000);
    m_slotlist->setHorizontalScrolling(false);
    m_container->addBaseUIElement(m_slotlist);

    updateLayout(m_osu->getScreenSize());
}

void OsuRoom::draw(Graphics *g) {
    if (!m_bVisible) return;
    m_container->draw(g);
}

void OsuRoom::update() {
    if (!m_bVisible) return;
    m_container->update();
}

void OsuRoom::onKeyDown(KeyboardEvent &key) {
    if(!m_bVisible || m_osu->m_songBrowser2->isVisible()) return;

    if(key.getKeyCode() == KEY_ESCAPE) {
        key.consume();

        // Exit straight to main menu
        m_bVisible = false;
        m_osu->m_mainMenu->setVisible(true);
        m_osu->m_chat->updateVisibility();
        return;
    }
}

void OsuRoom::onResolutionChange(Vector2 newResolution) {
    updateLayout(newResolution);
}

void OsuRoom::setVisible(bool visible) {
    abort(); // surprise!
}

void OsuRoom::updateLayout(Vector2 newResolution) {
    m_container->setSize(newResolution);

    auto heading = new CBaseUILabel(50, 30, 300, 40, "", "Multiplayer");
    heading->setFont(m_osu->getTitleFont());
    heading->setSizeToContent(0, 0);
    heading->setDrawFrame(false);
    heading->setDrawBackground(false);

    // m_slotlist->setPos();
    // m_slotlist->setSize();

    // TODO @kiwec: draw the rest of the room
}

void OsuRoom::on_room_joined(Room room) {
    bancho.room = room;

    // Currently we can only join rooms from the lobby.
    // If we add ability to join from links, you would need to hide all other
    // screens, kick the player out of the song we're currently playing, etc.
    m_osu->m_lobby->setVisible(false);
    m_bVisible = true;
    updateLayout(m_osu->getScreenSize());

    // TODO @kiwec: update status when playing a map
    OsuRichPresence::setBanchoStatus(m_osu, room.name.toUtf8(), MULTIPLAYER);
    m_osu->m_chat->updateVisibility();
}

void OsuRoom::on_room_updated(Room room) {
    // TODO @kiwec: don't update the room while we're playing? or at least not the slots?
    //              need to check if server sends those while we're playing first
    bancho.room = room;

    // TODO @kiwec

    updateLayout(m_osu->getScreenSize());
}

void OsuRoom::on_match_started(Room room) {
    bancho.room = room;

    // OsuRichPresence::setBanchoStatus(m_osu, /* TODO map name */, MULTIPLAYER);

    // TODO @kiwec
}

void OsuRoom::on_match_score_updated(Packet* packet) {
    int32_t update_tms = read_int(packet);
    uint8_t player_id = read_byte(packet);

    for(int i = 0; i < 16; i++) {
        if(bancho.room.slots[i].player_id != player_id) continue;

        bancho.room.slots[i].last_update_tms = update_tms;
        bancho.room.slots[i].num300 = read_short(packet);
        bancho.room.slots[i].num100 = read_short(packet);
        bancho.room.slots[i].num50 = read_short(packet);
        bancho.room.slots[i].num_geki = read_short(packet);
        bancho.room.slots[i].num_katu = read_short(packet);
        bancho.room.slots[i].num_miss = read_short(packet);
        bancho.room.slots[i].total_score = read_int(packet);
        bancho.room.slots[i].current_combo = read_short(packet);
        bancho.room.slots[i].max_combo = read_short(packet);
        bancho.room.slots[i].is_perfect = read_byte(packet);
        bancho.room.slots[i].current_hp = read_byte(packet);
        bancho.room.slots[i].tag = read_byte(packet);
        bancho.room.slots[i].is_scorev2 = read_byte(packet);
        return;
    }
}

void OsuRoom::on_all_players_loaded() {
    // TODO @kiwec
}

void OsuRoom::on_player_failed(int32_t slot_id) {
    if(slot_id < 0 || slot_id > 15) return;
    bancho.room.slots[slot_id].died = true;
}

void OsuRoom::on_match_finished() {
    // TODO @kiwec
}

void OsuRoom::on_all_players_skipped() {
    // TODO @kiwec
}

void OsuRoom::on_player_skip(int32_t user_id) {
    for(int i = 0; i < 16; i++) {
        if(bancho.room.slots[i].player_id == user_id) {
            bancho.room.slots[i].skipped = true;
            break;
        }
    }
}

void OsuRoom::on_match_aborted() {
    // TODO @kiwec
}
