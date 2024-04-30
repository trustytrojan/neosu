#include "RoomScreen.h"

#include <sstream>

#include "BackgroundImageHandler.h"
#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoUsers.h"
#include "Beatmap.h"
#include "CBaseUIButton.h"
#include "CBaseUIContainer.h"
#include "CBaseUILabel.h"
#include "CBaseUITextbox.h"
#include "Chat.h"
#include "Database.h"
#include "Downloader.h"
#include "Engine.h"
#include "HUD.h"
#include "Keyboard.h"
#include "Lobby.h"
#include "MainMenu.h"
#include "ModSelector.h"
#include "Mouse.h"
#include "Osu.h"
#include "PromptScreen.h"
#include "RankingScreen.h"
#include "Replay.h"
#include "ResourceManager.h"
#include "RichPresence.h"
#include "Skin.h"
#include "SkinImage.h"
#include "SongBrowser.h"
#include "SoundEngine.h"
#include "UIAvatar.h"
#include "UIButton.h"
#include "UICheckbox.h"
#include "UIContextMenu.h"
#include "UISongBrowserSongButton.h"
#include "UIUserContextMenu.h"

void UIModList::draw(Graphics *g) {
    std::vector<SkinImage *> mods;

    if(*m_flags & ModFlags::Nightcore)
        mods.push_back(bancho.osu->getSkin()->getSelectionModNightCore());
    else if(*m_flags & ModFlags::DoubleTime)
        mods.push_back(bancho.osu->getSkin()->getSelectionModDoubleTime());

    bool ht_enabled = *m_flags & ModFlags::HalfTime;
    if(ht_enabled && bancho.prefer_daycore)
        mods.push_back(bancho.osu->getSkin()->getSelectionModDayCore());
    else if(ht_enabled)
        mods.push_back(bancho.osu->getSkin()->getSelectionModHalfTime());

    if(*m_flags & ModFlags::Perfect)
        mods.push_back(bancho.osu->getSkin()->getSelectionModPerfect());
    else if(*m_flags & ModFlags::SuddenDeath)
        mods.push_back(bancho.osu->getSkin()->getSelectionModSuddenDeath());

    if(*m_flags & ModFlags::NoFail) mods.push_back(bancho.osu->getSkin()->getSelectionModNoFail());
    if(*m_flags & ModFlags::Easy) mods.push_back(bancho.osu->getSkin()->getSelectionModEasy());
    if(*m_flags & ModFlags::TouchDevice) mods.push_back(bancho.osu->getSkin()->getSelectionModTD());
    if(*m_flags & ModFlags::Hidden) mods.push_back(bancho.osu->getSkin()->getSelectionModHidden());
    if(*m_flags & ModFlags::HardRock) mods.push_back(bancho.osu->getSkin()->getSelectionModHardRock());
    if(*m_flags & ModFlags::Relax) mods.push_back(bancho.osu->getSkin()->getSelectionModRelax());
    if(*m_flags & ModFlags::Autoplay) mods.push_back(bancho.osu->getSkin()->getSelectionModAutoplay());
    if(*m_flags & ModFlags::SpunOut) mods.push_back(bancho.osu->getSkin()->getSelectionModSpunOut());
    if(*m_flags & ModFlags::Autopilot) mods.push_back(bancho.osu->getSkin()->getSelectionModAutopilot());
    if(*m_flags & ModFlags::Target) mods.push_back(bancho.osu->getSkin()->getSelectionModTarget());
    if(*m_flags & ModFlags::ScoreV2) mods.push_back(bancho.osu->getSkin()->getSelectionModScorev2());

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

bool UIModList::isVisible() { return *m_flags != 0; }

#define INIT_LABEL(label_name, default_text, is_big)                      \
    do {                                                                  \
        label_name = new CBaseUILabel(0, 0, 0, 0, "label", default_text); \
        label_name->setFont(is_big ? lfont : font);                       \
        label_name->setSizeToContent(0, 0);                               \
        label_name->setDrawFrame(false);                                  \
        label_name->setDrawBackground(false);                             \
    } while(0)

#define ADD_ELEMENT(element)                                   \
    do {                                                       \
        element->setPos(10, settings_y);                       \
        m_settings->getContainer()->addBaseUIElement(element); \
        settings_y += element->getSize().y + 20;               \
    } while(0)

#define ADD_BUTTON(button, label)                                       \
    do {                                                                \
        button->setPos(label->getSize().x + 20, label->getPos().y - 7); \
        m_settings->getContainer()->addBaseUIElement(button);           \
    } while(0)

RoomScreen::RoomScreen(Osu *osu) : OsuScreen(osu) {
    font = engine->getResourceManager()->getFont("FONT_DEFAULT");
    lfont = osu->getSubTitleFont();

    m_pauseButton = new MainMenuPauseButton(0, 0, 0, 0, "pause_btn", "");
    m_pauseButton->setClickCallback(fastdelegate::MakeDelegate(osu->m_mainMenu, &MainMenu::onPausePressed));
    addBaseUIElement(m_pauseButton);

    m_settings = new CBaseUIScrollView(0, 0, 0, 0, "room_settings");
    m_settings->setDrawFrame(false);  // it's off by 1 pixel, turn it OFF
    m_settings->setDrawBackground(true);
    m_settings->setBackgroundColor(0xdd000000);
    m_settings->setHorizontalScrolling(false);
    addBaseUIElement(m_settings);

    INIT_LABEL(m_room_name, "Multiplayer room", true);
    INIT_LABEL(m_host, "Host: None", false);  // XXX: make it an UIUserLabel

    INIT_LABEL(m_room_name_iptl, "Room name", false);
    m_room_name_ipt = new CBaseUITextbox(0, 0, m_settings->getSize().x, 40, "");
    m_room_name_ipt->setText("Multiplayer room");

    m_change_password_btn = new UIButton(osu, 0, 0, 190, 40, "change_password_btn", "Change password");
    m_change_password_btn->setColor(0xff0e94b5);
    m_change_password_btn->setUseDefaultSkin();
    m_change_password_btn->setClickCallback(fastdelegate::MakeDelegate(this, &RoomScreen::onChangePasswordClicked));

    INIT_LABEL(m_win_condition, "Win condition: Score", false);
    m_change_win_condition_btn = new UIButton(osu, 0, 0, 240, 40, "change_win_condition_btn", "Win condition: Score");
    m_change_win_condition_btn->setColor(0xff00ff00);
    m_change_win_condition_btn->setUseDefaultSkin();
    m_change_win_condition_btn->setClickCallback(
        fastdelegate::MakeDelegate(this, &RoomScreen::onChangeWinConditionClicked));

    INIT_LABEL(map_label, "Beatmap", true);
    m_select_map_btn = new UIButton(osu, 0, 0, 130, 40, "select_map_btn", "Select map");
    m_select_map_btn->setColor(0xff0e94b5);
    m_select_map_btn->setUseDefaultSkin();
    m_select_map_btn->setClickCallback(fastdelegate::MakeDelegate(this, &RoomScreen::onSelectMapClicked));

    INIT_LABEL(m_map_title, "(no map selected)", false);
    INIT_LABEL(m_map_stars, "", false);
    INIT_LABEL(m_map_attributes, "", false);
    INIT_LABEL(m_map_attributes2, "", false);

    INIT_LABEL(mods_label, "Mods", true);
    m_select_mods_btn = new UIButton(osu, 0, 0, 180, 40, "select_mods_btn", "Select mods [F1]");
    m_select_mods_btn->setColor(0xff0e94b5);
    m_select_mods_btn->setUseDefaultSkin();
    m_select_mods_btn->setClickCallback(fastdelegate::MakeDelegate(this, &RoomScreen::onSelectModsClicked));
    m_freemod = new UICheckbox(m_osu, 0, 0, 200, 50, "allow_freemod", "Freemod");
    m_freemod->setDrawFrame(false);
    m_freemod->setDrawBackground(false);
    m_freemod->setChangeCallback(fastdelegate::MakeDelegate(this, &RoomScreen::onFreemodCheckboxChanged));
    m_mods = new UIModList(&bancho.room.mods);
    INIT_LABEL(m_no_mods_selected, "No mods selected.", false);

    m_ready_btn = new UIButton(osu, 0, 0, 300, 50, "start_game_btn", "Start game");
    m_ready_btn->setColor(0xff00ff00);
    m_ready_btn->setUseDefaultSkin();
    m_ready_btn->setClickCallback(fastdelegate::MakeDelegate(this, &RoomScreen::onReadyButtonClick));
    m_ready_btn->is_loading = true;

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

    m_contextMenu = new UIContextMenu(m_osu, 50, 50, 150, 0, "", m_settings);
    addBaseUIElement(m_contextMenu);

    updateLayout(m_osu->getScreenSize());
}

void RoomScreen::draw(Graphics *g) {
    if(!m_bVisible) return;

    // XXX: Add convar for toggling room backgrounds
    SongBrowser::drawSelectedBeatmapBackgroundImage(g, m_osu, 1.0);
    OsuScreen::draw(g);

    // Update avatar visibility status
    for(auto elm : m_slotlist->getContainer()->getElements()) {
        if(elm->getName() == UString("avatar")) {
            // NOTE: Not checking horizontal visibility
            auto avatar = (UIAvatar *)elm;
            bool is_below_top = avatar->getPos().y + avatar->getSize().y >= m_slotlist->getPos().y;
            bool is_above_bottom = avatar->getPos().y <= m_slotlist->getPos().y + m_slotlist->getSize().y;
            avatar->on_screen = is_below_top && is_above_bottom;
        }
    }

    // Not technically drawing below this line, just checking for map download progress
    if(bancho.room.map_id == 0) return;
    if(bancho.room.map_id == -1) {
        m_map_title->setText("Host is selecting a map...");
        m_map_title->setSizeToContent(0, 0);
        m_ready_btn->is_loading = true;
        return;
    }

    auto search = mapset_by_mapid.find(bancho.room.map_id);
    if(search == mapset_by_mapid.end()) return;
    u32 set_id = search->second;
    if(set_id == 0) {
        auto error_str = UString::format("Could not find beatmapset for map ID %d", bancho.room.map_id);
        m_map_title->setText(error_str);
        m_map_title->setSizeToContent(0, 0);
        m_ready_btn->is_loading = true;
        return;
    }

    // Download mapset
    float progress = -1.f;
    download_beatmapset(set_id, &progress);
    if(progress == -1.f) {
        auto error_str = UString::format("Failed to download beatmapset %d :(", set_id);
        m_map_title->setText(error_str);
        m_map_title->setSizeToContent(0, 0);
        m_ready_btn->is_loading = true;
        bancho.room.map_id = 0;  // don't try downloading it again
    } else if(progress < 1.f) {
        auto text = UString::format("Downloading... %.2f%%", progress * 100.f);
        m_map_title->setText(text.toUtf8());
        m_map_title->setSizeToContent(0, 0);
        m_ready_btn->is_loading = true;
    } else {
        std::stringstream ss;
        ss << MCENGINE_DATA_DIR "maps/" << std::to_string(set_id) << "/";
        auto mapset_path = ss.str();
        // XXX: Make a permanent database for auto-downloaded songs, so we can load them like osu!.db's
        m_osu->m_songBrowser2->getDatabase()->addBeatmap(mapset_path);
        m_osu->m_songBrowser2->updateSongButtonSorting();
        debugLog("Finished loading beatmapset %d.\n", set_id);
        on_map_change(false);
    }
}

void RoomScreen::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible || m_osu->m_songBrowser2->isVisible()) return;

    const bool room_name_changed = m_room_name_ipt->getText() != bancho.room.name;
    if(bancho.room.is_host() && room_name_changed) {
        // XXX: should only update 500ms after last input
        bancho.room.name = m_room_name_ipt->getText();

        Packet packet;
        packet.id = MATCH_CHANGE_SETTINGS;
        bancho.room.pack(&packet);
        send_packet(packet);
    }

    m_pauseButton->setPaused(!m_osu->getSelectedBeatmap()->isPreviewMusicPlaying());

    m_contextMenu->mouse_update(propagate_clicks);
    if(!*propagate_clicks) return;

    OsuScreen::mouse_update(propagate_clicks);
}

