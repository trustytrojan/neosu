#include "RankingScreen.h"

#include "AnimationHandler.h"
#include "Bancho.h"
#include "Beatmap.h"
#include "CBaseUIContainer.h"
#include "CBaseUIImage.h"
#include "CBaseUILabel.h"
#include "CBaseUIScrollView.h"
#include "Chat.h"
#include "ConVar.h"
#include "Engine.h"
#include "GameRules.h"
#include "Icons.h"
#include "Keyboard.h"
#include "ModSelector.h"
#include "Mouse.h"
#include "OptionsMenu.h"
#include "Osu.h"
#include "Replay.h"
#include "ResourceManager.h"
#include "RoomScreen.h"
#include "Skin.h"
#include "SkinImage.h"
#include "SongBrowser/ScoreButton.h"
#include "SongBrowser/SongBrowser.h"
#include "SoundEngine.h"
#include "TooltipOverlay.h"
#include "UIButton.h"
#include "UIRankingScreenInfoLabel.h"
#include "UIRankingScreenRankingPanel.h"
#include "pp.h"
#include "score.h"

ConVar osu_rankingscreen_topbar_height_percent("osu_rankingscreen_topbar_height_percent", 0.785f, FCVAR_DEFAULT);
ConVar osu_rankingscreen_pp("osu_rankingscreen_pp", true, FCVAR_DEFAULT);
ConVar osu_draw_rankingscreen_background_image("osu_draw_rankingscreen_background_image", true, FCVAR_DEFAULT);

class RankingScreenIndexLabel : public CBaseUILabel {
   public:
    RankingScreenIndexLabel() : CBaseUILabel(-1, 0, 0, 0, "", "You achieved the #1 score on local rankings!") {
        m_bVisible2 = false;
    }

    virtual void draw(Graphics *g) {
        if(!m_bVisible || !m_bVisible2) return;

        // draw background gradient
        const Color topColor = 0xdd634e13;
        const Color bottomColor = 0xdd785f15;
        g->fillGradient(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y, topColor, topColor, bottomColor, bottomColor);

        // draw ranking index text
        const float textScale = 0.45f;
        g->pushTransform();
        {
            const float scale = (m_vSize.y / m_fStringHeight) * textScale;

            g->scale(scale, scale);
            g->translate((int)(m_vPos.x + m_vSize.x / 2 - m_fStringWidth * scale / 2),
                         (int)(m_vPos.y + m_vSize.y / 2 + m_font->getHeight() * scale / 2));
            g->translate(1, 1);
            g->setColor(0xff000000);
            g->drawString(m_font, m_sText);
            g->translate(-1, -1);
            g->setColor(m_textColor);
            g->drawString(m_font, m_sText);
        }
        g->popTransform();
    }

    void setVisible2(bool visible2) { m_bVisible2 = visible2; }

    inline bool isVisible2() const { return m_bVisible2; }

   private:
    bool m_bVisible2;
};

class RankingScreenBottomElement : public CBaseUILabel {
   public:
    RankingScreenBottomElement() : CBaseUILabel(-1, 0, 0, 0, "", "") { m_bVisible2 = false; }

    virtual void draw(Graphics *g) {
        if(!m_bVisible || !m_bVisible2) return;

        // draw background gradient
        const Color topColor = 0xdd3a2e0c;
        const Color bottomColor = 0xdd493a0f;
        g->fillGradient(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y, topColor, topColor, bottomColor, bottomColor);
    }

    void setVisible2(bool visible2) { m_bVisible2 = visible2; }

    inline bool isVisible2() const { return m_bVisible2; }

   private:
    bool m_bVisible2;
};

class RankingScreenScrollDownInfoButton : public CBaseUIButton {
   public:
    RankingScreenScrollDownInfoButton() : CBaseUIButton(0, 0, 0, 0, "") {
        m_bVisible2 = false;
        m_fAlpha = 1.0f;
    }

