#include "Bancho.h"
#include "BanchoDownloader.h"
#include "BanchoNetworking.h"
#include "BanchoUsers.h"
#include "Osu.h"
#include "OsuBackgroundImageHandler.h"
#include "OsuChat.h"
#include "OsuDatabase.h"
#include "OsuLobby.h"
#include "OsuMainMenu.h"
#include "OsuModSelector.h"
#include "OsuPromptScreen.h"
#include "OsuRankingScreen.h"
#include "OsuRoom.h"
#include "OsuRichPresence.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuSongBrowser2.h"
#include "OsuUIAvatar.h"
#include "OsuUIButton.h"
#include "OsuUICheckbox.h"
#include "OsuUISongBrowserSongButton.h"
#include "OsuUIUserContextMenu.h"

#include "CBaseUIButton.h"
#include "CBaseUIContainer.h"
#include "CBaseUILabel.h"
#include "CBaseUITextbox.h"
#include "Engine.h"
#include "Keyboard.h"
#include "ResourceManager.h"
#include "SoundEngine.h"


void OsuUIModList::draw(Graphics *g) {
    std::vector<OsuSkinImage*> mods;
    if(*m_flags & (1 << 0)) mods.push_back(bancho.osu->getSkin()->getSelectionModNoFail());
    if(*m_flags & (1 << 1)) mods.push_back(bancho.osu->getSkin()->getSelectionModEasy());
    if(*m_flags & (1 << 2)) mods.push_back(bancho.osu->getSkin()->getSelectionModTD());
    if(*m_flags & (1 << 3)) mods.push_back(bancho.osu->getSkin()->getSelectionModHidden());
    if(*m_flags & (1 << 4)) mods.push_back(bancho.osu->getSkin()->getSelectionModHardRock());
    if(*m_flags & (1 << 5)) mods.push_back(bancho.osu->getSkin()->getSelectionModSuddenDeath());
    if(*m_flags & (1 << 7)) mods.push_back(bancho.osu->getSkin()->getSelectionModRelax());
    if(*m_flags & (1 << 8)) mods.push_back(bancho.osu->getSkin()->getSelectionModHalfTime());
    if(*m_flags & (1 << 9)) mods.push_back(bancho.osu->getSkin()->getSelectionModNightCore());
    else if(*m_flags & (1 << 6)) mods.push_back(bancho.osu->getSkin()->getSelectionModDoubleTime());
    if(*m_flags & (1 << 11)) mods.push_back(bancho.osu->getSkin()->getSelectionModAutoplay());
    if(*m_flags & (1 << 12)) mods.push_back(bancho.osu->getSkin()->getSelectionModSpunOut());
    if(*m_flags & (1 << 13)) mods.push_back(bancho.osu->getSkin()->getSelectionModAutopilot());
    if(*m_flags & (1 << 14)) mods.push_back(bancho.osu->getSkin()->getSelectionModPerfect());
    if(*m_flags & (1 << 23)) mods.push_back(bancho.osu->getSkin()->getSelectionModTarget());
    if(*m_flags & (1 << 29)) mods.push_back(bancho.osu->getSkin()->getSelectionModScorev2());

    g->setColor(0xffffffff);
    Vector2 modPos = m_vPos;
    for(auto mod : mods) {
        float target_height = getSize().y;
        float scaling_factor = target_height / mod->getSize().y;
        float target_width = mod->getSize().x * scaling_factor;

        Vector2 fixed_pos = modPos;
        fixed_pos.x += (target_width / 2);
        fixed_pos.y += (target_height / 2);
        mod->draw(g, fixed_pos, scaling_factor);

        // Overlap mods a bit, just like peppy's client does
        modPos.x += target_width * 0.6;
    }
}

bool OsuUIModList::isVisible() {
    return *m_flags != 0;
}


#define INIT_LABEL(label_name, default_text, is_big) do {             \
    label_name = new CBaseUILabel(0, 0, 0, 0, "label", default_text); \
    label_name->setFont(is_big ? lfont : font);                       \
    label_name->setSizeToContent(0, 0);                               \
    label_name->setDrawFrame(false);                                  \
    label_name->setDrawBackground(false);                             \
    m_settings->getContainer()->addBaseUIElement(label_name);         \
} while(0)