void RoomScreen::onKeyDown(KeyboardEvent &key) {
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

void RoomScreen::onKeyUp(KeyboardEvent &key) {
    if(!m_bVisible || m_osu->m_songBrowser2->isVisible()) return;
    OsuScreen::onKeyUp(key);
}

void RoomScreen::onChar(KeyboardEvent &key) {
    if(!m_bVisible || m_osu->m_songBrowser2->isVisible()) return;
    OsuScreen::onChar(key);
}

void RoomScreen::onResolutionChange(Vector2 newResolution) { updateLayout(newResolution); }

CBaseUIContainer *RoomScreen::setVisible(bool visible) {
    // NOTE: Calling setVisible(false) does not quit the room! Call ragequit() instead.
    m_bVisible = visible;
    return this;
}

void RoomScreen::updateSettingsLayout(Vector2 newResolution) {
    const bool is_host = bancho.room.is_host();
    int settings_y = 10;

    m_settings->getContainer()->empty();
    m_settings->setPos(round(newResolution.x * 0.6), 0);
    m_settings->setSize(round(newResolution.x * 0.4), newResolution.y);

    // Room name (title)
    m_room_name->setText(bancho.room.name);
    m_room_name->setSizeToContent();
    ADD_ELEMENT(m_room_name);
    if(is_host) {
        ADD_BUTTON(m_change_password_btn, m_room_name);
    }

    // Host name
    if(!is_host) {
        UString host_str = "Host: None";
        if(bancho.room.host_id > 0) {
            auto host = get_user_info(bancho.room.host_id);
            host_str = UString::format("Host: %s", host->name.toUtf8());
        }
        m_host->setText(host_str);
        ADD_ELEMENT(m_host);
    }

    if(is_host) {
        // Room name (input)
        ADD_ELEMENT(m_room_name_iptl);
        m_room_name_ipt->setSize(m_settings->getSize().x - 20, 40);
        ADD_ELEMENT(m_room_name_ipt);
    }

    // Win condition
    if(bancho.room.win_condition == SCOREV1) {
        m_win_condition->setText("Win condition: Score");
    } else if(bancho.room.win_condition == ACCURACY) {
        m_win_condition->setText("Win condition: Accuracy");
    } else if(bancho.room.win_condition == COMBO) {
        m_win_condition->setText("Win condition: Combo");
    } else if(bancho.room.win_condition == SCOREV2) {
        m_win_condition->setText("Win condition: ScoreV2");
    }
    if(is_host) {
        m_change_win_condition_btn->setText(m_win_condition->getText());
        ADD_ELEMENT(m_change_win_condition_btn);
    } else {
        ADD_ELEMENT(m_win_condition);
    }

    // Beatmap
    settings_y += 10;
    ADD_ELEMENT(map_label);
    if(is_host) {
        ADD_BUTTON(m_select_map_btn, map_label);
    }
    ADD_ELEMENT(m_map_title);
    if(!m_ready_btn->is_loading) {
        ADD_ELEMENT(m_map_stars);
        ADD_ELEMENT(m_map_attributes);
        ADD_ELEMENT(m_map_attributes2);
    }

    // Mods
    settings_y += 10;
    ADD_ELEMENT(mods_label);
    if(is_host || bancho.room.freemods) {
        ADD_BUTTON(m_select_mods_btn, mods_label);
    }
    if(is_host) {
        m_freemod->setChecked(bancho.room.freemods);
        ADD_ELEMENT(m_freemod);
    }
    if(bancho.room.mods == 0) {
        ADD_ELEMENT(m_no_mods_selected);
    } else {
        m_mods->m_flags = &bancho.room.mods;
        m_mods->setSize(300, 90);
        ADD_ELEMENT(m_mods);
    }

    // Ready button
    int nb_ready = 0;
    bool is_ready = false;
    for(u8 i = 0; i < 16; i++) {
        if(bancho.room.slots[i].has_player() && bancho.room.slots[i].is_ready()) {
            nb_ready++;
            if(bancho.room.slots[i].player_id == bancho.user_id) {
                is_ready = true;
            }
        }
    }
    if(is_host && is_ready && nb_ready > 1) {
        auto force_start_str = UString::format("Force start (%d/%d)", nb_ready, bancho.room.nb_players);
        if(bancho.room.all_players_ready()) {
            force_start_str = UString("Start game");
        }
        m_ready_btn->setText(force_start_str);
        m_ready_btn->setColor(0xff00ff00);
    } else {
        m_ready_btn->setText(is_ready ? "Not ready" : "Ready!");
        m_ready_btn->setColor(is_ready ? 0xffff0000 : 0xff00ff00);
    }
    ADD_ELEMENT(m_ready_btn);

    m_settings->setScrollSizeToContent();
}

void RoomScreen::updateLayout(Vector2 newResolution) {
    setSize(newResolution);
    updateSettingsLayout(newResolution);

    const float dpiScale = Osu::getUIScale(m_osu);
    m_pauseButton->setSize(30 * dpiScale, 30 * dpiScale);
    m_pauseButton->setPos(round(newResolution.x * 0.6) - m_pauseButton->getSize().x * 2 - 10 * dpiScale,
                          m_pauseButton->getSize().y + 10 * dpiScale);

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
            auto avatar = new UIAvatar(bancho.room.slots[i].player_id, 10, y_total, SLOT_HEIGHT, SLOT_HEIGHT);
            m_slotlist->getContainer()->addBaseUIElement(avatar);

            auto user_box = new UIUserLabel(m_osu, bancho.room.slots[i].player_id, username);
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

            auto user_mods = new UIModList(&bancho.room.slots[i].mods);
            user_mods->setPos(user_box->getPos().x + user_box->getSize().x + 30, y_total);
            user_mods->setSize(350, SLOT_HEIGHT);
            m_slotlist->getContainer()->addBaseUIElement(user_mods);

            y_total += SLOT_HEIGHT + 5;
        }
    }
    m_slotlist->setScrollSizeToContent();
}