    virtual void draw(Graphics *g) {
        if(!m_bVisible || !m_bVisible2) return;

        const float textScale = 0.45f;
        g->pushTransform();
        {
            const float scale = (m_vSize.y / m_fStringHeight) * textScale;

            float animation = fmod((float)(engine->getTime() - 0.0f) * 3.2f, 2.0f);
            if(animation > 1.0f) animation = 2.0f - animation;

            animation = -animation * (animation - 2);  // quad out
            const float offset = -m_fStringHeight * scale * 0.25f + animation * m_fStringHeight * scale * 0.25f;

            g->scale(scale, scale);
            g->translate((int)(m_vPos.x + m_vSize.x / 2 - m_fStringWidth * scale / 2),
                         (int)(m_vPos.y + m_vSize.y / 2 + m_fStringHeight * scale / 2 - offset));
            g->translate(2, 2);
            g->setColor(0xff000000);
            g->setAlpha(m_fAlpha);
            g->drawString(m_font, m_sText);
            g->translate(-2, -2);
            g->setColor(0xffffffff);
            g->setAlpha(m_fAlpha);
            g->drawString(m_font, m_sText);
        }
        g->popTransform();

        /*
        g->setColor(0xffffffff);
        g->drawRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);
        */
    }

    void setAlpha(float alpha) { m_fAlpha = alpha; }

    void setVisible2(bool visible2) { m_bVisible2 = visible2; }

    inline bool isVisible2() const { return m_bVisible2; }

    bool m_bVisible2;
    float m_fAlpha;
};

RankingScreen::RankingScreen() : ScreenBackable() {
    m_osu_scores_enabled = convar->getConVarByName("osu_scores_enabled");

    m_rankings = new CBaseUIScrollView(-1, 0, 0, 0, "");
    m_rankings->setHorizontalScrolling(false);
    m_rankings->setVerticalScrolling(false);
    m_rankings->setDrawFrame(false);
    m_rankings->setDrawBackground(false);
    m_rankings->setDrawScrollbars(false);
    addBaseUIElement(m_rankings);

    m_songInfo = new UIRankingScreenInfoLabel(5, 5, 0, 0, "");
    addBaseUIElement(m_songInfo);

    m_rankingTitle = new CBaseUIImage(osu->getSkin()->getRankingTitle()->getName(), 0, 0, 0, 0, "");
    m_rankingTitle->setDrawBackground(false);
    m_rankingTitle->setDrawFrame(false);
    addBaseUIElement(m_rankingTitle);

    m_rankingPanel = new UIRankingScreenRankingPanel();
    m_rankingPanel->setDrawBackground(false);
    m_rankingPanel->setDrawFrame(false);
    m_rankings->getContainer()->addBaseUIElement(m_rankingPanel);

    m_rankingGrade = new CBaseUIImage(osu->getSkin()->getRankingA()->getName(), 0, 0, 0, 0, "");
    m_rankingGrade->setDrawBackground(false);
    m_rankingGrade->setDrawFrame(false);
    m_rankings->getContainer()->addBaseUIElement(m_rankingGrade);

    m_rankingBottom = new RankingScreenBottomElement();
    m_rankings->getContainer()->addBaseUIElement(m_rankingBottom);

    m_rankingIndex = new RankingScreenIndexLabel();
    m_rankingIndex->setDrawFrame(false);
    m_rankingIndex->setCenterText(true);
    m_rankingIndex->setFont(osu->getSongBrowserFont());
    m_rankingIndex->setTextColor(0xffffcb21);
    m_rankings->getContainer()->addBaseUIElement(m_rankingIndex);

    m_retry_btn = new UIButton(0, 0, 0, 0, "", "Retry");
    m_retry_btn->setClickCallback(fastdelegate::MakeDelegate(this, &RankingScreen::onRetryClicked));
    m_rankings->getContainer()->addBaseUIElement(m_retry_btn);
    m_watch_btn = new UIButton(0, 0, 0, 0, "", "Watch replay");
    m_watch_btn->setClickCallback(fastdelegate::MakeDelegate(this, &RankingScreen::onWatchClicked));
    m_rankings->getContainer()->addBaseUIElement(m_watch_btn);

    setGrade(FinishedScore::Grade::D);
    setIndex(0);  // TEMP

    m_fUnstableRate = 0.0f;
    m_fHitErrorAvgMin = 0.0f;
    m_fHitErrorAvgMax = 0.0f;

    m_bModSS = false;
    m_bModSD = false;
    m_bModEZ = false;
    m_bModHD = false;
    m_bModHR = false;
    m_bModNC = false;
    m_bModDT = false;
    m_bModNightmare = false;
    m_bModScorev2 = false;
    m_bModTarget = false;
    m_bModSpunout = false;
    m_bModRelax = false;
    m_bModNF = false;
    m_bModHT = false;
    m_bModAutopilot = false;
    m_bModAuto = false;
    m_bModTD = false;

    m_bIsUnranked = false;
}

