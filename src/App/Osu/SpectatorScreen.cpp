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

    osu->prompt->setVisible(false);
    osu->modSelector->setVisible(false);
    osu->songBrowser2->setVisible(false);
    osu->lobby->setVisible(false);
    osu->changelog->setVisible(false);
    osu->mainMenu->setVisible(false);
    if(osu->room->isVisible()) osu->room->ragequit(false);

    osu->spectatorScreen->setVisible(true);
    engine->getSound()->play(osu->getSkin()->menuHit);
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

    osu->spectatorScreen->setVisible(false);
    osu->mainMenu->setVisible(true);
    engine->getSound()->play(osu->getSkin()->menuBack);
}

SpectatorScreen::SpectatorScreen() {
    this->font = engine->getResourceManager()->getFont("FONT_DEFAULT");
    this->lfont = osu->getSubTitleFont();

    this->pauseButton = new MainMenuPauseButton(0, 0, 0, 0, "pause_btn", "");
    this->pauseButton->setClickCallback(fastdelegate::MakeDelegate(osu->mainMenu, &MainMenu::onPausePressed));
    this->addBaseUIElement(this->pauseButton);

    this->background = new CBaseUIScrollView(0, 0, 0, 0, "spectator_bg");
    this->background->setDrawFrame(true);
    this->background->setDrawBackground(true);
    this->background->setBackgroundColor(0xdd000000);
    this->background->setHorizontalScrolling(false);
    this->background->setVerticalScrolling(false);
    this->addBaseUIElement(this->background);

    INIT_LABEL(this->spectating, "Spectating", true);
    this->background->getContainer()->addBaseUIElement(this->spectating);

    this->userCard = new UserCard(0);
    this->background->getContainer()->addBaseUIElement(this->userCard);

    INIT_LABEL(this->status, "...", false);
    this->background->getContainer()->addBaseUIElement(this->status);

    this->stop_btn = new UIButton(0, 0, 190, 40, "stop_spec_btn", "Stop spectating");
    this->stop_btn->setColor(0xff00ff00);
    this->stop_btn->setUseDefaultSkin();
    this->stop_btn->setClickCallback(fastdelegate::MakeDelegate(this, &SpectatorScreen::onStopSpectatingClicked));
    this->addBaseUIElement(this->stop_btn);
}

void SpectatorScreen::draw(Graphics *g) {
    if(!this->isVisible()) return;

    auto user_info = get_user_info(bancho.spectated_player_id);

    if(bancho.spectated_player_id != current_user_id) {
        this->spectating->setText(UString::format("Spectating %s", user_info->name.toUtf8()));
        this->userCard->setID(bancho.spectated_player_id);
        current_user_id = bancho.spectated_player_id;
    }

    if(user_info->mode != STANDARD) {
        this->status->setText(UString::format("%s is playing minigames", user_info->name.toUtf8()));
    } else if(user_info->map_id == -1 || user_info->map_id == 0) {
        this->status->setText(UString::format("%s is picking a map...", user_info->name.toUtf8()));
    } else if(user_info->map_id != this->current_map_id) {
        float progress = -1.f;
        auto beatmap = download_beatmap(user_info->map_id, user_info->map_md5, &progress);
        if(progress == -1.f) {
            auto error_str = UString::format("Failed to download Beatmap #%d :(", user_info->map_id);
            this->status->setText(error_str);
        } else if(progress < 1.f) {
            auto text = UString::format("Downloading map... %.2f%%", progress * 100.f);
            this->status->setText(text);
        } else if(beatmap != NULL) {
            this->current_map_id = user_info->map_id;
            osu->songBrowser2->onDifficultySelected(beatmap, false);
            osu->getSelectedBeatmap()->spectate();
        }
    }

    const float dpiScale = Osu::getUIScale();
    auto resolution = osu->getScreenSize();
    this->setSize(resolution);

    this->pauseButton->setSize(30 * dpiScale, 30 * dpiScale);
    this->pauseButton->setPos(resolution.x - this->pauseButton->getSize().x * 2 - 10 * dpiScale,
                              this->pauseButton->getSize().y + 10 * dpiScale);
    this->pauseButton->setPaused(!osu->getSelectedBeatmap()->isPreviewMusicPlaying());

    this->background->setPos(resolution.x * 0.2, resolution.y + 50 * dpiScale);
    this->background->setSize(resolution.x * 0.6, resolution.y * 0.6 - 110 * dpiScale);
    resolution = this->background->getSize();
    {
        this->spectating->setSizeToContent();
        this->spectating->setPos(resolution.x / 2.f - this->spectating->getSize().x / 2.f,
                                 resolution.y / 2.f - 100 * dpiScale);

        this->userCard->setPos(resolution.x / 2.f - 100 * dpiScale, resolution.y / 2.f + 20 * dpiScale);

        this->status->setSizeToContent();
        this->status->setPos(resolution.x / 2.f - this->status->getSize().x / 2.f, resolution.y / 2.f + 150 * dpiScale);
    }
    this->background->setScrollSizeToContent();

    auto stop_pos = this->background->getPos();
    stop_pos.x += this->background->getSize().x / 2.f;
    stop_pos.y += 20 * dpiScale;
    this->stop_btn->setPos(stop_pos);

    // XXX: Add convar for toggling spectator background
    SongBrowser::drawSelectedBeatmapBackgroundImage(g, 1.0);

    OsuScreen::draw(g);
    this->background->getContainer()->draw_debug(g);
}

bool SpectatorScreen::isVisible() {
    return (bancho.spectated_player_id != 0) && !osu->getSelectedBeatmap()->isPlaying();
}

void SpectatorScreen::onKeyDown(KeyboardEvent &key) {
    if(!this->isVisible()) return;

    if(key.getKeyCode() == KEY_ESCAPE) {
        key.consume();
        this->onStopSpectatingClicked();
        return;
    }

    OsuScreen::onKeyDown(key);
}

void SpectatorScreen::onStopSpectatingClicked() { stop_spectating(); }