OsuRoom::OsuRoom(Osu *osu) : OsuScreen(osu) {
    // XXX: Place buttons to the right of the large labels

    font = engine->getResourceManager()->getFont("FONT_DEFAULT");
    lfont = osu->getSubTitleFont();

    m_pauseButton = new OsuMainMenuPauseButton(0, 0, 0, 0, "pause_btn", "");
    m_pauseButton->setClickCallback( fastdelegate::MakeDelegate(osu->m_mainMenu, &OsuMainMenu::onPausePressed) );
    addBaseUIElement(m_pauseButton);

    m_settings = new CBaseUIScrollView(0, 0, 0, 0, "room_settings");
    m_settings->setDrawFrame(false); // it's off by 1 pixel, turn it OFF
    m_settings->setDrawBackground(true);
    m_settings->setBackgroundColor(0xdd000000);
    m_settings->setHorizontalScrolling(false);
    addBaseUIElement(m_settings);

    INIT_LABEL(m_room_name, "Multiplayer room", true);
    INIT_LABEL(m_host, "Host: None", false); // XXX: make it an OsuUIUserLabel

    INIT_LABEL(m_room_name_iptl, "Room name", false);
    m_room_name_ipt = new CBaseUITextbox(0, 0, m_settings->getSize().x, 40, "");
    m_room_name_ipt->setText("Multiplayer room");
    m_settings->getContainer()->addBaseUIElement(m_room_name_ipt);

    m_change_password_btn = new OsuUIButton(osu, 0, 0, 190, 40, "change_password_btn", "Change password");
    m_change_password_btn->setColor(0xff0e94b5);
    m_change_password_btn->setUseDefaultSkin();
    m_change_password_btn->setClickCallback( fastdelegate::MakeDelegate(this, &OsuRoom::onChangePasswordClicked) );
    m_settings->getContainer()->addBaseUIElement(m_change_password_btn);

    CBaseUILabel* map_label;
    INIT_LABEL(map_label, "Beatmap", true);

    m_select_map_btn = new OsuUIButton(osu, 0, 0, 130, 40, "select_map_btn", "Select map");
    m_select_map_btn->setColor(0xff0e94b5);
    m_select_map_btn->setUseDefaultSkin();
    m_select_map_btn->setClickCallback( fastdelegate::MakeDelegate(this, &OsuRoom::onSelectMapClicked) );
    m_settings->getContainer()->addBaseUIElement(m_select_map_btn);

    INIT_LABEL(m_map_title, "(no map selected)", false);
    INIT_LABEL(m_map_stars, "", false);
    INIT_LABEL(m_map_attributes, "", false);
    INIT_LABEL(m_map_attributes2, "", false);

    CBaseUILabel* mods_label;
    INIT_LABEL(mods_label, "Mods", true);
    m_select_mods_btn = new OsuUIButton(osu, 0, 0, 180, 40, "select_mods_btn", "Select mods [F1]");
    m_select_mods_btn->setColor(0xff0e94b5);
    m_select_mods_btn->setUseDefaultSkin();
    m_select_mods_btn->setClickCallback( fastdelegate::MakeDelegate(this, &OsuRoom::onSelectModsClicked) );
    m_settings->getContainer()->addBaseUIElement(m_select_mods_btn);
    m_freemod = new OsuUICheckbox(m_osu, 0, 0, 200, 50, "allow_freemod", "Freemod");
    m_freemod->setDrawFrame(false);
    m_freemod->setDrawBackground(false);
    m_freemod->setChangeCallback( fastdelegate::MakeDelegate(this, &OsuRoom::onFreemodCheckboxChanged) );
    m_settings->getContainer()->addBaseUIElement(m_freemod);
    m_mods = new OsuUIModList(&bancho.room.mods);
    m_settings->getContainer()->addBaseUIElement(m_mods);
    INIT_LABEL(m_no_mods_selected, "No mods selected.", false);

    m_ready_btn = new OsuUIButton(osu, 0, 0, 300, 50, "start_game_btn", "Start game");
    m_ready_btn->setColor(0xff00ff00);
    m_ready_btn->setUseDefaultSkin();
    m_ready_btn->setClickCallback( fastdelegate::MakeDelegate(this, &OsuRoom::onReadyButtonClick) );
    m_ready_btn->is_loading = true;
    m_settings->getContainer()->addBaseUIElement(m_ready_btn);

    auto player_list_label = new CBaseUILabel(50, 50, 0, 0, "label", "Player list");
    player_list_label->setFont(lfont);
    player_list_label->setSizeToContent(0, 0);
    player_list_label->setDrawFrame(false);
    player_list_label->setDrawBackground(false);
    addBaseUIElement(player_list_label);
    m_slotlist = new CBaseUIScrollView(50, 90, 0, 0, "slot_list");
    m_slotlist->setDrawFrame(true);
    m_slotlist->setDrawBackground(true);
    m_slotlist->setBackgroundColor(0xdd000000);
    m_slotlist->setHorizontalScrolling(false);
    addBaseUIElement(m_slotlist);

    updateLayout(m_osu->getScreenSize());
}