void RankingScreen::draw(Graphics *g) {
    if(!m_bVisible) return;

    // draw background image
    if(osu_draw_rankingscreen_background_image.getBool()) {
        SongBrowser::drawSelectedBeatmapBackgroundImage(g);

        // draw top black bar
        g->setColor(0xff000000);
        g->fillRect(0, 0, osu->getScreenWidth(),
                    m_rankingTitle->getSize().y * osu_rankingscreen_topbar_height_percent.getFloat());
    }

    ScreenBackable::draw(g);

    // draw active mods
    const Vector2 modPosStart =
        Vector2(m_rankings->getSize().x - osu->getUIScale(20), m_rankings->getRelPosY() + osu->getUIScale(260));
    Vector2 modPos = modPosStart;
    Vector2 modPosMax;
    if(m_bModTD) drawModImage(g, osu->getSkin()->getSelectionModTD(), modPos, modPosMax);
    if(m_bModSS)
        drawModImage(g, osu->getSkin()->getSelectionModPerfect(), modPos, modPosMax);
    else if(m_bModSD)
        drawModImage(g, osu->getSkin()->getSelectionModSuddenDeath(), modPos, modPosMax);
    if(m_bModEZ) drawModImage(g, osu->getSkin()->getSelectionModEasy(), modPos, modPosMax);
    if(m_bModHD) drawModImage(g, osu->getSkin()->getSelectionModHidden(), modPos, modPosMax);
    if(m_bModHR) drawModImage(g, osu->getSkin()->getSelectionModHardRock(), modPos, modPosMax);
    if(m_bModNC)
        drawModImage(g, osu->getSkin()->getSelectionModNightCore(), modPos, modPosMax);
    else if(m_bModDT)
        drawModImage(g, osu->getSkin()->getSelectionModDoubleTime(), modPos, modPosMax);
    if(m_bModNightmare) drawModImage(g, osu->getSkin()->getSelectionModNightmare(), modPos, modPosMax);
    if(m_bModScorev2) drawModImage(g, osu->getSkin()->getSelectionModScorev2(), modPos, modPosMax);
    if(m_bModTarget) drawModImage(g, osu->getSkin()->getSelectionModTarget(), modPos, modPosMax);
    if(m_bModSpunout) drawModImage(g, osu->getSkin()->getSelectionModSpunOut(), modPos, modPosMax);
    if(m_bModRelax) drawModImage(g, osu->getSkin()->getSelectionModRelax(), modPos, modPosMax);
    if(m_bModNF) drawModImage(g, osu->getSkin()->getSelectionModNoFail(), modPos, modPosMax);
    if(m_bModHT) drawModImage(g, osu->getSkin()->getSelectionModHalfTime(), modPos, modPosMax);
    if(m_bModAutopilot) drawModImage(g, osu->getSkin()->getSelectionModAutopilot(), modPos, modPosMax);
    if(m_bModAuto) drawModImage(g, osu->getSkin()->getSelectionModAutoplay(), modPos, modPosMax);

    // draw experimental mods
    if(m_enabledExperimentalMods.size() > 0) {
        McFont *experimentalModFont = osu->getSubTitleFont();
        const UString prefix = "+ ";

        float maxStringWidth = 0.0f;
        for(int i = 0; i < m_enabledExperimentalMods.size(); i++) {
            UString experimentalModName = m_enabledExperimentalMods[i]->getName();
            experimentalModName.insert(0, prefix);
            const float width = experimentalModFont->getStringWidth(experimentalModName);
            if(width > maxStringWidth) maxStringWidth = width;
        }

        const int backgroundMargin = 6;
        const float heightMultiplier = 1.25f;
        const int experimentalModHeight = (experimentalModFont->getHeight() * heightMultiplier);
        const Vector2 experimentalModPos = Vector2(modPosStart.x - maxStringWidth - backgroundMargin,
                                                   max(modPosStart.y, modPosMax.y) + osu->getUIScale(10) +
                                                       experimentalModFont->getHeight() * heightMultiplier);
        const int backgroundWidth = maxStringWidth + 2 * backgroundMargin;
        const int backgroundHeight = experimentalModHeight * m_enabledExperimentalMods.size() + 2 * backgroundMargin;

        g->setColor(0x77000000);
        g->fillRect((int)experimentalModPos.x - backgroundMargin,
                    (int)experimentalModPos.y - experimentalModFont->getHeight() - backgroundMargin, backgroundWidth,
                    backgroundHeight);

        g->pushTransform();
        {
            g->translate((int)experimentalModPos.x, (int)experimentalModPos.y);
            for(int i = 0; i < m_enabledExperimentalMods.size(); i++) {
                UString experimentalModName = m_enabledExperimentalMods[i]->getName();
                experimentalModName.insert(0, prefix);

                g->translate(1.5f, 1.5f);
                g->setColor(0xff000000);
                g->drawString(experimentalModFont, experimentalModName);
                g->translate(-1.5f, -1.5f);
                g->setColor(0xffffffff);
                g->drawString(experimentalModFont, experimentalModName);
                g->translate(0, experimentalModHeight);
            }
        }
        g->popTransform();
    }

    // draw pp
    if(osu_rankingscreen_pp.getBool()) {
        const UString ppString = getPPString();
        const Vector2 ppPos = getPPPosRaw();

        g->pushTransform();
        {
            g->translate((int)ppPos.x + 2, (int)ppPos.y + 2);
            g->setColor(0xff000000);
            g->drawString(osu->getTitleFont(), ppString);
            g->translate(-2, -2);
            g->setColor(0xffffffff);
            g->drawString(osu->getTitleFont(), ppString);
        }
        g->popTransform();
    }
}

