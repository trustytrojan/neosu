#include "SpectatorScreen.h"

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoProtocol.h"
#include "BanchoUsers.h"
#include "Beatmap.h"
#include "CBaseUILabel.h"
#include "CBaseUIScrollView.h"
#include "Changelog.h"
#include "Database.h"
#include "Downloader.h"
#include "Engine.h"
#include "Keyboard.h"
#include "Lobby.h"
#include "MainMenu.h"
#include "ModSelector.h"
#include "NotificationOverlay.h"
#include "Osu.h"
#include "PromptScreen.h"
#include "ResourceManager.h"
#include "RoomScreen.h"
#include "Skin.h"
#include "SongBrowser/SongBrowser.h"
#include "SoundEngine.h"
#include "UIButton.h"
#include "UserCard.h"

i32 current_map_id = 0;
i32 current_user_id = 0;

void spectate_by_username(UString username) {
    auto user = find_user(username);
    if(user == NULL) {
        debugLog("Couldn't find user \"%s\"!", username.toUtf8());
        return;
    }

    debugLog("Spectating %s (user %d)...\n", username.toUtf8(), user->user_id);
    start_spectating(user->user_id);
}

ConVar spectate_cmd("spectate", FCVAR_HIDDEN, spectate_by_username);

#define INIT_LABEL(label_name, default_text, is_big)                      \
    do {                                                                  \
        label_name = new CBaseUILabel(0, 0, 0, 0, "label", default_text); \
        label_name->setFont(is_big ? lfont : font);                       \
        label_name->setSizeToContent(0, 0);                               \
        label_name->setDrawFrame(false);                                  \
        label_name->setDrawBackground(false);                             \
    } while(0)

void start_spectating(i32 user_id) {
    stop_spectating();

    Packet packet;
    packet.id = START_SPECTATING;
    write<i32>(&packet, user_id);
    send_packet(packet);

    bancho.spectated_player_id = user_id;
    current_map_id = 0;

    auto user_info = get_user_info(user_id);
    auto notif = UString::format("Started spectating %s", user_info->name.toUtf8());
    osu->getNotificationOverlay()->addNotification(notif);

    osu->m_prompt->setVisible(false);
    osu->m_modSelector->setVisible(false);
    osu->m_songBrowser2->setVisible(false);
    osu->m_lobby->setVisible(false);
    osu->m_changelog->setVisible(false);
    osu->m_mainMenu->setVisible(false);
    if(osu->m_room->isVisible()) osu->m_room->ragequit(false);

    osu->m_spectatorScreen->setVisible(true);
    engine->getSound()->play(osu->getSkin()->m_menuHit);
}

void stop_spectating() {
    if(bancho.spectated_player_id == 0) return;

    if(osu->getSelectedBeatmap()->isPlaying()) {
        osu->getSelectedBeatmap()->stop(true);
    }

    auto user_info = get_user_info(bancho.spectated_player_id);
    bancho.spectated_player_id = 0;
    current_map_id = 0;

    Packet packet;
    packet.id = STOP_SPECTATING;
    send_packet(packet);

    auto notif = UString::format("Stopped spectating %s", user_info->name.toUtf8());
    osu->getNotificationOverlay()->addNotification(notif);

    osu->m_spectatorScreen->setVisible(false);
    osu->m_mainMenu->setVisible(true);
    engine->getSound()->play(osu->getSkin()->m_menuBack);
}

SpectatorScreen::SpectatorScreen() {
    font = engine->getResourceManager()->getFont("FONT_DEFAULT");
    lfont = osu->getSubTitleFont();

    m_pauseButton = new MainMenuPauseButton(0, 0, 0, 0, "pause_btn", "");
    m_pauseButton->setClickCallback(fastdelegate::MakeDelegate(osu->m_mainMenu, &MainMenu::onPausePressed));
    addBaseUIElement(m_pauseButton);

    m_background = new CBaseUIScrollView(0, 0, 0, 0, "spectator_bg");
    m_background->setDrawFrame(true);
    m_background->setDrawBackground(true);
    m_background->setBackgroundColor(0xdd000000);
    m_background->setHorizontalScrolling(false);
    m_background->setVerticalScrolling(false);
    addBaseUIElement(m_background);

    INIT_LABEL(m_spectating, "Spectating", true);
    m_background->getContainer()->addBaseUIElement(m_spectating);

    m_userCard = new UserCard(0);
    m_background->getContainer()->addBaseUIElement(m_userCard);

    INIT_LABEL(m_status, "...", false);
    m_background->getContainer()->addBaseUIElement(m_status);

    m_stop_btn = new UIButton(0, 0, 190, 40, "stop_spec_btn", "Stop spectating");
    m_stop_btn->setColor(0xff00ff00);
    m_stop_btn->setUseDefaultSkin();
    m_stop_btn->setClickCallback(fastdelegate::MakeDelegate(this, &SpectatorScreen::onStopSpectatingClicked));
    addBaseUIElement(m_stop_btn);
}

