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
#include "Changelog.h"
#include "Chat.h"
#include "Database.h"
#include "Downloader.h"
#include "Engine.h"
#include "HUD.h"
#include "Keyboard.h"
#include "LegacyReplay.h"
#include "Lobby.h"
#include "MainMenu.h"
#include "ModSelector.h"
#include "Mouse.h"
#include "Osu.h"
#include "PromptScreen.h"
#include "RankingScreen.h"
#include "ResourceManager.h"
#include "RichPresence.h"
#include "Skin.h"
#include "SkinImage.h"
#include "SongBrowser/SongBrowser.h"
#include "SongBrowser/SongButton.h"
#include "SoundEngine.h"
#include "SpectatorScreen.h"
#include "UIAvatar.h"
#include "UIButton.h"
#include "UICheckbox.h"
#include "UIContextMenu.h"
#include "UIUserContextMenu.h"

void UIModList::draw(Graphics *g) {
    std::vector<SkinImage *> mods;

    if(*m_flags & LegacyFlags::Perfect)
        mods.push_back(osu->getSkin()->getSelectionModPerfect());
    else if(*m_flags & LegacyFlags::SuddenDeath)
        mods.push_back(osu->getSkin()->getSelectionModSuddenDeath());

    if(*m_flags & LegacyFlags::NoFail) mods.push_back(osu->getSkin()->getSelectionModNoFail());
    if(*m_flags & LegacyFlags::Easy) mods.push_back(osu->getSkin()->getSelectionModEasy());
    if(*m_flags & LegacyFlags::TouchDevice) mods.push_back(osu->getSkin()->getSelectionModTD());
    if(*m_flags & LegacyFlags::Hidden) mods.push_back(osu->getSkin()->getSelectionModHidden());
    if(*m_flags & LegacyFlags::HardRock) mods.push_back(osu->getSkin()->getSelectionModHardRock());
    if(*m_flags & LegacyFlags::Relax) mods.push_back(osu->getSkin()->getSelectionModRelax());
    if(*m_flags & LegacyFlags::Autoplay) mods.push_back(osu->getSkin()->getSelectionModAutoplay());
    if(*m_flags & LegacyFlags::SpunOut) mods.push_back(osu->getSkin()->getSelectionModSpunOut());
    if(*m_flags & LegacyFlags::Autopilot) mods.push_back(osu->getSkin()->getSelectionModAutopilot());
    if(*m_flags & LegacyFlags::Target) mods.push_back(osu->getSkin()->getSelectionModTarget());
    if(*m_flags & LegacyFlags::ScoreV2) mods.push_back(osu->getSkin()->getSelectionModScorev2());

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

RoomScreen::RoomScreen() : OsuScreen() {
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

    m_change_password_btn = new UIButton(0, 0, 190, 40, "change_password_btn", "Change password");
    m_change_password_btn->setColor(0xff0e94b5);
    m_change_password_btn->setUseDefaultSkin();
    m_change_password_btn->setClickCallback(fastdelegate::MakeDelegate(this, &RoomScreen::onChangePasswordClicked));

    INIT_LABEL(m_win_condition, "Win condition: Score", false);
    m_change_win_condition_btn = new UIButton(0, 0, 240, 40, "change_win_condition_btn", "Win condition: Score");
    m_change_win_condition_btn->setColor(0xff00ff00);
    m_change_win_condition_btn->setUseDefaultSkin();
    m_change_win_condition_btn->setClickCallback(
        fastdelegate::MakeDelegate(this, &RoomScreen::onChangeWinConditionClicked));

    INIT_LABEL(map_label, "Beatmap", true);
    m_select_map_btn = new UIButton(0, 0, 130, 40, "select_map_btn", "Select map");
    m_select_map_btn->setColor(0xff0e94b5);
    m_select_map_btn->setUseDefaultSkin();
    m_select_map_btn->setClickCallback(fastdelegate::MakeDelegate(this, &RoomScreen::onSelectMapClicked));

    INIT_LABEL(m_map_title, "(no map selected)", false);
    INIT_LABEL(m_map_stars, "", false);
    INIT_LABEL(m_map_attributes, "", false);
    INIT_LABEL(m_map_attributes2, "", false);

    INIT_LABEL(mods_label, "Mods", true);
    m_select_mods_btn = new UIButton(0, 0, 180, 40, "select_mods_btn", "Select mods [F1]");
    m_select_mods_btn->setColor(0xff0e94b5);
    m_select_mods_btn->setUseDefaultSkin();
    m_select_mods_btn->setClickCallback(fastdelegate::MakeDelegate(this, &RoomScreen::onSelectModsClicked));
    m_freemod = new UICheckbox(0, 0, 200, 50, "allow_freemod", "Freemod");
    m_freemod->setDrawFrame(false);
    m_freemod->setDrawBackground(false);
    m_freemod->setChangeCallback(fastdelegate::MakeDelegate(this, &RoomScreen::onFreemodCheckboxChanged));
    m_mods = new UIModList(&bancho.room.mods);
    INIT_LABEL(m_no_mods_selected, "No mods selected.", false);

    m_ready_btn = new UIButton(0, 0, 300, 50, "start_game_btn", "Start game");
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

    m_contextMenu = new UIContextMenu(50, 50, 150, 0, "", m_settings);
    addBaseUIElement(m_contextMenu);

    updateLayout(osu->getScreenSize());
}

RoomScreen::~RoomScreen() {
    m_settings->getContainer()->empty();
    SAFE_DELETE(m_room_name);
    SAFE_DELETE(m_change_password_btn);
    SAFE_DELETE(m_host);
    SAFE_DELETE(m_room_name_iptl);
    SAFE_DELETE(m_room_name_ipt);
    SAFE_DELETE(m_select_map_btn);
    SAFE_DELETE(m_select_mods_btn);
    SAFE_DELETE(m_change_win_condition_btn);
    SAFE_DELETE(m_win_condition);
    SAFE_DELETE(map_label);
    SAFE_DELETE(m_map_title);
    SAFE_DELETE(m_map_stars);
    SAFE_DELETE(m_map_attributes);
    SAFE_DELETE(m_map_attributes2);
    SAFE_DELETE(mods_label);
    SAFE_DELETE(m_freemod);
    SAFE_DELETE(m_no_mods_selected);
    SAFE_DELETE(m_mods);
    SAFE_DELETE(m_ready_btn);
}

void RoomScreen::draw(Graphics *g) {
    if(!m_bVisible) return;

    static i32 current_map_id = -1;
    if(bancho.room.map_id == -1 || bancho.room.map_id == 0) {
        m_map_title->setText("Host is selecting a map...");
        m_map_title->setSizeToContent(0, 0);
        m_ready_btn->is_loading = true;
    } else if(bancho.room.map_id != current_map_id) {
        float progress = -1.f;
        auto beatmap = download_beatmap(bancho.room.map_id, bancho.room.map_md5, &progress);
        if(progress == -1.f) {
            auto error_str = UString::format("Failed to download Beatmap #%d :(", bancho.room.map_id);
            m_map_title->setText(error_str);
            m_map_title->setSizeToContent(0, 0);
            m_ready_btn->is_loading = true;
        } else if(progress < 1.f) {
            auto text = UString::format("Downloading... %.2f%%", progress * 100.f);
            m_map_title->setText(text.toUtf8());
            m_map_title->setSizeToContent(0, 0);
            m_ready_btn->is_loading = true;
        } else if(beatmap != NULL) {
            current_map_id = bancho.room.map_id;
            on_map_change();
        }
    }

    // XXX: Add convar for toggling room backgrounds
    SongBrowser::drawSelectedBeatmapBackgroundImage(g, 1.0);
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
}

void RoomScreen::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible || osu->m_songBrowser2->isVisible()) return;

    const bool room_name_changed = m_room_name_ipt->getText() != bancho.room.name;
    if(bancho.room.is_host() && room_name_changed) {
        // XXX: should only update 500ms after last input
        bancho.room.name = m_room_name_ipt->getText();

        Packet packet;
        packet.id = MATCH_CHANGE_SETTINGS;
        bancho.room.pack(&packet);
        send_packet(packet);

        // Update room name in rich presence info
        RichPresence::onMultiplayerLobby();
    }

    m_pauseButton->setPaused(!osu->getSelectedBeatmap()->isPreviewMusicPlaying());

    m_contextMenu->mouse_update(propagate_clicks);
    if(!*propagate_clicks) return;

    OsuScreen::mouse_update(propagate_clicks);
}