void RankingScreen::drawModImage(Graphics *g, SkinImage *image, Vector2 &pos, Vector2 &max) {
    g->setColor(0xffffffff);
    image->draw(g, Vector2(pos.x - image->getSize().x / 2.0f, pos.y));

    pos.x -= osu->getUIScale(20);

    if(pos.y + image->getSize().y / 2 > max.y) max.y = pos.y + image->getSize().y / 2;
}

void RankingScreen::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible) return;
    ScreenBackable::mouse_update(propagate_clicks);

    // tooltip (pp + accuracy + unstable rate)
    if(!osu->getOptionsMenu()->isMouseInside() && engine->getMouse()->getPos().x < osu->getScreenWidth() * 0.5f) {
        osu->getTooltipOverlay()->begin();
        {
            osu->getTooltipOverlay()->addLine(UString::format("%.2fpp", m_score.ppv2_score));
            osu->getTooltipOverlay()->addLine("Difficulty:");
            osu->getTooltipOverlay()->addLine(UString::format("Stars: %.2f (%.2f aim, %.2f speed)",
                                                              m_score.ppv2_total_stars, m_score.ppv2_aim_stars,
                                                              m_score.ppv2_speed_stars));
            osu->getTooltipOverlay()->addLine(UString::format("Speed: %.3gx", m_score.speedMultiplier));
            osu->getTooltipOverlay()->addLine(
                UString::format("CS:%.2f AR:%.2f OD:%.2f HP:%.2f", m_score.CS,
                                GameRules::getRawApproachRateForSpeedMultiplier(
                                    GameRules::getRawApproachTime(m_score.AR), m_score.speedMultiplier),
                                GameRules::getRawOverallDifficultyForSpeedMultiplier(
                                    GameRules::getRawHitWindow300(m_score.OD), m_score.speedMultiplier),
                                m_score.HP));

            if(m_sMods.length() > 0) osu->getTooltipOverlay()->addLine(m_sMods);

            osu->getTooltipOverlay()->addLine("Accuracy:");
            osu->getTooltipOverlay()->addLine(
                UString::format("Error: %.2fms - %.2fms avg", m_fHitErrorAvgMin, m_fHitErrorAvgMax));
            osu->getTooltipOverlay()->addLine(UString::format("Unstable Rate: %.2f", m_fUnstableRate));
        }
        osu->getTooltipOverlay()->end();
    }
}

