//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		changelog screen
//
// $NoKeywords: $osulog
//===============================================================================//

#include "OsuChangelog.h"

#include "CBaseUIButton.h"
#include "CBaseUIContainer.h"
#include "CBaseUILabel.h"
#include "CBaseUIScrollView.h"
#include "ConVar.h"
#include "Engine.h"
#include "Mouse.h"
#include "Osu.h"
#include "OsuHUD.h"
#include "OsuOptionsMenu.h"
#include "OsuSkin.h"
#include "ResourceManager.h"
#include "SoundEngine.h"

OsuChangelog::OsuChangelog(Osu *osu) : OsuScreenBackable(osu) {
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
    latest.title =
        UString::format("%.2f (%s, %s)", convar->getConVarByName("osu_version")->getFloat(), __DATE__, __TIME__);
    latest.changes.push_back("- Added option to always pick Nightcore mod first");
    latest.changes.push_back("- Added osu_animation_speed_override cheat convar (code by Givikap120)");
    latest.changes.push_back("- Added flashlight_always_hard convar to lock flashlight radius to the 200+ combo size");
    latest.changes.push_back("- Allowed scores to submit when using mirror mods");
    latest.changes.push_back("- Now playing a random song on game launch");
    latest.changes.push_back("- Small UI improvements");
    changelogs.push_back(latest);

    CHANGELOG v34_07;
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
            change->setClickCallback(fastdelegate::MakeDelegate(this, &OsuChangelog::onChangeClicked));

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

OsuChangelog::~OsuChangelog() { m_changelogs.clear(); }

void OsuChangelog::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible) return;
    OsuScreenBackable::mouse_update(propagate_clicks);
}

CBaseUIContainer *OsuChangelog::setVisible(bool visible) {
    OsuScreenBackable::setVisible(visible);

    if(m_bVisible) updateLayout();

    return this;
}

void OsuChangelog::updateLayout() {
    OsuScreenBackable::updateLayout();

    const float dpiScale = Osu::getUIScale(m_osu);

    setSize(m_osu->getScreenSize() + Vector2(2, 2));
    m_scrollView->setSize(m_osu->getScreenSize() + Vector2(2, 2));

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

void OsuChangelog::onBack() {
    engine->getSound()->play(m_osu->getSkin()->getMenuClick());

    m_osu->toggleChangelog();
}

void OsuChangelog::onChangeClicked(CBaseUIButton *button) {
    const UString changeTextMaybeContainingClickableURL = button->getText();

    const int maybeURLBeginIndex = changeTextMaybeContainingClickableURL.find("http");
    if(maybeURLBeginIndex != -1 && changeTextMaybeContainingClickableURL.find("://") != -1) {
        UString url = changeTextMaybeContainingClickableURL.substr(maybeURLBeginIndex);
        if(url.length() > 0 && url[url.length() - 1] == L')') url = url.substr(0, url.length() - 1);

        debugLog("url = %s\n", url.toUtf8());

        m_osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
        env->openURLInDefaultBrowser(url);
    }
}
