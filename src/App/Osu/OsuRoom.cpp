#include "Bancho.h"
#include "BanchoDownloader.h"
#include "BanchoNetworking.h"
#include "BanchoUsers.h"
#include "Osu.h"
#include "OsuChat.h"
#include "OsuDatabase.h"
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

    m_map = new CBaseUIScrollView(0, 0, 0, 0, "");
    m_map->setDrawFrame(true);
    m_map->setDrawBackground(false);
    m_map->setBlockScrolling(true);
    m_container->addBaseUIElement(m_map);

    m_map_title = new CBaseUILabel(0, 0, 300, 20, "", "(no map selected)");
    m_map_title->setFont(font);
    m_map_title->setSizeToContent(0, 0);
    m_map_title->setDrawFrame(false);
    m_map_title->setDrawBackground(false);
    m_map->getContainer()->addBaseUIElement(m_map_title);

    m_map_extra = new CBaseUILabel(0, 0, 300, 20, "", "");
    m_map_extra->setFont(font);
    m_map_extra->setSizeToContent(0, 0);
    m_map_extra->setDrawFrame(false);
    m_map_extra->setDrawBackground(false);
    m_map->getContainer()->addBaseUIElement(m_map_extra);

    updateLayout(m_osu->getScreenSize());
}

void OsuRoom::draw(Graphics *g) {
    if (!m_bVisible) return;

    if(downloading_set_id != 0 && m_osu->getSelectedBeatmap() == NULL) {
        auto status = download_mapset(downloading_set_id);
        if(status.status == FAILURE) {
            downloading_set_id = 0;
            m_map_extra->setText("FAILED TO DOWNLOAD BEATMAP");
            // TODO @kiwec
        } else if(status.status == DOWNLOADING) {
            auto text = UString::format("Downloading... %d%", status.progress * 100);
            m_map_extra->setText(text.toUtf8());
        } else if(status.status == SUCCESS) {
            auto mapset_path = UString::format(MCENGINE_DATA_DIR "maps/%d", downloading_set_id);
            m_osu->m_songBrowser2->getDatabase()->addBeatmap(mapset_path);
            m_osu->m_songBrowser2->updateSongButtonSorting();
            downloading_set_id = 0;
            on_map_change();
        }
    }

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
        ragequit();
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

// Exit to main menu
void OsuRoom::ragequit() {
    if(!bancho.is_in_a_multi_room()) return;

    Packet packet = {0};
    packet.id = EXIT_ROOM;
    send_packet(packet);

    m_bVisible = false;
    m_osu->m_mainMenu->setVisible(true);
    m_osu->m_chat->removeChannel("#multiplayer");
    m_osu->m_chat->updateVisibility();
}

void OsuRoom::process_beatmapset_info_response(Packet packet) {
    if(packet.size == 0) {
        // TODO @kiwec
        return;
    }

    // {set_id}.osz|{artist}|{title}|{creator}|{status}|10.0|{last_update}|{set_id}|0|0|0|0|0
    char *saveptr = NULL;
    char *str = strtok_r((char*)packet.memory, "|", &saveptr);
    if (!str) return;
    // Do nothing with beatmapset filename

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return;
    // Do nothing with beatmap artist

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return;
    // Do nothing with beatmap title

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return;
    // Do nothing with beatmap creator

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return;
    // Do nothing with beatmap status

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return;
    // Do nothing with beatmap rating

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return;
    // Do nothing with beatmap last update

    str = strtok_r(NULL, "|", &saveptr);
    if(!str) return;
    bancho.osu->m_room->downloading_set_id = strtoul(str, NULL, 10);

    // Do nothing with the rest
}

void OsuRoom::on_map_change() {
    // TODO @kiwec: handle non-std maps

    if(bancho.room.map_id == 0) {
        m_map_title->setText("(no map selected)");
        m_map_extra->setText("");
    } else {
        std::string hash = bancho.room.map_md5.toUtf8(); // lol
        auto beatmap = m_osu->getSongBrowser()->getDatabase()->getBeatmapDifficulty(hash);
        if(beatmap == nullptr) {
            // Request beatmap info - automatically starts download
            std::string path = "/web/osu-search-set.php?b=" + std::to_string(bancho.room.map_id);
            path += "&u=" + std::string(bancho.username.toUtf8());
            path += "&h=" + std::string(bancho.pw_md5.toUtf8());
            APIRequest request = {
                .type = GET_BEATMAPSET_INFO,
                .path = path,
            };
            send_api_request(request);

            m_map_title->setText(bancho.room.map_name);
            m_map_extra->setText("Downloading... 0%");
        } else {
            m_osu->m_songBrowser2->onDifficultySelected(beatmap, false);
            m_map_title->setText(bancho.room.map_name);
            m_map_extra->setText(beatmap->getArtist());
            // TODO @kiwec: getBackgroundImageFileName() / display map image
        }
    }

    updateLayout(m_osu->getScreenSize());
}

void OsuRoom::on_room_joined(Room room) {
    bancho.room = room;
    on_map_change();

    // Currently we can only join rooms from the lobby.
    // If we add ability to join from links, you would need to hide all other
    // screens, kick the player out of the song we're currently playing, etc.
    m_osu->m_lobby->setVisible(false);
    m_bVisible = true;
    updateLayout(m_osu->getScreenSize());

    OsuRichPresence::setBanchoStatus(m_osu, room.name.toUtf8(), MULTIPLAYER);
    m_osu->m_chat->addChannel("#multiplayer");
    m_osu->m_chat->updateVisibility();
}

void OsuRoom::on_room_updated(Room room) {
    if(bancho.is_playing_a_multi_map() || !bancho.is_in_a_multi_room()) return;

    bool map_changed = bancho.room.map_id != room.map_id;
    bancho.room = room;
    if(map_changed) {
        on_map_change();
    }

    updateLayout(m_osu->getScreenSize());
}

void OsuRoom::on_match_started(Room room) {
    bancho.room = room;
    if(m_osu->getSelectedBeatmap() == nullptr) {
        debugLog("We received MATCH_STARTED without being ready, wtf!\n");
        return;
    }

    auto map_name = UString::format("%s [%s]", m_osu->getSelectedBeatmap()->getTitle(), m_osu->getSelectedBeatmap()->getSelectedDifficulty2()->getDifficultyName());
    OsuRichPresence::setBanchoStatus(m_osu, map_name.toUtf8(), MULTIPLAYER);

    m_bVisible = false;
    // TODO @kiwec: onbeforeplay etc?
    m_osu->getSelectedBeatmap()->play();
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
    bancho.room.all_players_loaded = true;
}

void OsuRoom::on_player_failed(int32_t slot_id) {
    if(slot_id < 0 || slot_id > 15) return;
    bancho.room.slots[slot_id].died = true;
}

// All players have finished.
void OsuRoom::on_match_finished() {
    if(!bancho.is_playing_a_multi_map()) return;
    m_osu->onPlayEnd(false, false);
    m_bVisible = true;
}

void OsuRoom::on_all_players_skipped() {
    bancho.room.all_players_skipped = true;
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
    if(!bancho.is_playing_a_multi_map()) return;
    m_osu->onPlayEnd(false, true);
}

void OsuRoom::onClientScoreChange() {
    if(!bancho.is_playing_a_multi_map()) return;

    Packet packet = {0};
    packet.id = UPDATE_MATCH_SCORE;

    write_int(&packet, (int32_t)m_osu->getSelectedBeatmap()->getTime());

    uint8_t slot_id = 0;
    for(uint8_t i = 0; i < 16; i++) {
        if(bancho.room.slots[i].player_id == bancho.user_id) {
            slot_id = i;
            break;
        }
    }
    write_byte(&packet, slot_id);

    write_short(&packet, (uint16_t)m_osu->getScore()->getNum300s());
    write_short(&packet, (uint16_t)m_osu->getScore()->getNum100s());
    write_short(&packet, (uint16_t)m_osu->getScore()->getNum50s());
    write_short(&packet, (uint16_t)m_osu->getScore()->getNum300gs());
    write_short(&packet, (uint16_t)m_osu->getScore()->getNum100ks());
    write_short(&packet, (uint16_t)m_osu->getScore()->getNumMisses());
    write_int(&packet, (int32_t)m_osu->getScore()->getScore());
    write_short(&packet, (uint16_t)m_osu->getScore()->getCombo());
    write_short(&packet, (uint16_t)m_osu->getScore()->getComboMax());
    write_byte(&packet, m_osu->getScore()->getNumSliderBreaks() == 0 && m_osu->getScore()->getNumMisses() == 0); // TODO @kiwec: is this correct?
    write_byte(&packet, m_osu->getSelectedBeatmap()->getHealth() * 100); // TODO @kiwec: currently doing 0-100, might be 0-255
    write_byte(&packet, 0); // 4P, not supported
    write_byte(&packet, m_osu->getModScorev2());
    send_packet(packet);
}
