#include "Changelog.h"

#include "CBaseUIButton.h"
#include "CBaseUIContainer.h"
#include "CBaseUILabel.h"
#include "CBaseUIScrollView.h"
#include "ConVar.h"
#include "Engine.h"
#include "HUD.h"
#include "Mouse.h"
#include "OptionsMenu.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "Skin.h"
#include "SoundEngine.h"

Changelog::Changelog() : ScreenBackable() {
    setPos(-1, -1);
    m_scrollView = new CBaseUIScrollView(-1, -1, 0, 0, "");
    m_scrollView->setVerticalScrolling(true);
    m_scrollView->setHorizontalScrolling(true);
    m_scrollView->setDrawBackground(false);
    m_scrollView->setDrawFrame(false);
    m_scrollView->setScrollResistance(0);
    addBaseUIElement(m_scrollView);

    std::vector<CHANGELOG> changelogs;

    CHANGELOG latest;
    latest.title = UString::format("%.2f (%s, %s)", cv_version.getFloat(), __DATE__, __TIME__);
    latest.changes.push_back(
        "- Added \"tooearly.wav\" and \"toolate.wav\" hitsounds, which play when you hit too early or too late (if "
        "your skin has them)");
    latest.changes.push_back("- Added \"Actual Flashlight\" mod");
    latest.changes.push_back("- Added keybind to open skin selection menu");
    latest.changes.push_back("- Added slider instafade setting");
    latest.changes.push_back("- Fixed local scores not saving avatar");
    latest.changes.push_back("- Fixed map settings (like local offset) not getting saved");
    latest.changes.push_back("- Fixed Nightcore getting auto-selected instead of Double Time in some cases");
    latest.changes.push_back("- Improved loudness normalization");
    latest.changes.push_back("- Merged pp calculation optimizations from McOsu");
    latest.changes.push_back("- Optimized database loading");
    latest.changes.push_back("- Removed DT/HT mods (use speed slider instead)");
    latest.changes.push_back("- Removed custom HP drain (use nofail instead)");
    latest.changes.push_back("- Removed mandala mod, random mod");
    latest.changes.push_back("- Removed rosu-pp");
    latest.changes.push_back("- Updated osu! version to b20240820.1");
    latest.changes.push_back("- Linux: Fixed \"Skin.ini\" failing to load");
    changelogs.push_back(latest);

    CHANGELOG v35_09;
    v35_09.title = "35.09 (2024-07-03)";
    v35_09.changes.push_back("- Added keybind to toggle current map background");
    v35_09.changes.push_back("- Fixed beatmaps not getting selected properly in some cases");
    v35_09.changes.push_back("- Fixed crash when osu! folder couldn't be found");
    v35_09.changes.push_back("- Fixed mod selection not being restored properly");
    v35_09.changes.push_back("- Fixed skin selection menu being drawn behind back button");
    changelogs.push_back(v35_09);

    CHANGELOG v35_08;
    v35_08.title = "35.08 (2024-06-28)";
    v35_08.changes.push_back("- Added ability to import .osk and .osz files (drop them onto the neosu window)");
    v35_08.changes.push_back(
        "- Added persistent map database (downloaded or imported maps stay after restarting the game)");
    v35_08.changes.push_back("- Added skin folder");
    v35_08.changes.push_back("- Now publishing 32-bit releases (for PCs running Windows 7)");
    v35_08.changes.push_back("- Fixed songs failing to restart");
    changelogs.push_back(v35_08);

    CHANGELOG v35_07;
    v35_07.title = "35.07 (2024-06-27)";
    v35_07.changes.push_back("- Added sort_skins_by_name convar");
    v35_07.changes.push_back("- Added setting to prevent servers from replacing the main menu logo");
    v35_07.changes.push_back("- Chat: added missing chat commands");
    v35_07.changes.push_back("- Chat: added missing keyboard shortcuts");
    v35_07.changes.push_back("- Chat: added support for user links");
    v35_07.changes.push_back("- Chat: improved map link support");
    v35_07.changes.push_back("- Fixed freeze when switching between songs in song browser");
    v35_07.changes.push_back("- Lowered audio latency for default (not ASIO/WASAPI) output");
    changelogs.push_back(v35_07);

    CHANGELOG v35_06;
    v35_06.title = "35.06 (2024-06-17)";
    v35_06.changes.push_back("- Added cursor trail customization settings");
    v35_06.changes.push_back("- Added instafade checkbox");
    v35_06.changes.push_back("- Added more UI sounds");
    v35_06.changes.push_back("- Added submit_after_pause convar");
    v35_06.changes.push_back("- Chat: added support for /me command");
    v35_06.changes.push_back("- Chat: added support for links");
    v35_06.changes.push_back("- Chat: added support for map links (auto-downloads)");
    v35_06.changes.push_back("- Chat: added support for multiplayer invite links");
    v35_06.changes.push_back("- FPS counter will now display worst frametime instead of current frametime");
    v35_06.changes.push_back("- Improved song browser performance");
    v35_06.changes.push_back("- Skins are now sorted alphabetically, ignoring meme characters");
    v35_06.changes.push_back("- Unlocked osu_drain_kill convar");
    changelogs.push_back(v35_06);

    CHANGELOG v35_05;
    v35_05.title = "35.05 (2024-06-13)";
    v35_05.changes.push_back("- Fixed Artist/Creator/Title sorting to be in A-Z order");
    v35_05.changes.push_back("- Improved sound engine reliability");
    v35_05.changes.push_back("- Removed herobrine");
    changelogs.push_back(v35_05);

    CHANGELOG v35_04;
    v35_04.title = "35.04 (2024-06-11)";
    v35_04.changes.push_back("- Changed \"Open Skins folder\" button to open the currently selected skin's folder");
    v35_04.changes.push_back("- Fixed master volume control not working on exclusive WASAPI");
    v35_04.changes.push_back("- Fixed screenshots failing to save");
    v35_04.changes.push_back("- Fixed skins with non-ANSI folder names failing to open on Windows");
    v35_04.changes.push_back("- Fixed sliderslide and spinnerspin sounds not looping");
    v35_04.changes.push_back("- Improved sound engine reliability");
    v35_04.changes.push_back("- Re-added strain graphs");
    v35_04.changes.push_back(
        "- Removed sliderhead fadeout animation (set osu_slider_sliderhead_fadeout to 1 for old behavior)");
    changelogs.push_back(v35_04);

    CHANGELOG v35_03;
    v35_03.title = "35.03 (2024-06-10)";
    v35_03.changes.push_back("- Added SoundEngine auto-restart settings");
    v35_03.changes.push_back("- Disabled FPoSu noclip by default");
    v35_03.changes.push_back("- Fixed auto mod staying on after Ctrl+clicking a map");
    v35_03.changes.push_back("- Fixed downloads sometimes failing on Windows");
    v35_03.changes.push_back("- Fixed recent score times not being visible in leaderboards");
    v35_03.changes.push_back("- Fixed restarting map while watching a replay");
    v35_03.changes.push_back("- Improved sound engine reliability");
    v35_03.changes.push_back("- Re-added win_snd_wasapi_exclusive convar");
    v35_03.changes.push_back("- User mods will no longer change when watching a replay or joining a multiplayer room");
    changelogs.push_back(v35_03);

    CHANGELOG v35_02;
    v35_02.title = "35.02 (2024-06-08)";
    v35_02.changes.push_back("- Fixed online leaderboards displaying incorrect values");
    changelogs.push_back(v35_02);

    CHANGELOG v35_01;
    v35_01.title = "35.01 (2024-06-08)";
    v35_01.changes.push_back("- Added ability to get spectated");
    v35_01.changes.push_back("- Added use_https convar (to support plain HTTP servers)");
    v35_01.changes.push_back(
        "- Added restart_sound_engine_before_playing convar (\"fixes\" sound engine lagging after a while)");
    v35_01.changes.push_back("- Fixed chat channels being unread after joining");
    v35_01.changes.push_back("- Fixed flashlight mod");
    v35_01.changes.push_back("- Fixed FPoSu mode");
    v35_01.changes.push_back("- Fixed playfield borders not being visible");
    v35_01.changes.push_back("- Fixed sound engine not being restartable during gameplay or while paused");
    v35_01.changes.push_back("- Fixed missing window icon");
    v35_01.changes.push_back("- Hid password cvar from console command list");
    v35_01.changes.push_back("- Now making 64-bit MSVC builds");
    v35_01.changes.push_back("- Now using rosu-pp for some pp calculations");
    v35_01.changes.push_back("- Removed DirectX, Software, Vulkan renderers");
    v35_01.changes.push_back("- Removed OpenCL support");
    v35_01.changes.push_back("- Removed user stats screen");
    changelogs.push_back(v35_01);

    CHANGELOG v35_00;
    v35_00.title = "35.00 (2024-05-05)";
    v35_00.changes.push_back("- Renamed 'McOsu Multiplayer' to 'neosu'");
    v35_00.changes.push_back("- Added option to normalize loudness across songs");
    v35_00.changes.push_back("- Added server logo to main menu button");
    v35_00.changes.push_back("- Added instant_replay_duration convar");
    v35_00.changes.push_back("- Added ability to remove beatmaps from osu!stable collections (only affects neosu)");
    v35_00.changes.push_back("- Allowed singleplayer cheats when the server doesn't accept score submissions");
    v35_00.changes.push_back("- Changed scoreboard name color to red for friends");
    v35_00.changes.push_back("- Changed default instant replay key to F2 to avoid conflicts with mod selector");
    v35_00.changes.push_back("- Disabled score submission when mods are toggled mid-game");
    v35_00.changes.push_back("- Fixed ALT key not working on linux");
    v35_00.changes.push_back("- Fixed chat layout updating while chat was hidden");
    v35_00.changes.push_back("- Fixed experimental mods not getting set while watching replays");
    v35_00.changes.push_back("- Fixed FPoSu camera not following cursor while watching replays");
    v35_00.changes.push_back("- Fixed FPoSu mod not being included in score data");
    v35_00.changes.push_back("- Fixed level bar always being at 0%");
    v35_00.changes.push_back("- Fixed music pausing on first song database load");
    v35_00.changes.push_back("- Fixed not being able to adjust volume while song database was loading");
    v35_00.changes.push_back("- Fixed pause button not working after cancelling database load");
    v35_00.changes.push_back("- Fixed replay playback starting too fast");
    v35_00.changes.push_back("- Fixed restarting SoundEngine not kicking the player out of play mode");
    v35_00.changes.push_back("- Improved audio engine");
    v35_00.changes.push_back("- Improved overall stability");
    v35_00.changes.push_back("- Optimized song database loading speed (a lot)");
    v35_00.changes.push_back("- Optimized collection processing speed");
    v35_00.changes.push_back("- Removed support for the Nintendo Switch");
    v35_00.changes.push_back("- Updated protocol version");
    changelogs.push_back(v35_00);

    CHANGELOG v34_10;
    v34_10.title = "34.10 (2024-04-14)";
    v34_10.changes.push_back("- Fixed replays not saving/submitting correctly");
    v34_10.changes.push_back("- Fixed scores, collections and stars/pp cache not saving correctly");
    changelogs.push_back(v34_10);

    CHANGELOG v34_09;
    v34_09.title = "34.09 (2024-04-13)";
    v34_09.changes.push_back("- Added replay viewer");
    v34_09.changes.push_back("- Added instant replay (press F1 while paused or after failing)");
    v34_09.changes.push_back("- Added option to disable in-game scoreboard animations");
    v34_09.changes.push_back("- Added start_first_main_menu_song_at_preview_point convar (it does what it says)");
    v34_09.changes.push_back("- Added extra slot to in-game scoreboard");
    v34_09.changes.push_back("- Fixed hitobjects being hittable after failing");
    v34_09.changes.push_back("- Fixed login packet sending incorrect adapters list");
    v34_09.changes.push_back("- Removed VR support");
    v34_09.changes.push_back("- Updated protocol and database version to b20240411.1");
    changelogs.push_back(v34_09);

    CHANGELOG v34_08;
    v34_08.title = "34.08 (2024-03-30)";
    v34_08.changes.push_back("- Added animations for the in-game scoreboard");
    v34_08.changes.push_back("- Added option to always pick Nightcore mod first");
    v34_08.changes.push_back("- Added osu_animation_speed_override cheat convar (code by Givikap120)");
    v34_08.changes.push_back("- Added flashlight_always_hard convar to lock flashlight radius to the 200+ combo size");
    v34_08.changes.push_back("- Allowed scores to submit when using mirror mods");
    v34_08.changes.push_back("- Now playing a random song on game launch");
    v34_08.changes.push_back("- Small UI improvements");
    v34_08.changes.push_back("- Updated protocol and database version to b20240330.2");
    changelogs.push_back(v34_08);

    CHANGELOG v34_07;
    v34_07.title = "34.07 (2024-03-28)";
    v34_07.changes.push_back("- Added Flashlight mod");
    v34_07.changes.push_back("- Fixed a few bugs");
    changelogs.push_back(v34_07);

    CHANGELOG v34_06;
    v34_06.title = "34.06 (2024-03-12)";
    v34_06.changes.push_back("- Fixed pausing not working correctly");
    changelogs.push_back(v34_06);

    CHANGELOG v34_05;
    v34_05.title = "34.05 (2024-03-12)";
    v34_05.changes.push_back("- Added support for ASIO output");
    v34_05.changes.push_back("- Disabled ability to fail when using Relax (online)");
    v34_05.changes.push_back("- Enabled non-vanilla mods (disables score submission)");
    v34_05.changes.push_back("- Fixed speed modifications not getting applied to song previews when switching songs");
    v34_05.changes.push_back("- Improved Nightcore/Daycore audio quality");
    v34_05.changes.push_back("- Improved behavior of speed modifier mod selection");
    v34_05.changes.push_back("- Improved WASAPI output latency");
    changelogs.push_back(v34_05);

    CHANGELOG v34_04;
    v34_04.title = "34.04 (2024-03-03)";
    v34_04.changes.push_back("- Fixed replays having incorrect tickrate when using speed modifying mods (again)");
    v34_04.changes.push_back("- Fixed auto-updater not working");
    v34_04.changes.push_back("- Fixed scores getting submitted for 0-score plays");
    v34_04.changes.push_back("- Replay frames will now be written for slider ticks/ends");
    v34_04.changes.push_back("- When using Relax, replay frames will now also be written for every hitcircle");
    v34_04.changes.push_back("- Improved beatmap database loading performance");
    v34_04.changes.push_back("- Improved build process");
    changelogs.push_back(v34_04);

    CHANGELOG v34_03;
    v34_03.title = "34.03 (2024-02-29)";
    v34_03.changes.push_back("- Fixed replays having incorrect tickrate when using speed modifying mods");
    changelogs.push_back(v34_03);

    CHANGELOG v34_02;
    v34_02.title = "34.02 (2024-02-27)";
    v34_02.changes.push_back("- Added score submission (for servers that allow it via the x-mcosu-features header)");
    v34_02.changes.push_back("- Added [quit] indicator next to users who quit a match");
    v34_02.changes.push_back("- Made main menu shuffle through songs instead of looping over the same one");
    v34_02.changes.push_back("- Fixed \"No records set!\" banner display");
    v34_02.changes.push_back("- Fixed \"Server has restarded\" loop after a login error");
    v34_02.changes.push_back("- Fixed chat being force-hid during breaks and before map start");
    v34_02.changes.push_back("- Fixed chat not supporting expected keyboard navigation");
    v34_02.changes.push_back("- Fixed text selection for password field");
    v34_02.changes.push_back("- Fixed version header not being sent on login");
    changelogs.push_back(v34_02);

    CHANGELOG v34_01;
    v34_01.title = "34.01 (2024-02-18)";
    v34_01.changes.push_back("- Added ability to close chat channels with /close");
    v34_01.changes.push_back("- Added \"Force Start\" button to avoid host accidentally starting the match");
    v34_01.changes.push_back("- Disabled force start when there are only two players in the room");
    v34_01.changes.push_back("- Made tab button switch between endpoint, username and password fields in options menu");
    v34_01.changes.push_back("- Fixed not being visible on peppy client when starting a match as host");
    v34_01.changes.push_back("- Fixed Daycore, Nightcore and Perfect mod selection");
    v34_01.changes.push_back("- Fixed mod selection sound playing twice");
    v34_01.changes.push_back("- Fixed client not realizing when it gets kicked from a room");
    v34_01.changes.push_back("- Fixed room settings not updating immediately");
    v34_01.changes.push_back("- Fixed login failing with \"No account by the username '?' exists.\"");
    v34_01.changes.push_back("- Removed Steam Workshop button");
    changelogs.push_back(v34_01);

    CHANGELOG v34_00;
    v34_00.title = "34.00 (2024-02-16)";
    v34_00.changes.push_back("- Added ability to connect to servers using Bancho protocol");
    v34_00.changes.push_back("- Added ability to see, join and play in multiplayer rooms");
    v34_00.changes.push_back("- Added online leaderboards");
    v34_00.changes.push_back("- Added chat");
    v34_00.changes.push_back("- Added automatic map downloads for multiplayer rooms");
    v34_00.changes.push_back("");
    v34_00.changes.push_back("");
    v34_00.changes.push_back("");
    v34_00.changes.push_back("");
    v34_00.changes.push_back("");
    changelogs.push_back(v34_00);

    for(int i = 0; i < changelogs.size(); i++) {
        CHANGELOG_UI changelog;

        // title label
        changelog.title = new CBaseUILabel(0, 0, 0, 0, "", changelogs[i].title);
        if(i == 0)
            changelog.title->setTextColor(0xff00ff00);
        else
            changelog.title->setTextColor(0xff888888);

        changelog.title->setSizeToContent();
        changelog.title->setDrawBackground(false);
        changelog.title->setDrawFrame(false);

        m_scrollView->getContainer()->addBaseUIElement(changelog.title);

        // changes
        for(int c = 0; c < changelogs[i].changes.size(); c++) {
            class CustomCBaseUILabel : public CBaseUIButton {
               public:
                CustomCBaseUILabel(UString text) : CBaseUIButton(0, 0, 0, 0, "", text) { ; }

                virtual void draw(Graphics *g) {
                    if(m_bVisible && isMouseInside()) {
                        g->setColor(0x3fffffff);

                        const int margin = 0;
                        const int marginX = margin + 10;
                        g->fillRect(m_vPos.x - marginX, m_vPos.y - margin, m_vSize.x + marginX * 2,
                                    m_vSize.y + margin * 2);
                    }

                    if(!m_bVisible) return;

                    g->setColor(m_textColor);
                    g->pushTransform();
                    {
                        g->translate((int)(m_vPos.x + m_vSize.x / 2.0f - m_fStringWidth / 2.0f),
                                     (int)(m_vPos.y + m_vSize.y / 2.0f + m_fStringHeight / 2.0f));
                        g->drawString(m_font, m_sText);
                    }
                    g->popTransform();
                }
            };

            CBaseUIButton *change = new CustomCBaseUILabel(changelogs[i].changes[c]);
            change->setClickCallback(fastdelegate::MakeDelegate(this, &Changelog::onChangeClicked));

            if(i > 0) change->setTextColor(0xff888888);

            change->setSizeToContent();
            change->setDrawBackground(false);
            change->setDrawFrame(false);

            changelog.changes.push_back(change);

            m_scrollView->getContainer()->addBaseUIElement(change);
        }

        m_changelogs.push_back(changelog);
    }
}