void OsuRoom::draw(Graphics *g) {
    if (!m_bVisible) return;

    // XXX: Add convar for toggling room backgrounds
    OsuSongBrowser2::drawSelectedBeatmapBackgroundImage(g, m_osu, 1.0);
    OsuScreen::draw(g);

    // Update avatar visibility status
    for(auto elm : m_slotlist->getContainer()->getElements()) {
        if(elm->getName() == UString("avatar")) {
            // NOTE: Not checking horizontal visibility
            auto avatar = (OsuUIAvatar*)elm;
            bool is_below_top = avatar->getPos().y + avatar->getSize().y >= m_slotlist->getPos().y;
            bool is_above_bottom = avatar->getPos().y <= m_slotlist->getPos().y + m_slotlist->getSize().y;
            avatar->on_screen = is_below_top && is_above_bottom;
        }
    }

    // Not technically drawing below this line, just checking for map download progress
    if(m_osu->getSelectedBeatmap() != NULL) return;
    if(bancho.room.map_id == 0) return;
    if(bancho.room.map_id == -1) {
        m_map_title->setText("Host is selecting a map...");
        m_map_title->setSizeToContent(0, 0);
        m_ready_btn->is_loading = true;
        return;
    }

    auto search = mapset_by_mapid.find(bancho.room.map_id);
    if (search == mapset_by_mapid.end()) return;
    uint32_t set_id = search->second;
    if(set_id == 0) {
        auto error_str = UString::format("Could not find beatmapset for map ID %d", bancho.room.map_id);
        m_map_title->setText(error_str);
        m_map_title->setSizeToContent(0, 0);
        m_ready_btn->is_loading = true;
        return;
    }

    auto status = download_mapset(set_id);
    if(status.status == FAILURE) {
        auto error_str = UString::format("Failed to download beatmapset %d :(", set_id);
        m_map_title->setText(error_str);
        m_map_title->setSizeToContent(0, 0);
        m_ready_btn->is_loading = true;
        bancho.room.map_id = 0; // don't try downloading it again
    } else if(status.status == DOWNLOADING) {
        auto text = UString::format("Downloading... %.2f%%", status.progress * 100.f);
        m_map_title->setText(text.toUtf8());
        m_map_title->setSizeToContent(0, 0);
        m_ready_btn->is_loading = true;
    } else if(status.status == SUCCESS) {
        auto mapset_path = UString::format(MCENGINE_DATA_DIR "maps/%d/", set_id);
        // XXX: Make a permanent database for auto-downloaded songs, so we can load them like osu!.db's
        m_osu->m_songBrowser2->getDatabase()->addBeatmap(mapset_path);
        m_osu->m_songBrowser2->updateSongButtonSorting();
        debugLog("Finished loading beatmapset %d.\n", set_id);
        on_map_change(false);
    }
}

void OsuRoom::mouse_update(bool *propagate_clicks) {
    if (!m_bVisible || m_osu->m_songBrowser2->isVisible()) return;

    const bool room_name_changed = m_room_name_ipt->getText() != bancho.room.name;
    if(bancho.room.is_host() && room_name_changed) {
        // XXX: should only update 500ms after last input
        bancho.room.name = m_room_name_ipt->getText();

        Packet packet = {0};
        packet.id = MATCH_CHANGE_SETTINGS;
        bancho.room.pack(&packet);
        send_packet(packet);
    }

    if (m_osu->getSelectedBeatmap() != NULL)
        m_pauseButton->setPaused(!m_osu->getSelectedBeatmap()->isPreviewMusicPlaying());
    else
        m_pauseButton->setPaused(true);

    OsuScreen::mouse_update(propagate_clicks);
}