void RoomScreen::onKeyDown(KeyboardEvent &key) {
    if(!m_bVisible || osu->m_songBrowser2->isVisible()) return;

    if(key.getKeyCode() == KEY_ESCAPE) {
        key.consume();
        ragequit();
        return;
    }

    if(key.getKeyCode() == KEY_F1) {
        key.consume();
        if(bancho.room.freemods || bancho.room.is_host()) {
            osu->m_modSelector->setVisible(!osu->m_modSelector->isVisible());
            m_bVisible = !osu->m_modSelector->isVisible();
        }
        return;
    }

    OsuScreen::onKeyDown(key);
}

void RoomScreen::onKeyUp(KeyboardEvent &key) {
    if(!m_bVisible || osu->m_songBrowser2->isVisible()) return;
    OsuScreen::onKeyUp(key);
}

void RoomScreen::onChar(KeyboardEvent &key) {
    if(!m_bVisible || osu->m_songBrowser2->isVisible()) return;
    OsuScreen::onChar(key);
}

void RoomScreen::onResolutionChange(Vector2 newResolution) { updateLayout(newResolution); }

CBaseUIContainer *RoomScreen::setVisible(bool visible) {
    if(m_bVisible == visible) return this;

    // NOTE: Calling setVisible(false) does not quit the room! Call ragequit() instead.
    m_bVisible = visible;

    if(visible) {
        engine->getSound()->play(osu->getSkin()->m_menuBack);
    }

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

    const float dpiScale = Osu::getUIScale();
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

            auto user_box = new UIUserLabel(bancho.room.slots[i].player_id, username);
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
void RoomScreen::ragequit(bool play_sound) {
    m_bVisible = false;
    bancho.match_started = false;

    Packet packet;
    packet.id = EXIT_ROOM;
    send_packet(packet);

    osu->m_modSelector->resetMods();
    osu->m_modSelector->updateButtons();

    bancho.room = Room();
    osu->m_mainMenu->setVisible(true);
    osu->m_chat->removeChannel("#multiplayer");
    osu->m_chat->updateVisibility();

    Replay::Mods::use(osu->previous_mods);

    if(play_sound) {
        engine->getSound()->play(osu->getSkin()->m_menuBack);
    }
}

void RoomScreen::on_map_change() {
    // Results screen has map background and such showing, so prevent map from changing while we're on it.
    if(osu->m_rankingScreen->isVisible()) return;

    debugLog("Map changed to ID %d, MD5 %s: %s\n", bancho.room.map_id, bancho.room.map_md5.hash,
             bancho.room.map_name.toUtf8());
    m_ready_btn->is_loading = true;

    // Deselect current map
    m_pauseButton->setPaused(true);
    osu->m_songBrowser2->m_beatmap->deselect();

    if(bancho.room.map_id == 0) {
        m_map_title->setText("(no map selected)");
        m_map_title->setSizeToContent(0, 0);
        m_ready_btn->is_loading = true;
    } else {
        auto beatmap = db->getBeatmapDifficulty(bancho.room.map_md5);
        if(beatmap != NULL) {
            osu->m_songBrowser2->onDifficultySelected(beatmap, false);
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

            // TODO @kiwec: calc actual star rating on the fly and update
            auto stars = UString::format("Star rating: %.2f*", beatmap->getStarsNomod());
            m_map_stars->setText(stars);
            m_map_stars->setSizeToContent(0, 0);
            m_ready_btn->is_loading = false;

            Packet packet;
            packet.id = MATCH_HAS_BEATMAP;
            send_packet(packet);
        } else {
            Packet packet;
            packet.id = MATCH_NO_BEATMAP;
            send_packet(packet);
        }
    }

    updateLayout(osu->getScreenSize());
}

void RoomScreen::on_room_joined(Room room) {
    bancho.room = room;
    debugLog("Joined room #%d\nPlayers:\n", room.id);
    for(int i = 0; i < 16; i++) {
        if(room.slots[i].has_player()) {
            auto user_info = get_user_info(room.slots[i].player_id);
            debugLog("- %s\n", user_info->name.toUtf8());
        }
    }

    on_map_change();

    // Close all screens and stop any activity the player is in
    stop_spectating();
    if(osu->getSelectedBeatmap()->isPlaying()) {
        osu->getSelectedBeatmap()->stop(true);
    }
    osu->m_rankingScreen->setVisible(false);
    osu->m_songBrowser2->setVisible(false);
    osu->m_changelog->setVisible(false);
    osu->m_mainMenu->setVisible(false);
    osu->m_lobby->setVisible(false);

    updateLayout(osu->getScreenSize());
    m_bVisible = true;

    RichPresence::setBanchoStatus(room.name.toUtf8(), MULTIPLAYER);
    RichPresence::onMultiplayerLobby();
    osu->m_chat->addChannel("#multiplayer");
    osu->m_chat->updateVisibility();

    osu->previous_mods = Replay::Mods::from_cvars();

    osu->m_modSelector->resetMods();
    osu->m_modSelector->enableModsFromFlags(bancho.room.mods);
}

void RoomScreen::on_room_updated(Room room) {
    if(bancho.is_playing_a_multi_map() || !bancho.is_in_a_multi_room()) return;

    if(bancho.room.nb_players < room.nb_players) {
        engine->getSound()->play(osu->getSkin()->m_roomJoined);
    } else if(bancho.room.nb_players > room.nb_players) {
        engine->getSound()->play(osu->getSkin()->m_roomQuit);
    }
    if(bancho.room.nb_ready() < room.nb_ready()) {
        engine->getSound()->play(osu->getSkin()->m_roomReady);
    } else if(bancho.room.nb_ready() > room.nb_ready()) {
        engine->getSound()->play(osu->getSkin()->m_roomNotReady);
    }
    if(!bancho.room.all_players_ready() && room.all_players_ready()) {
        engine->getSound()->play(osu->getSkin()->m_matchConfirm);
    }

    bool was_host = bancho.room.is_host();
    bool map_changed = bancho.room.map_id != room.map_id;
    bancho.room = room;

    Slot *player_slot = NULL;
    for(int i = 0; i < 16; i++) {
        if(bancho.room.slots[i].player_id == bancho.user_id) {
            player_slot = &bancho.room.slots[i];
            break;
        }
    }
    if(player_slot == NULL) {
        // Player got kicked
        ragequit();
        return;
    }

    if(map_changed) {
        on_map_change();
    }

    if(!was_host && bancho.room.is_host()) {
        if(m_room_name_ipt->getText() != bancho.room.name) {
            m_room_name_ipt->setText(bancho.room.name);

            // Update room name in rich presence info
            RichPresence::onMultiplayerLobby();
        }
    }

    osu->m_modSelector->updateButtons();
    osu->m_modSelector->resetMods();
    osu->m_modSelector->enableModsFromFlags(bancho.room.mods | player_slot->mods);

    updateLayout(osu->getScreenSize());
}

void RoomScreen::on_match_started(Room room) {
    bancho.room = room;
    if(osu->getSelectedBeatmap() == NULL) {
        debugLog("We received MATCH_STARTED without being ready, wtf!\n");
        return;
    }

    last_packet_tms = time(NULL);

    if(osu->getSelectedBeatmap()->play()) {
        m_bVisible = false;
        bancho.match_started = true;
        osu->m_songBrowser2->m_bHasSelectedAndIsPlaying = true;
        osu->m_chat->updateVisibility();

        engine->getSound()->play(osu->getSkin()->m_matchStart);
    } else {
        ragequit();  // map failed to load
    }
}

void RoomScreen::on_match_score_updated(Packet *packet) {
    auto frame = read<ScoreFrame>(packet);
    if(frame.slot_id > 15) return;

    auto slot = &bancho.room.slots[frame.slot_id];
    slot->last_update_tms = frame.time;
    slot->num300 = frame.num300;
    slot->num100 = frame.num100;
    slot->num50 = frame.num50;
    slot->num_geki = frame.num_geki;
    slot->num_katu = frame.num_katu;
    slot->num_miss = frame.num_miss;
    slot->total_score = frame.total_score;
    slot->max_combo = frame.max_combo;
    slot->current_combo = frame.current_combo;
    slot->is_perfect = frame.is_perfect;
    slot->current_hp = frame.current_hp;
    slot->tag = frame.tag;

    bool is_scorev2 = read<u8>(packet);
    if(is_scorev2) {
        slot->sv2_combo = read<f64>(packet);
        slot->sv2_bonus = read<f64>(packet);
    }

    osu->m_hud->updateScoreboard(true);
}

void RoomScreen::on_all_players_loaded() {
    bancho.room.all_players_loaded = true;
    osu->m_chat->updateVisibility();
}

void RoomScreen::on_player_failed(i32 slot_id) {
    if(slot_id < 0 || slot_id > 15) return;
    bancho.room.slots[slot_id].died = true;
}

FinishedScore RoomScreen::get_approximate_score() {
    FinishedScore score;
    score.player_id = bancho.user_id;
    score.playerName = bancho.username.toUtf8();
    score.diff2 = osu->getSelectedBeatmap()->getSelectedDifficulty2();

    for(int i = 0; i < 16; i++) {
        auto slot = &bancho.room.slots[i];
        if(slot->player_id != bancho.user_id) continue;

        score.mods = Replay::Mods::from_legacy(slot->mods);
        score.passed = !slot->died;
        score.unixTimestamp = slot->last_update_tms;
        score.num300s = slot->num300;
        score.num100s = slot->num100;
        score.num50s = slot->num50;
        score.numGekis = slot->num_geki;
        score.numKatus = slot->num_katu;
        score.numMisses = slot->num_miss;
        score.score = slot->total_score;
        score.comboMax = slot->max_combo;
        score.perfect = slot->is_perfect;
    }

    score.grade = score.calculate_grade();

    return score;
}

// All players have finished.
void RoomScreen::on_match_finished() {
    if(!bancho.is_playing_a_multi_map()) return;
    memcpy(bancho.last_scores, bancho.room.slots, sizeof(bancho.room.slots));

    osu->onPlayEnd(get_approximate_score(), false, false);

    bancho.match_started = false;
    osu->m_rankingScreen->setVisible(true);
    osu->m_chat->updateVisibility();

    // Display room presence instead of map again
    RichPresence::onMultiplayerLobby();
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
    osu->onPlayEnd(get_approximate_score(), false, true);
    m_bVisible = true;
    bancho.match_started = false;

    // Display room presence instead of map again
    RichPresence::onMultiplayerLobby();
}

void RoomScreen::onClientScoreChange(bool force) {
    if(!bancho.is_playing_a_multi_map()) return;

    // Update at most once every 250ms
    bool should_update = difftime(time(NULL), last_packet_tms) > 0.25;
    if(!should_update && !force) return;

    Packet packet;
    packet.id = UPDATE_MATCH_SCORE;
    write<ScoreFrame>(&packet, ScoreFrame::get());
    send_packet(packet);

    last_packet_tms = time(NULL);
}

void RoomScreen::onReadyButtonClick() {
    if(m_ready_btn->is_loading) return;
    engine->getSound()->play(osu->getSkin()->getMenuHit());

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
    engine->getSound()->play(osu->getSkin()->getMenuHit());
    osu->m_modSelector->setVisible(true);
    m_bVisible = false;
}

void RoomScreen::onSelectMapClicked() {
    if(!bancho.room.is_host()) return;

    engine->getSound()->play(osu->getSkin()->getMenuHit());

    Packet packet;
    packet.id = MATCH_CHANGE_SETTINGS;
    bancho.room.map_id = -1;
    bancho.room.map_name = "";
    bancho.room.map_md5 = "";
    bancho.room.pack(&packet);
    send_packet(packet);

    osu->m_songBrowser2->setVisible(true);
}

void RoomScreen::onChangePasswordClicked() {
    osu->m_prompt->prompt("New password:", fastdelegate::MakeDelegate(this, &RoomScreen::set_new_password));
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

    updateLayout(osu->getScreenSize());
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