Changelog::~Changelog() { m_changelogs.clear(); }

void Changelog::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible) return;
    ScreenBackable::mouse_update(propagate_clicks);
}

CBaseUIContainer *Changelog::setVisible(bool visible) {
    ScreenBackable::setVisible(visible);

    if(m_bVisible) updateLayout();

    return this;
}

void Changelog::updateLayout() {
    ScreenBackable::updateLayout();

    const float dpiScale = Osu::getUIScale();

    setSize(osu->getScreenSize() + Vector2(2, 2));
    m_scrollView->setSize(osu->getScreenSize() + Vector2(2, 2));

    float yCounter = 0;
    for(const CHANGELOG_UI &changelog : m_changelogs) {
        changelog.title->onResized();  // HACKHACK: framework, setSizeToContent() does not update string metrics
        changelog.title->setSizeToContent();

        yCounter += changelog.title->getSize().y;
        changelog.title->setRelPos(15 * dpiScale, yCounter);
        /// yCounter += 10 * dpiScale;

        for(CBaseUIButton *change : changelog.changes) {
            change->onResized();  // HACKHACK: framework, setSizeToContent() does not update string metrics
            change->setSizeToContent();
            change->setSizeY(change->getSize().y * 2.0f);
            yCounter += change->getSize().y /* + 13 * dpiScale*/;
            change->setRelPos(35 * dpiScale, yCounter);
        }

        // gap to previous version
        yCounter += 65 * dpiScale;
    }

    m_scrollView->setScrollSizeToContent(15 * dpiScale);
}

void Changelog::onBack() { osu->toggleChangelog(); }

void Changelog::onChangeClicked(CBaseUIButton *button) {
    const UString changeTextMaybeContainingClickableURL = button->getText();

    const int maybeURLBeginIndex = changeTextMaybeContainingClickableURL.find("http");
    if(maybeURLBeginIndex != -1 && changeTextMaybeContainingClickableURL.find("://") != -1) {
        UString url = changeTextMaybeContainingClickableURL.substr(maybeURLBeginIndex);
        if(url.length() > 0 && url[url.length() - 1] == L')') url = url.substr(0, url.length() - 1);

        debugLog("url = %s\n", url.toUtf8());

        osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
        env->openURLInDefaultBrowser(url);
    }
}