void OsuRoom::onKeyDown(KeyboardEvent &key) {
    if(!m_bVisible || m_osu->m_songBrowser2->isVisible()) return;

    if(key.getKeyCode() == KEY_ESCAPE) {
        key.consume();
        ragequit();
        return;
    }

    if(key.getKeyCode() == KEY_F1) {
        key.consume();
        if(bancho.room.freemods || bancho.room.is_host()) {
            m_osu->m_modSelector->setVisible(!m_osu->m_modSelector->isVisible());
            m_bVisible = !m_osu->m_modSelector->isVisible();
        }
        return;
    }

    OsuScreen::onKeyDown(key);
}

void OsuRoom::onKeyUp(KeyboardEvent &key) {
    if(!m_bVisible || m_osu->m_songBrowser2->isVisible()) return;
    OsuScreen::onKeyUp(key);
}

void OsuRoom::onChar(KeyboardEvent &key) {
    if(!m_bVisible || m_osu->m_songBrowser2->isVisible()) return;
    OsuScreen::onChar(key);
}

void OsuRoom::onResolutionChange(Vector2 newResolution) {
    updateLayout(newResolution);
}

CBaseUIContainer* OsuRoom::setVisible(bool visible) {
    // NOTE: Calling setVisible(false) does not quit the room! Call ragequit() instead.
    m_bVisible = visible;
    return this;
}

void OsuRoom::updateLayout(Vector2 newResolution) {
    setSize(newResolution);

    m_settings->setPos(round(newResolution.x * 0.6), 0);
    m_settings->setSize(round(newResolution.x * 0.4), newResolution.y);

    const float dpiScale = Osu::getUIScale(m_osu);
    m_pauseButton->setSize(30 * dpiScale, 30 * dpiScale);
    m_pauseButton->setPos(round(newResolution.x * 0.6) - m_pauseButton->getSize().x*2 - 10 * dpiScale, m_pauseButton->getSize().y + 10 * dpiScale);

    const bool is_host = bancho.room.is_host();
    UString host_str = "Host: None";
    if(bancho.room.host_id > 0) {
        auto host = get_user_info(bancho.room.host_id);
        host_str = UString::format("Host: %s", host->name.toUtf8());
    }
    m_host->setText(host_str);
    m_host->setVisible(!is_host);

    m_room_name->setText(bancho.room.name);
    m_room_name->setSizeToContent();
    if(!is_host) {
        m_room_name_ipt->setText(bancho.room.name);
    }
    m_room_name_iptl->setVisible(is_host);
    m_room_name_ipt->setSize(m_settings->getSize().x - 20, 40);
    m_room_name_ipt->setVisible(is_host);

    m_change_password_btn->setVisible(is_host);

    m_map_title->setVisible(true);
    m_map_attributes->setVisible(!m_ready_btn->is_loading);
    m_map_attributes2->setVisible(!m_ready_btn->is_loading);
    m_map_stars->setVisible(!m_ready_btn->is_loading);

    m_select_map_btn->setVisible(is_host);

    m_select_mods_btn->setVisible(is_host || bancho.room.freemods);
    m_freemod->setChecked(bancho.room.freemods);
    m_freemod->setVisible(is_host);
    m_mods->m_flags = &bancho.room.mods;
    m_mods->setSize(300, 90);
    m_no_mods_selected->setVisible(bancho.room.mods == 0);
    
    bool is_ready = false;
    for(uint8_t i = 0; i < 16; i++) {
        if(bancho.room.slots[i].player_id == bancho.user_id) {
            is_ready = bancho.room.slots[i].is_ready();
            break;
        }
    }
    if(is_host) {
        m_ready_btn->setText("Start game");
        m_ready_btn->setColor(0xff00ff00);
    } else {
        m_ready_btn->setText(is_ready ? "Not ready" : "Ready!");
        m_ready_btn->setColor(is_ready ? 0xffff0000 : 0xff00ff00);
    }
    m_ready_btn->setVisible(true); // wtf...
    m_settings->setScrollSizeToContent();

    float settings_y = 10;
    for(auto& element : m_settings->getContainer()->getElements()) {
        if(element->isVisible()) {
            element->setRelPos(10, settings_y);
            settings_y += element->getSize().y + 20;
        } else {
            // HACK: Still getting drawn despite isVisible() being false. idk
            element->setRelPos(-999, -999);
        }
    }

    // XXX: Display detailed user presence
    m_slotlist->setSize(newResolution.x * 0.6 - 200, newResolution.y * 0.6 - 110);
    m_slotlist->clear();
    int y_total = 10;
    for(int i = 0; i < 16; i++) {
        if(bancho.room.slots[i].has_player()) {
            auto user_info = get_user_info(bancho.room.slots[i].player_id);
            auto username = user_info->name;
            if(bancho.room.slots[i].is_player_playing()) {
                username = UString::format("[playing] %s", user_info->name.toUtf8());
            }

            const float SLOT_HEIGHT = 40.f;
            auto avatar = new OsuUIAvatar(bancho.room.slots[i].player_id, 10, y_total, SLOT_HEIGHT, SLOT_HEIGHT);
            m_slotlist->getContainer()->addBaseUIElement(avatar);

            auto user_box = new OsuUIUserLabel(m_osu, bancho.room.slots[i].player_id, username);
            user_box->setFont(lfont);
            user_box->setPos(avatar->getRelPos().x + avatar->getSize().x + 10, y_total);

            auto color = 0xffffffff;
            if(bancho.room.slots[i].is_ready()) {
                color = 0xff00ff00;
            } else if(bancho.room.slots[i].no_map()) {
                color = 0xffdd0000;
            }
            user_box->setTextColor(color);
            user_box->setSizeToContent();
            user_box->setSize(user_box->getSize().x, SLOT_HEIGHT);
            m_slotlist->getContainer()->addBaseUIElement(user_box);

            auto user_mods = new OsuUIModList(&bancho.room.slots[i].mods);
            user_mods->setPos(user_box->getPos().x + user_box->getSize().x + 30, y_total);
            user_mods->setSize(350, SLOT_HEIGHT);
            m_slotlist->getContainer()->addBaseUIElement(user_mods);

            y_total += SLOT_HEIGHT + 5;
        }
    }
    m_slotlist->setScrollSizeToContent();
}

