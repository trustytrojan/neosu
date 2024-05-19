#include "SpectatorScreen.h"

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoProtocol.h"
#include "BanchoUsers.h"
#include "Beatmap.h"
#include "CBaseUILabel.h"
#include "Changelog.h"
#include "Database.h"
#include "Downloader.h"
#include "Engine.h"
#include "Lobby.h"
#include "MainMenu.h"
#include "ModSelector.h"
#include "NotificationOverlay.h"
#include "Osu.h"
#include "PromptScreen.h"
#include "ResourceManager.h"
#include "RoomScreen.h"
#include "SongBrowser/SongBrowser.h"
#include "UserCard.h"

i32 current_map_id = 0;

void spectate_by_username(UString username) {
    auto user = find_user(username);
    if(user == nullptr) {
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
    if(osu->m_room->isVisible()) osu->m_room->ragequit();

    osu->m_spectatorScreen->setVisible(true);
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
}

SpectatorScreen::SpectatorScreen() {
    font = engine->getResourceManager()->getFont("FONT_DEFAULT");
    lfont = osu->getSubTitleFont();

    m_pauseButton = new MainMenuPauseButton(0, 0, 0, 0, "pause_btn", "");
    m_pauseButton->setClickCallback(fastdelegate::MakeDelegate(osu->m_mainMenu, &MainMenu::onPausePressed));
    addBaseUIElement(m_pauseButton);

    INIT_LABEL(m_spectating, "Spectating", true);
    addBaseUIElement(m_spectating);

    m_userCard = new UserCard(0);
    addBaseUIElement(m_userCard);

    INIT_LABEL(m_status, "...", false);
    addBaseUIElement(m_status);

    // TODO @kiwec: stop spectating button
}

void SpectatorScreen::mouse_update(bool *propagate_clicks) {
    if(bancho.spectated_player_id == 0 || osu->getSelectedBeatmap()->isPlaying()) return;

    static i32 prev_player_id = 0;
    if(bancho.spectated_player_id != prev_player_id) {
        auto user_info = get_user_info(bancho.spectated_player_id);
        m_spectating->setText(UString::format("Spectating %s", user_info->name.toUtf8()));

        m_userCard->setID(bancho.spectated_player_id);
        prev_player_id = bancho.spectated_player_id;
    }

    OsuScreen::mouse_update(propagate_clicks);
}

void SpectatorScreen::draw(Graphics *g) {
    if(bancho.spectated_player_id == 0 || osu->getSelectedBeatmap()->isPlaying()) return;

    auto user_info = get_user_info(bancho.spectated_player_id);
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
        } else if(beatmap != nullptr) {
            current_map_id = user_info->map_id;
            osu->m_songBrowser2->onDifficultySelected(beatmap, false);
            osu->getSelectedBeatmap()->spectate();
        }
    }

    // XXX: Add convar for toggling spectator background
    SongBrowser::drawSelectedBeatmapBackgroundImage(g, 1.0);
    OsuScreen::draw(g);
}