CBaseUIContainer *RankingScreen::setVisible(bool visible) {
    ScreenBackable::setVisible(visible);

    if(m_bVisible) {
        m_backButton->resetAnimation();
        m_rankings->scrollToY(0, false);

        updateLayout();
    } else {
        // Stop applause sound
        if(osu->getSkin()->getApplause() != NULL && osu->getSkin()->getApplause()->isPlaying()) {
            engine->getSound()->stop(osu->getSkin()->getApplause());
        }

        if(bancho.is_in_a_multi_room()) {
            // We backed out of the ranking screen, display the room again
            osu->m_room->setVisible(true);
            osu->m_chat->updateVisibility();

            // Since we prevented on_map_change() from running while the ranking screen was visible, run it now.
            osu->m_room->on_map_change();
        } else {
            osu->m_songBrowser2->setVisible(true);
        }
    }

    return this;
}

void RankingScreen::onRetryClicked() {
    // TODO @kiwec: doesn't work, just backs out to song browser, idk why
    setVisible(false);
    osu->getSelectedBeatmap()->play();
}

void RankingScreen::onWatchClicked() {
    // TODO @kiwec: doesn't work, just backs out to song browser, idk why
    setVisible(false);
    osu->getSelectedBeatmap()->watch(m_score, 0.0);
}