// Exit to main menu
void RoomScreen::ragequit() {
    m_bVisible = false;
    bancho.match_started = false;

    Packet packet;
    packet.id = EXIT_ROOM;
    send_packet(packet);

    m_osu->m_modSelector->resetMods();
    m_osu->m_modSelector->updateButtons();

    bancho.room = Room();
    m_osu->m_mainMenu->setVisible(true);
    m_osu->m_chat->removeChannel("#multiplayer");
    m_osu->m_chat->updateVisibility();

    ConVars::sv_cheats.setValue(!bancho.submit_scores());
}

void RoomScreen::process_beatmapset_info_response(Packet packet) {
    u32 map_id = packet.extra_int;
    if(packet.size == 0) {
        bancho.osu->m_room->mapset_by_mapid[map_id] = 0;
        return;
    }

    // {set_id}.osz|{artist}|{title}|{creator}|{status}|10.0|{last_update}|{set_id}|0|0|0|0|0
    char *saveptr = NULL;
    char *str = strtok_r((char *)packet.memory, "|", &saveptr);
    if(!str) return;
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

void RoomScreen::on_map_change(bool download) {
    // Results screen has map background and such showing, so prevent map from changing while we're on it.
    if(m_osu->m_rankingScreen->isVisible()) return;

    debugLog("Map changed to ID %d, MD5 %s: %s\n", bancho.room.map_id, bancho.room.map_md5.hash,
             bancho.room.map_name.toUtf8());
    m_ready_btn->is_loading = true;

    // Deselect current map
    m_pauseButton->setPaused(true);
    m_osu->m_songBrowser2->m_selectedBeatmap->deselect();

    if(bancho.room.map_id == 0) {
        m_map_title->setText("(no map selected)");
        m_map_title->setSizeToContent(0, 0);
        m_ready_btn->is_loading = true;
    } else {
        auto beatmap = m_osu->getSongBrowser()->getDatabase()->getBeatmapDifficulty(bancho.room.map_md5);
        if(beatmap != nullptr) {
            m_osu->m_songBrowser2->onDifficultySelected(beatmap, false);
            m_map_title->setText(bancho.room.map_name);
            m_map_title->setSizeToContent(0, 0);
            auto attributes = UString::format("AR: %.1f, CS: %.1f, HP: %.1f, OD: %.1f", beatmap->getAR(),
                                              beatmap->getCS(), beatmap->getHP(), beatmap->getOD());
            m_map_attributes->setText(attributes);
            m_map_attributes->setSizeToContent(0, 0);
            auto attributes2 = UString::format("Length: %d seconds, BPM: %d (%d - %d)", beatmap->getLengthMS() / 1000,
                                               beatmap->getMostCommonBPM(), beatmap->getMinBPM(), beatmap->getMaxBPM());
            m_map_attributes2->setText(attributes2);
            m_map_attributes2->setSizeToContent(0, 0);

            auto stars = UString::format("Star rating: %.2f*", beatmap->getStarsNomod());
            m_map_stars->setText(stars);
            m_map_stars->setSizeToContent(0, 0);
            m_ready_btn->is_loading = false;

            Packet packet;
            packet.id = MATCH_HAS_BEATMAP;
            send_packet(packet);
        } else if(download) {
            // Request beatmap info - automatically starts download
            auto path = UString::format("/web/osu-search-set.php?b=%d&u=%s&h=%s", bancho.room.map_id,
                                        bancho.username.toUtf8(), bancho.pw_md5.toUtf8());
            APIRequest request;
            request.type = GET_BEATMAPSET_INFO;
            request.path = path;
            request.mime = NULL;
            request.extra_int = (u32)bancho.room.map_id;

            send_api_request(request);

            m_map_title->setText("Loading...");
            m_map_title->setSizeToContent(0, 0);
            m_ready_btn->is_loading = true;

            Packet packet;
            packet.id = MATCH_NO_BEATMAP;
            send_packet(packet);
        } else {
            m_map_title->setText("Failed to load map. Is it catch/taiko/mania?");
            m_map_title->setSizeToContent(0, 0);
            m_ready_btn->is_loading = true;
            bancho.room.map_id = 0;  // prevents trying to load it again

            Packet packet;
            packet.id = MATCH_NO_BEATMAP;
            send_packet(packet);
        }
    }

    updateLayout(m_osu->getScreenSize());
}

void RoomScreen::on_room_joined(Room room) {
    ConVars::sv_cheats.setValue(false);

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

    RichPresence::setBanchoStatus(m_osu, room.name.toUtf8(), MULTIPLAYER);
    m_osu->m_chat->addChannel("#multiplayer");
    m_osu->m_chat->updateVisibility();

    m_osu->m_modSelector->resetMods();
    m_osu->m_modSelector->enableModsFromFlags(bancho.room.mods);
}

void RoomScreen::on_room_updated(Room room) {
    if(bancho.is_playing_a_multi_map() || !bancho.is_in_a_multi_room()) return;

    bool was_host = bancho.room.is_host();
    bool map_changed = bancho.room.map_id != room.map_id;
    bancho.room = room;

    Slot *player_slot = nullptr;
    for(int i = 0; i < 16; i++) {
        if(bancho.room.slots[i].player_id == bancho.user_id) {
            player_slot = &bancho.room.slots[i];
            break;
        }
    }
    if(player_slot == nullptr) {
        // Player got kicked
        ragequit();
        return;
    }

    if(map_changed) {
        on_map_change(true);
    }

    if(!was_host && bancho.room.is_host()) {
        m_room_name_ipt->setText(bancho.room.name);
    }

    m_osu->m_modSelector->updateButtons();
    m_osu->m_modSelector->resetMods();
    m_osu->m_modSelector->enableModsFromFlags(bancho.room.mods | player_slot->mods);

    updateLayout(m_osu->getScreenSize());
}

void RoomScreen::on_match_started(Room room) {
    bancho.room = room;
    if(m_osu->getSelectedBeatmap() == nullptr) {
        debugLog("We received MATCH_STARTED without being ready, wtf!\n");
        return;
    }

    last_packet_tms = time(NULL);

    if(m_osu->getSelectedBeatmap()->play()) {
        m_bVisible = false;
        bancho.match_started = true;
        m_osu->m_songBrowser2->m_bHasSelectedAndIsPlaying = true;
        m_osu->onPlayStart();
        m_osu->m_chat->updateVisibility();
    } else {
        ragequit();  // map failed to load
    }
}

void RoomScreen::on_match_score_updated(Packet *packet) {
    i32 update_tms = read_u32(packet);
    u8 slot_id = read_u8(packet);
    if(slot_id > 15) return;

    auto slot = &bancho.room.slots[slot_id];
    slot->last_update_tms = update_tms;
    slot->num300 = read_u16(packet);
    slot->num100 = read_u16(packet);
    slot->num50 = read_u16(packet);
    slot->num_geki = read_u16(packet);
    slot->num_katu = read_u16(packet);
    slot->num_miss = read_u16(packet);
    slot->total_score = read_u32(packet);
    slot->max_combo = read_u16(packet);
    slot->current_combo = read_u16(packet);
    slot->is_perfect = read_u8(packet);
    slot->current_hp = read_u8(packet);
    slot->tag = read_u8(packet);

    bool is_scorev2 = read_u8(packet);
    if(is_scorev2) {
        slot->sv2_combo = read_f64(packet);
        slot->sv2_bonus = read_f64(packet);
    }

    bancho.osu->m_hud->updateScoreboard(true);
}

void RoomScreen::on_all_players_loaded() {
    bancho.room.all_players_loaded = true;
    m_osu->m_chat->updateVisibility();
}

void RoomScreen::on_player_failed(i32 slot_id) {
    if(slot_id < 0 || slot_id > 15) return;
    bancho.room.slots[slot_id].died = true;
}

// All players have finished.
void RoomScreen::on_match_finished() {
    if(!bancho.is_playing_a_multi_map()) return;
    memcpy(bancho.last_scores, bancho.room.slots, sizeof(bancho.room.slots));
    m_osu->onPlayEnd(false, false);
    bancho.match_started = false;
    m_osu->m_rankingScreen->setVisible(true);
    m_osu->m_chat->updateVisibility();
}

void RoomScreen::on_all_players_skipped() { bancho.room.all_players_skipped = true; }

void RoomScreen::on_player_skip(i32 user_id) {
    for(int i = 0; i < 16; i++) {
        if(bancho.room.slots[i].player_id == user_id) {
            bancho.room.slots[i].skipped = true;
            break;
        }
    }
}

void RoomScreen::on_match_aborted() {
    if(!bancho.is_playing_a_multi_map()) return;
    m_osu->onPlayEnd(false, true);
    m_bVisible = true;
    bancho.match_started = false;
}

void RoomScreen::onClientScoreChange(bool force) {
    if(!bancho.is_playing_a_multi_map()) return;

    // Update at most once every 250ms
    bool should_update = difftime(time(NULL), last_packet_tms) > 0.25;
    if(!should_update && !force) return;

    Packet packet;
    packet.id = UPDATE_MATCH_SCORE;

    write_u32(&packet, (i32)m_osu->getSelectedBeatmap()->getTime());

    u8 slot_id = 0;
    for(u8 i = 0; i < 16; i++) {
        if(bancho.room.slots[i].player_id == bancho.user_id) {
            slot_id = i;
            break;
        }
    }
    write_u8(&packet, slot_id);

    write_u16(&packet, (u16)m_osu->getScore()->getNum300s());
    write_u16(&packet, (u16)m_osu->getScore()->getNum100s());
    write_u16(&packet, (u16)m_osu->getScore()->getNum50s());
    write_u16(&packet, (u16)m_osu->getScore()->getNum300gs());
    write_u16(&packet, (u16)m_osu->getScore()->getNum100ks());
    write_u16(&packet, (u16)m_osu->getScore()->getNumMisses());
    write_u32(&packet, (i32)m_osu->getScore()->getScore());
    write_u16(&packet, (u16)m_osu->getScore()->getCombo());
    write_u16(&packet, (u16)m_osu->getScore()->getComboMax());
    write_u8(&packet, m_osu->getScore()->getNumSliderBreaks() == 0 && m_osu->getScore()->getNumMisses() == 0 &&
                          m_osu->getScore()->getNum50s() == 0 && m_osu->getScore()->getNum100s() == 0);
    write_u8(&packet, m_osu->getSelectedBeatmap()->getHealth() * 200);
    write_u8(&packet, 0);  // 4P, not supported
    write_u8(&packet, m_osu->getModScorev2());
    send_packet(packet);

    last_packet_tms = time(NULL);
}

void RoomScreen::onReadyButtonClick() {
    if(m_ready_btn->is_loading) return;
    engine->getSound()->play(m_osu->getSkin()->getMenuHit());

    int nb_ready = 0;
    bool is_ready = false;
    for(u8 i = 0; i < 16; i++) {
        if(bancho.room.slots[i].has_player() && bancho.room.slots[i].is_ready()) {
            nb_ready++;
            if(bancho.room.slots[i].player_id == bancho.user_id) {
                is_ready = true;
            }
        }
    }
    if(bancho.room.is_host() && is_ready && nb_ready > 1) {
        Packet packet;
        packet.id = START_MATCH;
        send_packet(packet);
    } else {
        Packet packet;
        packet.id = is_ready ? MATCH_NOT_READY : MATCH_READY;
        send_packet(packet);
    }
}

void RoomScreen::onSelectModsClicked() {
    engine->getSound()->play(m_osu->getSkin()->getMenuHit());
    m_osu->m_modSelector->setVisible(true);
    m_bVisible = false;
}

void RoomScreen::onSelectMapClicked() {
    if(!bancho.room.is_host()) return;

    engine->getSound()->play(m_osu->getSkin()->getMenuHit());

    Packet packet;
    packet.id = MATCH_CHANGE_SETTINGS;
    bancho.room.map_id = -1;
    bancho.room.map_name = "";
    bancho.room.map_md5 = "";
    bancho.room.pack(&packet);
    send_packet(packet);

    m_osu->m_songBrowser2->setVisible(true);
}

void RoomScreen::onChangePasswordClicked() {
    m_osu->m_prompt->prompt("New password:", fastdelegate::MakeDelegate(this, &RoomScreen::set_new_password));
}

void RoomScreen::onChangeWinConditionClicked() {
    m_contextMenu->setVisible(false);
    m_contextMenu->begin();
    m_contextMenu->addButton("Score V1", SCOREV1);
    m_contextMenu->addButton("Score V2", SCOREV2);
    m_contextMenu->addButton("Accuracy", ACCURACY);
    m_contextMenu->addButton("Combo", COMBO);
    m_contextMenu->end(false, false);
    m_contextMenu->setPos(engine->getMouse()->getPos());
    m_contextMenu->setClickCallback(fastdelegate::MakeDelegate(this, &RoomScreen::onWinConditionSelected));
    m_contextMenu->setVisible(true);
}

void RoomScreen::onWinConditionSelected(UString win_condition_str, int win_condition) {
    bancho.room.win_condition = win_condition;

    Packet packet;
    packet.id = MATCH_CHANGE_SETTINGS;
    bancho.room.pack(&packet);
    send_packet(packet);

    updateLayout(m_osu->getScreenSize());
}

void RoomScreen::set_new_password(UString new_password) {
    bancho.room.has_password = new_password.length() > 0;
    bancho.room.password = new_password;

    Packet packet;
    packet.id = CHANGE_ROOM_PASSWORD;
    bancho.room.pack(&packet);
    send_packet(packet);
}

void RoomScreen::onFreemodCheckboxChanged(CBaseUICheckbox *checkbox) {
    if(!bancho.room.is_host()) return;

    bancho.room.freemods = checkbox->isChecked();

    Packet packet;
    packet.id = MATCH_CHANGE_SETTINGS;
    bancho.room.pack(&packet);
    send_packet(packet);

    on_room_updated(bancho.room);
}