void SpectatorScreen::draw(Graphics *g) {
    if(!isVisible()) return;

    auto user_info = get_user_info(bancho.spectated_player_id);

    if(bancho.spectated_player_id != current_user_id) {
        m_spectating->setText(UString::format("Spectating %s", user_info->name.toUtf8()));
        m_userCard->setID(bancho.spectated_player_id);
        current_user_id = bancho.spectated_player_id;
    }

    if(user_info->mode != STANDARD) {
        m_status->setText(UString::format("%s is playing minigames", user_info->name.toUtf8()));
    } else if(user_info->map_id == -1 || user_info->map_id == 0) {
        m_status->setText(UString::format("%s is picking a map...", user_info->name.toUtf8()));
    } else if(user_info->map_id != current_map_id) {
        float progress = -1.f;
        auto beatmap = download_beatmap(user_info->map_id, user_info->map_md5, &progress);
        if(progress == -1.f) {
            auto error_str = UString::format("Failed to download Beatmap #%d :(", user_info->map_id);
            m_status->setText(error_str);
        } else if(progress < 1.f) {
            auto text = UString::format("Downloading map... %.2f%%", progress * 100.f);
            m_status->setText(text);
        } else if(beatmap != NULL) {
            current_map_id = user_info->map_id;
            osu->m_songBrowser2->onDifficultySelected(beatmap, false);
            osu->getSelectedBeatmap()->spectate();
        }
    }

    const float dpiScale = Osu::getUIScale();
    auto resolution = osu->getScreenSize();
    setSize(resolution);

    m_pauseButton->setSize(30 * dpiScale, 30 * dpiScale);
    m_pauseButton->setPos(resolution.x - m_pauseButton->getSize().x * 2 - 10 * dpiScale,
                          m_pauseButton->getSize().y + 10 * dpiScale);
    m_pauseButton->setPaused(!osu->getSelectedBeatmap()->isPreviewMusicPlaying());

    m_background->setPos(resolution.x * 0.2, resolution.y + 50 * dpiScale);
    m_background->setSize(resolution.x * 0.6, resolution.y * 0.6 - 110 * dpiScale);
    resolution = m_background->getSize();
    {
        m_spectating->setSizeToContent();
        m_spectating->setPos(resolution.x / 2.f - m_spectating->getSize().x / 2.f, resolution.y / 2.f - 100 * dpiScale);

        m_userCard->setPos(resolution.x / 2.f - 100 * dpiScale, resolution.y / 2.f + 20 * dpiScale);

        m_status->setSizeToContent();
        m_status->setPos(resolution.x / 2.f - m_status->getSize().x / 2.f, resolution.y / 2.f + 150 * dpiScale);
    }
    m_background->setScrollSizeToContent();

    auto stop_pos = m_background->getPos();
    stop_pos.x += m_background->getSize().x / 2.f;
    stop_pos.y += 20 * dpiScale;
    m_stop_btn->setPos(stop_pos);

    // XXX: Add convar for toggling spectator background
    SongBrowser::drawSelectedBeatmapBackgroundImage(g, 1.0);

    OsuScreen::draw(g);
    m_background->getContainer()->draw_debug(g);
}

bool SpectatorScreen::isVisible() {
    return (bancho.spectated_player_id != 0) && !osu->getSelectedBeatmap()->isPlaying();
}

void SpectatorScreen::onKeyDown(KeyboardEvent &key) {
    if(!isVisible()) return;

    if(key.getKeyCode() == KEY_ESCAPE) {
        key.consume();
        onStopSpectatingClicked();
        return;
    }

    OsuScreen::onKeyDown(key);
}

void SpectatorScreen::onStopSpectatingClicked() { stop_spectating(); }