void RankingScreen::setScore(FinishedScore score) {
    auto current_name = convar->getConVarByName("name")->getString();
    bool is_same_player = !score.playerName.compare(current_name.toUtf8());

    m_score = score;

    // TODO @kiwec: buttons don't work correctly
    // m_retry_btn->setVisible(is_same_player && !bancho.is_in_a_multi_room());
    // m_watch_btn->setVisible(score.has_replay && !bancho.is_in_a_multi_room());

    // TODO @kiwec: i can't even setVisible(false) man this is so cancer
    m_retry_btn->setVisible(false);
    m_watch_btn->setVisible(false);

    m_bIsUnranked = false;

    char dateString[64];
    memset(dateString, '\0', 64);
    std::tm *tm = std::localtime((std::time_t *)(&score.unixTimestamp));
    std::strftime(dateString, 63, "%d-%b-%y %H:%M:%S", tm);

    m_songInfo->setDate(dateString);
    m_songInfo->setPlayer(score.playerName);

    m_rankingPanel->setScore(score);
    setGrade(LiveScore::calculateGrade(score.num300s, score.num100s, score.num50s, score.numMisses,
                                       score.modsLegacy & ModFlags::Hidden, score.modsLegacy & ModFlags::Flashlight));
    setIndex(-1);

    m_fUnstableRate = score.unstableRate;
    m_fHitErrorAvgMin = score.hitErrorAvgMin;
    m_fHitErrorAvgMax = score.hitErrorAvgMax;

    const UString modsString = ScoreButton::getModsStringForDisplay(score.modsLegacy);
    if(modsString.length() > 0) {
        m_sMods = "Mods: ";
        m_sMods.append(modsString);
    } else
        m_sMods = "";

    m_bModSS = score.modsLegacy & ModFlags::Perfect;
    m_bModSD = score.modsLegacy & ModFlags::SuddenDeath;
    m_bModEZ = score.modsLegacy & ModFlags::Easy;
    m_bModHD = score.modsLegacy & ModFlags::Hidden;
    m_bModHR = score.modsLegacy & ModFlags::HardRock;
    m_bModNC = score.modsLegacy & ModFlags::Nightcore;
    m_bModDT = score.modsLegacy & ModFlags::DoubleTime;
    m_bModNightmare = score.modsLegacy & ModFlags::Nightmare;
    m_bModScorev2 = score.modsLegacy & ModFlags::ScoreV2;
    m_bModTarget = score.modsLegacy & ModFlags::Target;
    m_bModSpunout = score.modsLegacy & ModFlags::SpunOut;
    m_bModRelax = score.modsLegacy & ModFlags::Relax;
    m_bModNF = score.modsLegacy & ModFlags::NoFail;
    m_bModHT = score.modsLegacy & ModFlags::HalfTime;
    m_bModAutopilot = score.modsLegacy & ModFlags::Autopilot;
    m_bModAuto = score.modsLegacy & ModFlags::Autoplay;
    m_bModTD = score.modsLegacy & ModFlags::TouchDevice;

    m_enabledExperimentalMods.clear();
    if(score.experimentalModsConVars.length() > 0) {
        auto cv = UString(score.experimentalModsConVars.c_str());
        std::vector<UString> experimentalMods = cv.split(";");
        for(int i = 0; i < experimentalMods.size(); i++) {
            if(experimentalMods[i].length() > 0) {
                ConVar *cvar = convar->getConVarByName(experimentalMods[i], false);
                if(cvar != NULL) m_enabledExperimentalMods.push_back(cvar);
            }
        }
    }
}

void RankingScreen::setBeatmapInfo(Beatmap *beatmap, DatabaseBeatmap *diff2) {
    m_score.diff2 = diff2;
    m_songInfo->setFromBeatmap(beatmap, diff2);

    UString local_name = convar->getConVarByName("name")->getString();
    m_songInfo->setPlayer(m_bIsUnranked ? "neosu" : local_name.toUtf8());

    if(m_score.is_peppy_imported() && m_score.ppv2_score == 0.f) {
        m_score.CS = diff2->getCS();
        m_score.AR = diff2->getAR();
        m_score.OD = diff2->getOD();
        m_score.HP = diff2->getHP();

        // @PPV3: fix this (broke when removing rosu-pp)
        m_score.ppv2_score = 0.f;

        // @PPV3: update m_fStarsTomAim, m_fStarsTomSpeed, m_fHitErrorAvgMin, m_fHitErrorAvgMax, m_fUnstableRate
    }
}