// Exit to main menu
void OsuRoom::ragequit() {
    m_bVisible = false;
    bancho.match_started = false;

    Packet packet = {0};
    packet.id = EXIT_ROOM;
    send_packet(packet);

    m_osu->m_modSelector->resetMods();
    m_osu->m_modSelector->updateButtons();

    bancho.room = Room();
    m_osu->m_mainMenu->setVisible(true);
    m_osu->m_chat->removeChannel("#multiplayer");
    m_osu->m_chat->updateVisibility();
}

void OsuRoom::process_beatmapset_info_response(Packet packet) {
    uint32_t map_id = packet.extra_int;
    if(packet.size == 0) {
        bancho.osu->m_room->mapset_by_mapid[map_id] = 0;
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
    bancho.osu->m_room->mapset_by_mapid[map_id] = strtoul(str, NULL, 10);

    // Do nothing with the rest
}

void OsuRoom::on_map_change(bool download) {
    // Results screen has map background and such showing, so prevent map from changing while we're on it.
    if(m_osu->m_rankingScreen->isVisible()) return;

    debugLog("Map changed to ID %d, MD5 %s: %s\n", bancho.room.map_id, bancho.room.map_md5.toUtf8(), bancho.room.map_name.toUtf8());
    m_ready_btn->is_loading = true;

    // Deselect current map
    if(m_osu->getSelectedBeatmap() != nullptr) {
        m_pauseButton->setPaused(true);
        m_osu->m_songBrowser2->m_selectedBeatmap->deselect();
        m_osu->m_songBrowser2->m_selectedBeatmap = nullptr;
    }

    if(bancho.room.map_id == 0) {
        m_map_title->setText("(no map selected)");
        m_map_title->setSizeToContent(0, 0);
        m_ready_btn->is_loading = true;
    } else {
        std::string hash = bancho.room.map_md5.toUtf8(); // lol
        auto beatmap = m_osu->getSongBrowser()->getDatabase()->getBeatmapDifficulty(hash);
        if(beatmap != nullptr) {
            m_osu->m_songBrowser2->onDifficultySelected(beatmap, false);
            m_map_title->setText(bancho.room.map_name);
            m_map_title->setSizeToContent(0, 0);
            auto attributes = UString::format("AR: %.1f, CS: %.1f, HP: %.1f, OD: %.1f", beatmap->getAR(), beatmap->getCS(), beatmap->getHP(), beatmap->getOD());
            m_map_attributes->setText(attributes);
            m_map_attributes->setSizeToContent(0, 0);
            auto attributes2 = UString::format("Length: %d seconds, BPM: %d (%d - %d)", beatmap->getLengthMS() / 1000, beatmap->getMostCommonBPM(), beatmap->getMinBPM(), beatmap->getMaxBPM());
            m_map_attributes2->setText(attributes2);
            m_map_attributes2->setSizeToContent(0, 0);

            auto stars = UString::format("Star rating: %.2f*", beatmap->getStarsNomod());
            m_map_stars->setText(stars);
            m_map_stars->setSizeToContent(0, 0);
            m_ready_btn->is_loading = false;

            Packet packet = {0};
            packet.id = MATCH_HAS_BEATMAP;
            send_packet(packet);
        } else if(download) {
            // Request beatmap info - automatically starts download
            auto path = UString::format("/web/osu-search-set.php?b=%d&u=%s&h=%s", bancho.room.map_id, bancho.username.toUtf8(), bancho.pw_md5.toUtf8());
            APIRequest request = {
                .type = GET_BEATMAPSET_INFO,
                .path = path,
                .extra_int = (uint32_t)bancho.room.map_id,
            };
            send_api_request(request);

            m_map_title->setText("Loading...");
            m_map_title->setSizeToContent(0, 0);
            m_ready_btn->is_loading = true;

            Packet packet = {0};
            packet.id = MATCH_NO_BEATMAP;
            send_packet(packet);
        } else {
            m_map_title->setText("Failed to load map. Is it catch/taiko/mania?");
            m_map_title->setSizeToContent(0, 0);
            m_ready_btn->is_loading = true;
            bancho.room.map_id = 0; // prevents trying to load it again

            Packet packet = {0};
            packet.id = MATCH_NO_BEATMAP;
            send_packet(packet);
        }
    }

    updateLayout(m_osu->getScreenSize());
}

void OsuRoom::on_room_joined(Room room) {
    bancho.room = room;
    debugLog("Joined room #%d\nPlayers:\n", room.id);
    for(int i = 0; i < 16; i++) {
        if(room.slots[i].has_player()) {
            auto user_info = get_user_info(room.slots[i].player_id);
            debugLog("- %s\n", user_info->name.toUtf8());
        }
    }

    on_map_change(true);

    // Currently we can only join rooms from the lobby.
    // If we add ability to join from links, you would need to hide all other
    // screens, kick the player out of the song they're currently playing, etc.
    m_osu->m_lobby->setVisible(false);
    updateLayout(m_osu->getScreenSize());
    m_bVisible = true;

    OsuRichPresence::setBanchoStatus(m_osu, room.name.toUtf8(), MULTIPLAYER);
    m_osu->m_chat->addChannel("#multiplayer");
    m_osu->m_chat->updateVisibility();

    m_osu->m_modSelector->resetMods();
    m_osu->m_modSelector->enableModsFromFlags(bancho.room.mods);
}

void OsuRoom::on_room_updated(Room room) {
    if(bancho.is_playing_a_multi_map() || !bancho.is_in_a_multi_room()) return;

    bool map_changed = bancho.room.map_id != room.map_id;
    bancho.room = room;
    if(map_changed) {
        on_map_change(true);
    }

    uint32_t player_mods = 0;
    for(int i = 0; i < 16; i++) {
        if(bancho.room.slots[i].player_id == bancho.user_id) {
            player_mods = bancho.room.slots[i].mods;
            break;
        }
    }
    m_osu->m_modSelector->updateButtons();
    m_osu->m_modSelector->resetMods();
    m_osu->m_modSelector->enableModsFromFlags(bancho.room.mods | player_mods);

    updateLayout(m_osu->getScreenSize());
}

void OsuRoom::on_match_started(Room room) {
    bancho.room = room;
    if(m_osu->getSelectedBeatmap() == nullptr) {
        debugLog("We received MATCH_STARTED without being ready, wtf!\n");
        return;
    }

    last_packet_tms = time(NULL);

    m_osu->onBeforePlayStart();
    if(m_osu->getSelectedBeatmap()->play()) {
        m_bVisible = false;
        bancho.match_started = true;
        m_osu->m_songBrowser2->m_bHasSelectedAndIsPlaying = true;
        m_osu->onPlayStart();
        m_osu->m_chat->updateVisibility();
    } else {
        ragequit(); // map failed to load
    }
}

void OsuRoom::on_match_score_updated(Packet* packet) {
    int32_t update_tms = read_int(packet);
    uint8_t slot_id = read_byte(packet);
    if(slot_id > 15) return;

    auto slot = &bancho.room.slots[slot_id];
    slot->last_update_tms = update_tms;
    slot->num300 = read_short(packet);
    slot->num100 = read_short(packet);
    slot->num50 = read_short(packet);
    slot->num_geki = read_short(packet);
    slot->num_katu = read_short(packet);
    slot->num_miss = read_short(packet);
    slot->total_score = read_int(packet);
    slot->max_combo = read_short(packet);
    slot->current_combo = read_short(packet);
    slot->is_perfect = read_byte(packet);
    slot->current_hp = read_byte(packet);
    slot->tag = read_byte(packet);

    bool is_scorev2 = read_byte(packet);
    if(is_scorev2) {
        slot->sv2_combo = read_float64(packet);
        slot->sv2_bonus = read_float64(packet);
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
    memcpy(bancho.last_scores, bancho.room.slots, sizeof(bancho.room.slots));
    m_osu->onPlayEnd(false, false);
    bancho.match_started = false;
    m_osu->m_rankingScreen->setVisible(true);
    m_osu->m_chat->updateVisibility();
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
    m_bVisible = true;
    bancho.match_started = false;
}

void OsuRoom::onClientScoreChange(bool force) {
    if(!bancho.is_playing_a_multi_map()) return;

    // Update at most once every 250ms
    bool should_update = difftime(time(NULL), last_packet_tms) > 0.25;
    if(!should_update && !force) return;

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
    write_byte(&packet, m_osu->getScore()->getNumSliderBreaks() == 0 && m_osu->getScore()->getNumMisses() == 0 && m_osu->getScore()->getNum50s() == 0 && m_osu->getScore()->getNum100s() == 0);
    write_byte(&packet, m_osu->getSelectedBeatmap()->getHealth() * 200);
    write_byte(&packet, 0); // 4P, not supported
    write_byte(&packet, m_osu->getModScorev2());
    send_packet(packet);

    last_packet_tms = time(NULL);
}

void OsuRoom::onReadyButtonClick() {
    if(m_ready_btn->is_loading) return;
    engine->getSound()->play(m_osu->getSkin()->getMenuHit());

    if(bancho.room.is_host()) {
        Packet packet = {0};
        packet.id = START_MATCH;
        send_packet(packet);
        return;
    }

    bool is_ready = false;
    for(uint8_t i = 0; i < 16; i++) {
        if(bancho.room.slots[i].player_id == bancho.user_id) {
            is_ready = bancho.room.slots[i].is_ready();
            break;
        }
    }

    Packet packet = {0};
    packet.id = is_ready ? MATCH_NOT_READY : MATCH_READY;
    send_packet(packet);
}

void OsuRoom::onSelectModsClicked() {
    engine->getSound()->play(m_osu->getSkin()->getMenuHit());
    m_osu->m_modSelector->setVisible(true);
    m_bVisible = false;
}

void OsuRoom::onSelectMapClicked() {
    if(!bancho.room.is_host()) return;

    engine->getSound()->play(m_osu->getSkin()->getMenuHit());

    Packet packet = {0};
    packet.id = MATCH_CHANGE_SETTINGS;
    bancho.room.map_id = -1;
    bancho.room.map_name = "";
    bancho.room.map_md5 = "";
    bancho.room.pack(&packet);
    send_packet(packet);

    m_osu->m_songBrowser2->setVisible(true);
}

void OsuRoom::onChangePasswordClicked() {
    m_osu->m_prompt->prompt("New password:", fastdelegate::MakeDelegate(this, &OsuRoom::set_new_password));
}

void OsuRoom::set_new_password(UString new_password) {
    bancho.room.has_password = new_password.length() > 0;
    bancho.room.password = new_password;

    Packet packet = {0};
    packet.id = CHANGE_ROOM_PASSWORD;
    bancho.room.pack(&packet);
    send_packet(packet);
}

void OsuRoom::onFreemodCheckboxChanged(CBaseUICheckbox *checkbox) {
    if(!bancho.room.is_host()) return;

    bancho.room.freemods = checkbox->isChecked();

    Packet packet = {0};
    packet.id = MATCH_CHANGE_SETTINGS;
    bancho.room.pack(&packet);
    send_packet(packet);
    
    // TODO @kiwec: this is not updating correctly, idk why
    updateLayout(m_osu->getScreenSize());
}