void RankingScreen::updateLayout() {
    ScreenBackable::updateLayout();

    const float uiScale = Osu::ui_scale->getFloat();

    setSize(osu->getScreenSize());

    m_rankingTitle->setImage(osu->getSkin()->getRankingTitle());
    m_rankingTitle->setScale(Osu::getImageScale(m_rankingTitle->getImage(), 75.0f) * uiScale,
                             Osu::getImageScale(m_rankingTitle->getImage(), 75.0f) * uiScale);
    m_rankingTitle->setSize(m_rankingTitle->getImage()->getWidth() * m_rankingTitle->getScale().x,
                            m_rankingTitle->getImage()->getHeight() * m_rankingTitle->getScale().y);
    m_rankingTitle->setRelPos(getSize().x - m_rankingTitle->getSize().x - osu->getUIScale(20.0f), 0);

    m_songInfo->setSize(osu->getScreenWidth(),
                        max(m_songInfo->getMinimumHeight(),
                            m_rankingTitle->getSize().y * osu_rankingscreen_topbar_height_percent.getFloat()));

    float btn_width = 150 * uiScale;
    float btn_height = 50 * uiScale;
    m_retry_btn->setSize(btn_width, btn_height);
    m_watch_btn->setSize(btn_width, btn_height);
    m_watch_btn->setRelPos(osu->getScreenSize().x - (btn_width + 10.f * uiScale),
                           osu->getScreenSize().y - (btn_height + 130.f * uiScale));
    m_retry_btn->setRelPos(osu->getScreenSize().x - (btn_width + 10.f * uiScale),
                           m_watch_btn->getRelPos().y - (btn_height + 5.f * uiScale));

    m_rankings->setSize(osu->getScreenSize().x + 2, osu->getScreenSize().y - m_songInfo->getSize().y + 3);
    m_rankings->setRelPosY(m_songInfo->getSize().y - 1);
    update_pos();

    // NOTE: no uiScale for rankingPanel and rankingGrade, doesn't really work due to legacy layout expectations
    const Vector2 hardcodedOsuRankingPanelImageSize =
        Vector2(622, 505) * (osu->getSkin()->isRankingPanel2x() ? 2.0f : 1.0f);
    m_rankingPanel->setImage(osu->getSkin()->getRankingPanel());
    m_rankingPanel->setScale(Osu::getImageScale(hardcodedOsuRankingPanelImageSize, 317.0f),
                             Osu::getImageScale(hardcodedOsuRankingPanelImageSize, 317.0f));
    m_rankingPanel->setSize(max(hardcodedOsuRankingPanelImageSize.x * m_rankingPanel->getScale().x,
                                m_rankingPanel->getImage()->getWidth() * m_rankingPanel->getScale().x),
                            max(hardcodedOsuRankingPanelImageSize.y * m_rankingPanel->getScale().y,
                                m_rankingPanel->getImage()->getHeight() * m_rankingPanel->getScale().y));

    m_rankingIndex->setSize(m_rankings->getSize().x + 2, osu->getScreenHeight() * 0.07f * uiScale);
    m_rankingIndex->setBackgroundColor(0xff745e13);
    m_rankingIndex->setRelPosY(m_rankings->getSize().y + 1);

    m_rankingBottom->setSize(m_rankings->getSize().x + 2, osu->getScreenHeight() * 0.2f);
    m_rankingBottom->setRelPosY(m_rankingIndex->getRelPos().y + m_rankingIndex->getSize().y);

    setGrade(m_grade);

    update_pos();
    m_rankings->getContainer()->update_pos();
    m_rankings->setScrollSizeToContent(0);
}

void RankingScreen::onBack() { setVisible(false); }

void RankingScreen::setGrade(FinishedScore::Grade grade) {
    m_grade = grade;

    Vector2 hardcodedOsuRankingGradeImageSize = Vector2(369, 422);
    switch(grade) {
        case FinishedScore::Grade::XH:
            hardcodedOsuRankingGradeImageSize *= (osu->getSkin()->isRankingXH2x() ? 2.0f : 1.0f);
            m_rankingGrade->setImage(osu->getSkin()->getRankingXH());
            break;
        case FinishedScore::Grade::SH:
            hardcodedOsuRankingGradeImageSize *= (osu->getSkin()->isRankingSH2x() ? 2.0f : 1.0f);
            m_rankingGrade->setImage(osu->getSkin()->getRankingSH());
            break;
        case FinishedScore::Grade::X:
            hardcodedOsuRankingGradeImageSize *= (osu->getSkin()->isRankingX2x() ? 2.0f : 1.0f);
            m_rankingGrade->setImage(osu->getSkin()->getRankingX());
            break;
        case FinishedScore::Grade::S:
            hardcodedOsuRankingGradeImageSize *= (osu->getSkin()->isRankingS2x() ? 2.0f : 1.0f);
            m_rankingGrade->setImage(osu->getSkin()->getRankingS());
            break;
        case FinishedScore::Grade::A:
            hardcodedOsuRankingGradeImageSize *= (osu->getSkin()->isRankingA2x() ? 2.0f : 1.0f);
            m_rankingGrade->setImage(osu->getSkin()->getRankingA());
            break;
        case FinishedScore::Grade::B:
            hardcodedOsuRankingGradeImageSize *= (osu->getSkin()->isRankingB2x() ? 2.0f : 1.0f);
            m_rankingGrade->setImage(osu->getSkin()->getRankingB());
            break;
        case FinishedScore::Grade::C:
            hardcodedOsuRankingGradeImageSize *= (osu->getSkin()->isRankingC2x() ? 2.0f : 1.0f);
            m_rankingGrade->setImage(osu->getSkin()->getRankingC());
            break;
        default:
            hardcodedOsuRankingGradeImageSize *= (osu->getSkin()->isRankingD2x() ? 2.0f : 1.0f);
            m_rankingGrade->setImage(osu->getSkin()->getRankingD());
            break;
    }

    const float uiScale = /*Osu::ui_scale->getFloat()*/ 1.0f;  // NOTE: no uiScale for rankingPanel and rankingGrade,
                                                               // doesn't really work due to legacy layout expectations

    const float rankingGradeImageScale = Osu::getImageScale(hardcodedOsuRankingGradeImageSize, 230.0f) * uiScale;
    m_rankingGrade->setScale(rankingGradeImageScale, rankingGradeImageScale);
    m_rankingGrade->setSize(m_rankingGrade->getImage()->getWidth() * m_rankingGrade->getScale().x,
                            m_rankingGrade->getImage()->getHeight() * m_rankingGrade->getScale().y);
    m_rankingGrade->setRelPos(m_rankings->getSize().x - osu->getUIScale(120) -
                                  m_rankingGrade->getImage()->getWidth() * m_rankingGrade->getScale().x / 2.0f,
                              -m_rankings->getRelPos().y +
                                  osu->getUIScale(osu->getSkin()->getVersion() > 1.0f ? 200 : 170) -
                                  m_rankingGrade->getImage()->getHeight() * m_rankingGrade->getScale().x / 2.0f);
}

void RankingScreen::setIndex(int index) {
    if(!m_osu_scores_enabled->getBool()) index = -1;

    if(index > -1) {
        m_rankingIndex->setText(UString::format("You achieved the #%i score on local rankings!", (index + 1)));
        m_rankingIndex->setVisible2(true);
        m_rankingBottom->setVisible2(true);
    } else {
        m_rankingIndex->setVisible2(false);
        m_rankingBottom->setVisible2(false);
    }
}

UString RankingScreen::getPPString() { return UString::format("%ipp", (int)(std::round(m_score.ppv2_score))); }

Vector2 RankingScreen::getPPPosRaw() {
    const UString ppString = getPPString();
    float ppStringWidth = osu->getTitleFont()->getStringWidth(ppString);
    return Vector2(m_rankingGrade->getPos().x, Osu::ui_scale->getFloat() * 10.f) +
           Vector2(m_rankingGrade->getSize().x / 2 - (ppStringWidth / 2 + Osu::ui_scale->getFloat() * 100.f),
                   m_rankings->getRelPosY() + osu->getUIScale(400) + osu->getTitleFont()->getHeight() / 2);
}
