//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		clickable button displaying score, grade, name, acc, mods, combo
//
// $NoKeywords: $
//===============================================================================//

#include "UISongBrowserScoreButton.h"

#include <chrono>

#include "AnimationHandler.h"
#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoUsers.h"
#include "ConVar.h"
#include "Console.h"
#include "Database.h"
#include "DatabaseBeatmap.h"
#include "Engine.h"
#include "GameRules.h"
#include "Icons.h"
#include "Keyboard.h"
#include "ModSelector.h"
#include "Mouse.h"
#include "NotificationOverlay.h"
#include "Osu.h"
#include "Replay.h"
#include "ResourceManager.h"
#include "Skin.h"
#include "SkinImage.h"
#include "SongBrowser.h"
#include "SoundEngine.h"
#include "TooltipOverlay.h"
#include "UIAvatar.h"
#include "UIContextMenu.h"
#include "UserStatsScreen.h"

ConVar *UISongBrowserScoreButton::m_osu_scores_sort_by_pp_ref = NULL;
UString UISongBrowserScoreButton::recentScoreIconString;

UISongBrowserScoreButton::UISongBrowserScoreButton(Osu *osu, UIContextMenu *contextMenu, float xPos, float yPos,
                                                   float xSize, float ySize, STYLE style)
    : CBaseUIButton(xPos, yPos, xSize, ySize, "", "") {
    m_osu = osu;
    m_contextMenu = contextMenu;
    m_style = style;

    if(m_osu_scores_sort_by_pp_ref == NULL)
        m_osu_scores_sort_by_pp_ref = convar->getConVarByName("osu_scores_sort_by_pp");

    if(recentScoreIconString.length() < 1) recentScoreIconString.insert(0, Icons::ARROW_CIRCLE_UP);

    m_fIndexNumberAnim = 0.0f;
    m_bIsPulseAnim = false;

    m_bRightClick = false;
    m_bRightClickCheck = false;

    m_iScoreIndexNumber = 1;
    m_iScoreUnixTimestamp = 0;

    m_scoreGrade = FinishedScore::Grade::D;
}

UISongBrowserScoreButton::~UISongBrowserScoreButton() { anim->deleteExistingAnimation(&m_fIndexNumberAnim); }

void UISongBrowserScoreButton::draw(Graphics *g) {
    if(!m_bVisible) return;

    // background
    if(m_style == STYLE::SCORE_BROWSER) {
        g->setColor(0xff000000);
        g->setAlpha(0.59f * (0.5f + 0.5f * m_fIndexNumberAnim));
        Image *backgroundImage = m_osu->getSkin()->getMenuButtonBackground();
        g->pushTransform();
        {
            // allow overscale
            Vector2 hardcodedImageSize =
                Vector2(699.0f, 103.0f) * (m_osu->getSkin()->isMenuButtonBackground2x() ? 2.0f : 1.0f);
            const float scale = Osu::getImageScaleToFillResolution(hardcodedImageSize, m_vSize);

            g->scale(scale, scale);
            g->translate(m_vPos.x + hardcodedImageSize.x * scale / 2.0f,
                         m_vPos.y + hardcodedImageSize.y * scale / 2.0f);
            g->drawImage(backgroundImage);
        }
        g->popTransform();
    } else if(m_style == STYLE::TOP_RANKS) {
        g->setColor(0xff666666);  // from 33413c to 4e7466
        g->setAlpha(0.59f * (0.5f + 0.5f * m_fIndexNumberAnim));
        g->fillRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);
    }

    const int yPos = (int)m_vPos.y;  // avoid max shimmering

    // index number
    if(m_avatar) {
        const float margin = m_vSize.y * 0.1;
        m_avatar->setPos(m_vPos.x + margin, m_vPos.y + margin);
        m_avatar->setSize(m_vSize.y - (2 * margin), m_vSize.y - (2 * margin));

        // Update avatar visibility status
        // NOTE: Not checking horizontal visibility
        auto m_scoreBrowser = m_osu->m_songBrowser2->m_scoreBrowser;
        bool is_below_top = m_avatar->getPos().y + m_avatar->getSize().y >= m_scoreBrowser->getPos().y;
        bool is_above_bottom = m_avatar->getPos().y <= m_scoreBrowser->getPos().y + m_scoreBrowser->getSize().y;
        m_avatar->on_screen = is_below_top && is_above_bottom;
        m_avatar->draw(g, 1.f);
    }
    const float indexNumberScale = 0.35f;
    const float indexNumberWidthPercent = (m_style == STYLE::TOP_RANKS ? 0.075f : 0.15f);
    McFont *indexNumberFont = m_osu->getSongBrowserFontBold();
    g->pushTransform();
    {
        UString indexNumberString = UString::format("%i", m_iScoreIndexNumber);
        const float scale = (m_vSize.y / indexNumberFont->getHeight()) * indexNumberScale;

        g->scale(scale, scale);
        g->translate((int)(m_vPos.x + m_vSize.x * indexNumberWidthPercent * 0.5f -
                           indexNumberFont->getStringWidth(indexNumberString) * scale / 2.0f),
                     (int)(yPos + m_vSize.y / 2.0f + indexNumberFont->getHeight() * scale / 2.0f));
        g->translate(0.5f, 0.5f);
        g->setColor(0xff000000);
        g->setAlpha(1.0f - (1.0f - m_fIndexNumberAnim));
        g->drawString(indexNumberFont, indexNumberString);
        g->translate(-0.5f, -0.5f);
        g->setColor(0xffffffff);
        g->setAlpha(1.0f - (1.0f - m_fIndexNumberAnim) * (1.0f - m_fIndexNumberAnim));
        g->drawString(indexNumberFont, indexNumberString);
    }
    g->popTransform();

    // grade
    const float gradeHeightPercent = 0.7f;
    SkinImage *grade = getGradeImage(m_osu, m_scoreGrade);
    int gradeWidth = 0;
    g->pushTransform();
    {
        const float scale = Osu::getImageScaleToFitResolution(
            grade->getSizeBaseRaw(),
            Vector2(m_vSize.x * (1.0f - indexNumberWidthPercent), m_vSize.y * gradeHeightPercent));
        gradeWidth = grade->getSizeBaseRaw().x * scale;

        g->setColor(0xffffffff);
        grade->drawRaw(g,
                       Vector2((int)(m_vPos.x + m_vSize.x * indexNumberWidthPercent + gradeWidth / 2.0f),
                               (int)(m_vPos.y + m_vSize.y / 2.0f)),
                       scale);
    }
    g->popTransform();

    const float gradePaddingRight = m_vSize.y * 0.165f;

    // username | (artist + songName + diffName)
    const float usernameScale = (m_style == STYLE::TOP_RANKS ? 0.6f : 0.7f);
    McFont *usernameFont = m_osu->getSongBrowserFont();
    g->pushClipRect(McRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y));
    g->pushTransform();
    {
        const float height = m_vSize.y * 0.5f;
        const float paddingTopPercent = (1.0f - usernameScale) * (m_style == STYLE::TOP_RANKS ? 0.15f : 0.4f);
        const float paddingTop = height * paddingTopPercent;
        const float scale = (height / usernameFont->getHeight()) * usernameScale;

        UString &string = (m_style == STYLE::TOP_RANKS ? m_sScoreTitle : m_sScoreUsername);

        g->scale(scale, scale);
        g->translate((int)(m_vPos.x + m_vSize.x * indexNumberWidthPercent + gradeWidth + gradePaddingRight),
                     (int)(yPos + height / 2.0f + usernameFont->getHeight() * scale / 2.0f + paddingTop));
        g->translate(0.75f, 0.75f);
        g->setColor(0xff000000);
        g->setAlpha(0.75f);
        g->drawString(usernameFont, string);
        g->translate(-0.75f, -0.75f);
        g->setColor(is_friend ? 0xffD424B0 : 0xffffffff);
        g->drawString(usernameFont, string);
    }
    g->popTransform();
    g->popClipRect();

    // score | pp | weighted 95% (pp)
    const float scoreScale = 0.5f;
    McFont *scoreFont = (m_vSize.y < 50 ? engine->getResourceManager()->getFont("FONT_DEFAULT")
                                        : usernameFont);  // HACKHACK: switch font for very low resolutions
    g->pushTransform();
    {
        const float height = m_vSize.y * 0.5f;
        const float paddingBottomPercent = (1.0f - scoreScale) * (m_style == STYLE::TOP_RANKS ? 0.1f : 0.25f);
        const float paddingBottom = height * paddingBottomPercent;
        const float scale = (height / scoreFont->getHeight()) * scoreScale;

        UString &string = (m_style == STYLE::TOP_RANKS ? m_sScoreScorePPWeightedPP : m_sScoreScorePP);

        g->scale(scale, scale);
        g->translate((int)(m_vPos.x + m_vSize.x * indexNumberWidthPercent + gradeWidth + gradePaddingRight),
                     (int)(yPos + height * 1.5f + scoreFont->getHeight() * scale / 2.0f - paddingBottom));
        g->translate(0.75f, 0.75f);
        g->setColor(0xff000000);
        g->setAlpha(0.75f);
        g->drawString(scoreFont, (m_osu_scores_sort_by_pp_ref->getBool() && !m_score.isLegacyScore
                                      ? string
                                      : (m_style == STYLE::TOP_RANKS ? string : m_sScoreScore)));
        g->translate(-0.75f, -0.75f);
        g->setColor((m_style == STYLE::TOP_RANKS ? 0xffdeff87 : 0xffffffff));
        g->drawString(scoreFont, (m_osu_scores_sort_by_pp_ref->getBool() && !m_score.isLegacyScore
                                      ? string
                                      : (m_style == STYLE::TOP_RANKS ? string : m_sScoreScore)));

        if(m_style == STYLE::TOP_RANKS) {
            g->translate(scoreFont->getStringWidth(string) * scale, 0);
            g->translate(0.75f, 0.75f);
            g->setColor(0xff000000);
            g->setAlpha(0.75f);
            g->drawString(scoreFont, m_sScoreScorePPWeightedWeight);
            g->translate(-0.75f, -0.75f);
            g->setColor(0xffbbbbbb);
            g->drawString(scoreFont, m_sScoreScorePPWeightedWeight);
        }
    }
    g->popTransform();

    const float rightSideThirdHeight = m_vSize.y * 0.333f;
    const float rightSidePaddingRight = (m_style == STYLE::TOP_RANKS ? 5 : m_vSize.x * 0.025f);

    // mods
    const float modScale = 0.7f;
    McFont *modFont = m_osu->getSubTitleFont();
    g->pushTransform();
    {
        const float height = rightSideThirdHeight;
        const float paddingTopPercent = (1.0f - modScale) * 0.45f;
        const float paddingTop = height * paddingTopPercent;
        const float scale = (height / modFont->getHeight()) * modScale;

        g->scale(scale, scale);
        g->translate(
            (int)(m_vPos.x + m_vSize.x - modFont->getStringWidth(m_sScoreMods) * scale - rightSidePaddingRight),
            (int)(yPos + height * 0.5f + modFont->getHeight() * scale / 2.0f + paddingTop));
        g->translate(0.75f, 0.75f);
        g->setColor(0xff000000);
        g->setAlpha(0.75f);
        g->drawString(modFont, m_sScoreMods);
        g->translate(-0.75f, -0.75f);
        g->setColor(0xffffffff);
        g->drawString(modFont, m_sScoreMods);
    }
    g->popTransform();

    // accuracy
    const float accScale = 0.65f;
    McFont *accFont = m_osu->getSubTitleFont();
    g->pushTransform();
    {
        const UString &scoreAccuracy = (m_style == STYLE::TOP_RANKS ? m_sScoreAccuracyFC : m_sScoreAccuracy);

        const float height = rightSideThirdHeight;
        const float paddingTopPercent = (1.0f - modScale) * 0.45f;
        const float paddingTop = height * paddingTopPercent;
        const float scale = (height / accFont->getHeight()) * accScale;

        g->scale(scale, scale);
        g->translate(
            (int)(m_vPos.x + m_vSize.x - accFont->getStringWidth(scoreAccuracy) * scale - rightSidePaddingRight),
            (int)(yPos + height * 1.5f + accFont->getHeight() * scale / 2.0f + paddingTop));
        g->translate(0.75f, 0.75f);
        g->setColor(0xff000000);
        g->setAlpha(0.75f);
        g->drawString(accFont, scoreAccuracy);
        g->translate(-0.75f, -0.75f);
        g->setColor((m_style == STYLE::TOP_RANKS ? 0xffffcc22 : 0xffffffff));
        g->drawString(accFont, scoreAccuracy);
    }
    g->popTransform();

    // custom info (Spd.)
    if(m_style == STYLE::SCORE_BROWSER && m_sCustom.length() > 0) {
        const float customScale = 0.50f;
        McFont *customFont = m_osu->getSubTitleFont();
        g->pushTransform();
        {
            const float height = rightSideThirdHeight;
            const float paddingTopPercent = (1.0f - modScale) * 0.45f;
            const float paddingTop = height * paddingTopPercent;
            const float scale = (height / customFont->getHeight()) * customScale;

            g->scale(scale, scale);
            g->translate(
                (int)(m_vPos.x + m_vSize.x - customFont->getStringWidth(m_sCustom) * scale - rightSidePaddingRight),
                (int)(yPos + height * 2.325f + customFont->getHeight() * scale / 2.0f + paddingTop));
            g->translate(0.75f, 0.75f);
            g->setColor(0xff000000);
            g->setAlpha(0.75f);
            g->drawString(customFont, m_sCustom);
            g->translate(-0.75f, -0.75f);
            g->setColor((m_style == STYLE::TOP_RANKS ? 0xffffcc22 : 0xffffffff));
            g->drawString(customFont, m_sCustom);
        }
        g->popTransform();
    }

    if(m_style == STYLE::TOP_RANKS) {
        // weighted percent
        const float weightScale = 0.65f;
        McFont *weightFont = m_osu->getSubTitleFont();
        g->pushTransform();
        {
            const float height = rightSideThirdHeight;
            const float paddingBottomPercent = (1.0f - weightScale) * 0.05f;
            const float paddingBottom = height * paddingBottomPercent;
            const float scale = (height / weightFont->getHeight()) * weightScale;

            g->scale(scale, scale);
            g->translate((int)(m_vPos.x + m_vSize.x - weightFont->getStringWidth(m_sScoreWeight) * scale -
                               rightSidePaddingRight),
                         (int)(yPos + height * 2.5f + weightFont->getHeight() * scale / 2.0f - paddingBottom));
            g->translate(0.75f, 0.75f);
            g->setColor(0xff000000);
            g->setAlpha(0.75f);
            g->drawString(weightFont, m_sScoreWeight);
            g->translate(-0.75f, -0.75f);
            g->setColor(0xff999999);
            g->drawString(weightFont, m_sScoreWeight);
        }
        g->popTransform();
    }

    // recent icon + elapsed time since score
    const float upIconScale = 0.35f;
    const float timeElapsedScale = accScale;
    McFont *iconFont = m_osu->getFontIcons();
    McFont *timeFont = accFont;
    if(m_iScoreUnixTimestamp > 0) {
        const float iconScale = (m_vSize.y / iconFont->getHeight()) * upIconScale;
        const float iconHeight = iconFont->getHeight() * iconScale;
        const float iconPaddingLeft = 2 + (m_style == STYLE::TOP_RANKS ? m_vSize.y * 0.125f : 0);

        g->pushTransform();
        {
            g->scale(iconScale, iconScale);
            g->translate((int)(m_vPos.x + m_vSize.x + iconPaddingLeft), (int)(yPos + m_vSize.y / 2 + iconHeight / 2));
            g->translate(1, 1);
            g->setColor(0xff000000);
            g->setAlpha(0.75f);
            g->drawString(iconFont, recentScoreIconString);
            g->translate(-1, -1);
            g->setColor(0xffffffff);
            g->drawString(iconFont, recentScoreIconString);
        }
        g->popTransform();

        // elapsed time since score
        if(m_sScoreTime.length() > 0) {
            const float timeHeight = rightSideThirdHeight;
            const float timeScale = (timeHeight / timeFont->getHeight()) * timeElapsedScale;
            const float timePaddingLeft = 8;

            g->pushTransform();
            {
                g->scale(timeScale, timeScale);
                g->translate((int)(m_vPos.x + m_vSize.x + iconPaddingLeft +
                                   iconFont->getStringWidth(recentScoreIconString) * iconScale + timePaddingLeft),
                             (int)(yPos + m_vSize.y / 2 + timeFont->getHeight() * timeScale / 2));
                g->translate(0.75f, 0.75f);
                g->setColor(0xff000000);
                g->setAlpha(0.85f);
                g->drawString(timeFont, m_sScoreTime);
                g->translate(-0.75f, -0.75f);
                g->setColor(0xffffffff);
                g->drawString(timeFont, m_sScoreTime);
            }
            g->popTransform();
        }
    }

    // TODO: difference to below score in list, +12345

    if(m_style == STYLE::TOP_RANKS) {
        g->setColor(0xff111111);
        g->drawRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);
    }
}

void UISongBrowserScoreButton::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible) return;

    if(m_avatar) {
        m_avatar->mouse_update(propagate_clicks);
        if(!*propagate_clicks) return;
    }

    CBaseUIButton::mouse_update(propagate_clicks);

    // HACKHACK: this should really be part of the UI base
    // right click detection
    if(engine->getMouse()->isRightDown()) {
        if(!m_bRightClickCheck) {
            m_bRightClickCheck = true;
            m_bRightClick = isMouseInside();
        }
    } else {
        if(m_bRightClick) {
            if(isMouseInside()) onRightMouseUpInside();
        }

        m_bRightClickCheck = false;
        m_bRightClick = false;
    }

    // tooltip (extra stats)
    if(isMouseInside()) {
        if(!isContextMenuVisible()) {
            if(m_fIndexNumberAnim > 0.0f) {
                m_osu->getTooltipOverlay()->begin();
                {
                    for(int i = 0; i < m_tooltipLines.size(); i++) {
                        if(m_tooltipLines[i].length() > 0) m_osu->getTooltipOverlay()->addLine(m_tooltipLines[i]);
                    }
                }
                m_osu->getTooltipOverlay()->end();
            }
        } else {
            anim->deleteExistingAnimation(&m_fIndexNumberAnim);
            m_fIndexNumberAnim = 0.0f;
        }
    }

    // update elapsed time string
    updateElapsedTimeString();

    // stuck anim reset
    if(!isMouseInside() && !anim->isAnimating(&m_fIndexNumberAnim)) m_fIndexNumberAnim = 0.0f;
}

void UISongBrowserScoreButton::highlight() {
    m_bIsPulseAnim = true;

    const int numPulses = 10;
    const float timescale = 1.75f;
    for(int i = 0; i < 2 * numPulses; i++) {
        if(i % 2 == 0)
            anim->moveQuadOut(&m_fIndexNumberAnim, 1.0f, 0.125f * timescale,
                              ((i / 2) * (0.125f + 0.15f)) * timescale - 0.001f, (i == 0));
        else
            anim->moveLinear(&m_fIndexNumberAnim, 0.0f, 0.15f * timescale,
                             (0.125f + (i / 2) * (0.125f + 0.15f)) * timescale - 0.001f);
    }
}

void UISongBrowserScoreButton::resetHighlight() {
    m_bIsPulseAnim = false;
    anim->deleteExistingAnimation(&m_fIndexNumberAnim);
    m_fIndexNumberAnim = 0.0f;
}

void UISongBrowserScoreButton::updateElapsedTimeString() {
    if(m_iScoreUnixTimestamp > 0) {
        const uint64_t curUnixTime =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
        const uint64_t delta = curUnixTime - m_iScoreUnixTimestamp;

        const uint64_t deltaInSeconds = delta;
        const uint64_t deltaInMinutes = delta / 60;
        const uint64_t deltaInHours = deltaInMinutes / 60;
        const uint64_t deltaInDays = deltaInHours / 24;
        const uint64_t deltaInYears = deltaInDays / 365;

        if(deltaInHours < 96 || m_style == STYLE::TOP_RANKS) {
            if(deltaInDays > 364)
                m_sScoreTime = UString::format("%iy", (int)(deltaInYears));
            else if(deltaInHours > 47)
                m_sScoreTime = UString::format("%id", (int)(deltaInDays));
            else if(deltaInHours >= 1)
                m_sScoreTime = UString::format("%ih", (int)(deltaInHours));
            else if(deltaInMinutes > 0)
                m_sScoreTime = UString::format("%im", (int)(deltaInMinutes));
            else
                m_sScoreTime = UString::format("%is", (int)(deltaInSeconds));
        } else {
            m_iScoreUnixTimestamp = 0;

            if(m_sScoreTime.length() > 0) m_sScoreTime.clear();
        }
    }
}

void UISongBrowserScoreButton::onClicked() {
    engine->getSound()->play(m_osu->getSkin()->getMenuHit());
    CBaseUIButton::onClicked();
}

void UISongBrowserScoreButton::onMouseInside() {
    m_bIsPulseAnim = false;

    if(!isContextMenuVisible())
        anim->moveQuadOut(&m_fIndexNumberAnim, 1.0f, 0.125f * (1.0f - m_fIndexNumberAnim), true);
}

void UISongBrowserScoreButton::onMouseOutside() {
    m_bIsPulseAnim = false;

    anim->moveLinear(&m_fIndexNumberAnim, 0.0f, 0.15f * m_fIndexNumberAnim, true);
}

void UISongBrowserScoreButton::onFocusStolen() {
    CBaseUIButton::onFocusStolen();

    m_bRightClick = false;

    if(!m_bIsPulseAnim) {
        anim->deleteExistingAnimation(&m_fIndexNumberAnim);
        m_fIndexNumberAnim = 0.0f;
    }
}

void UISongBrowserScoreButton::onRightMouseUpInside() {
    const Vector2 pos = engine->getMouse()->getPos();

    if(m_contextMenu != NULL) {
        m_contextMenu->setPos(pos);
        m_contextMenu->setRelPos(pos);
        m_contextMenu->begin(0, true);
        {
            m_contextMenu->addButton("Use Mods", 1);  // for scores without mods this will just nomod

            if(m_score.has_replay) {
                m_contextMenu->addButton("View replay", 2);
            }

            CBaseUIButton *spacer = m_contextMenu->addButton("---");
            spacer->setEnabled(false);
            spacer->setTextColor(0xff888888);
            spacer->setTextDarkColor(0xff000000);
            CBaseUIButton *deleteButton = m_contextMenu->addButton("Delete Score", 3);
            if(m_score.isLegacyScore) {
                deleteButton->setEnabled(false);
                deleteButton->setTextColor(0xff888888);
                deleteButton->setTextDarkColor(0xff000000);
            }
        }
        m_contextMenu->end(false, false);
        m_contextMenu->setClickCallback(fastdelegate::MakeDelegate(this, &UISongBrowserScoreButton::onContextMenu));
        UIContextMenu::clampToRightScreenEdge(m_contextMenu);
        UIContextMenu::clampToBottomScreenEdge(m_contextMenu);
    }
}

void UISongBrowserScoreButton::onContextMenu(UString text, int id) {
    if(id == 1) {
        onUseModsClicked();
        return;
    }

    if(id == 2) {
        Replay::load_and_watch(m_score);
        return;
    }

    if(id == 3) {
        if(engine->getKeyboard()->isShiftDown())
            onDeleteScoreConfirmed(text, 1);
        else
            onDeleteScoreClicked();

        return;
    }
}

void UISongBrowserScoreButton::onUseModsClicked() {
    bool nomod = m_osu->useMods(&m_score);
    engine->getSound()->play(nomod ? m_osu->getSkin()->getCheckOff() : m_osu->getSkin()->getCheckOn());
}

void UISongBrowserScoreButton::onDeleteScoreClicked() {
    if(m_contextMenu != NULL) {
        m_contextMenu->begin(0, true);
        {
            m_contextMenu->addButton("Really delete score?")->setEnabled(false);
            CBaseUIButton *spacer = m_contextMenu->addButton("---");
            spacer->setTextLeft(false);
            spacer->setEnabled(false);
            spacer->setTextColor(0xff888888);
            spacer->setTextDarkColor(0xff000000);
            m_contextMenu->addButton("Yes", 1)->setTextLeft(false);
            m_contextMenu->addButton("No")->setTextLeft(false);
        }
        m_contextMenu->end(false, false);
        m_contextMenu->setClickCallback(
            fastdelegate::MakeDelegate(this, &UISongBrowserScoreButton::onDeleteScoreConfirmed));
        UIContextMenu::clampToRightScreenEdge(m_contextMenu);
        UIContextMenu::clampToBottomScreenEdge(m_contextMenu);
    }
}

void UISongBrowserScoreButton::onDeleteScoreConfirmed(UString text, int id) {
    if(id != 1) return;

    debugLog("Deleting score\n");

    // absolutely disgusting
    if(m_style == STYLE::SCORE_BROWSER)
        m_osu->getSongBrowser()->onScoreContextMenu(this, 2);
    else if(m_style == STYLE::TOP_RANKS) {
        // in this case, deletion of the actual scores is handled in SongBrowser::onScoreContextMenu()
        // this is nice because it updates the songbrowser scorebrowser too (so if the user closes the top ranks screen
        // everything is in sync, even for the currently selected beatmap)
        m_osu->getSongBrowser()->onScoreContextMenu(this, 2);
        m_osu->getUserStatsScreen()->onScoreContextMenu(this, 2);
    }
}

void UISongBrowserScoreButton::setScore(const FinishedScore &score, const DatabaseBeatmap *diff2, int index,
                                        UString titleString, float weight) {
    m_score = score;
    m_iScoreIndexNumber = index;

    const float accuracy =
        LiveScore::calculateAccuracy(score.num300s, score.num100s, score.num50s, score.numMisses) * 100.0f;
    const bool modHidden = score.modsLegacy & ModFlags::Hidden;
    const bool modFlashlight = score.modsLegacy & ModFlags::Flashlight;

    const bool fullCombo = (!score.isImportedLegacyScore && !score.isLegacyScore && score.maxPossibleCombo > 0 &&
                            score.numMisses == 0 && score.numSliderBreaks == 0);  // NOTE: allows dropped sliderends

    if(m_avatar) {
        delete m_avatar;
        m_avatar = nullptr;
    }
    if(score.player_id != 0) {
        m_avatar = new UIAvatar(score.player_id, m_vPos.x, m_vPos.y, m_vSize.y, m_vSize.y);

        auto user = get_user_info(score.player_id);
        is_friend = user->is_friend();
    }

    // display
    m_scoreGrade = LiveScore::calculateGrade(score.num300s, score.num100s, score.num50s, score.numMisses, modHidden,
                                             modFlashlight);
    m_sScoreUsername = UString(score.playerName.c_str());
    m_sScoreScore = UString::format(
        (score.perfect ? "Score: %llu (%ix PFC)" : (fullCombo ? "Score: %llu (%ix FC)" : "Score: %llu (%ix)")),
        score.score, score.comboMax);
    m_sScoreScorePP =
        UString::format((score.perfect ? "PP: %ipp (%ix PFC)" : (fullCombo ? "PP: %ipp (%ix FC)" : "PP: %ipp (%ix)")),
                        (int)std::round(score.pp), score.comboMax);
    m_sScoreAccuracy = UString::format("%.2f%%", accuracy);
    m_sScoreAccuracyFC =
        UString::format((score.perfect ? "PFC %.2f%%" : (fullCombo ? "FC %.2f%%" : "%.2f%%")), accuracy);
    m_sScoreMods = getModsStringForDisplay(score.modsLegacy);
    if(score.experimentalModsConVars.length() > 0) {
        if(m_sScoreMods.length() > 0) m_sScoreMods.append(",");

        auto cv = UString(score.experimentalModsConVars.c_str());
        std::vector<UString> experimentalMods = cv.split(";");
        for(int i = 0; i < experimentalMods.size(); i++) {
            if(experimentalMods[i].length() > 0) m_sScoreMods.append("+");
        }
    }
    m_sCustom = (score.speedMultiplier != 1.0f ? UString::format("Spd: %gx", score.speedMultiplier) : "");
    if(diff2 != NULL && !score.isImportedLegacyScore && !score.isLegacyScore) {
        const Replay::BEATMAP_VALUES beatmapValuesForModsLegacy = Replay::getBeatmapValuesForModsLegacy(
            score.modsLegacy, diff2->getAR(), diff2->getCS(), diff2->getOD(), diff2->getHP());

        const float compensatedCS = std::round(score.CS * 100.0f) / 100.0f;
        const float compensatedAR = std::round(GameRules::getRawApproachRateForSpeedMultiplier(
                                                   GameRules::getRawApproachTime(score.AR), score.speedMultiplier) *
                                               100.0f) /
                                    100.0f;
        const float compensatedOD = std::round(GameRules::getRawOverallDifficultyForSpeedMultiplier(
                                                   GameRules::getRawHitWindow300(score.OD), score.speedMultiplier) *
                                               100.0f) /
                                    100.0f;
        const float compensatedHP = std::round(score.HP * 100.0f) / 100.0f;

        // only show these values if they are not default (or default with applied mods)
        // only show these values if they are not default with applied mods

        if(beatmapValuesForModsLegacy.CS != score.CS) {
            if(m_sCustom.length() > 0) m_sCustom.append(", ");

            m_sCustom.append(UString::format("CS:%.4g", compensatedCS));
        }

        if(beatmapValuesForModsLegacy.AR != score.AR) {
            if(m_sCustom.length() > 0) m_sCustom.append(", ");

            m_sCustom.append(UString::format("AR:%.4g", compensatedAR));
        }

        if(beatmapValuesForModsLegacy.OD != score.OD) {
            if(m_sCustom.length() > 0) m_sCustom.append(", ");

            m_sCustom.append(UString::format("OD:%.4g", compensatedOD));
        }

        if(beatmapValuesForModsLegacy.HP != score.HP) {
            if(m_sCustom.length() > 0) m_sCustom.append(", ");

            m_sCustom.append(UString::format("HP:%.4g", compensatedHP));
        }
    }

    char dateString[64];
    memset(dateString, '\0', 64);
    std::tm *tm = std::localtime((std::time_t *)(&score.unixTimestamp));
    std::strftime(dateString, 63, "%d-%b-%y %H:%M:%S", tm);
    m_sScoreDateTime = UString(dateString);
    m_iScoreUnixTimestamp = score.unixTimestamp;

    UString achievedOn = "Achieved on ";
    achievedOn.append(m_sScoreDateTime);

    // tooltip
    m_tooltipLines.clear();
    m_tooltipLines.push_back(achievedOn);

    if(m_score.isLegacyScore || m_score.isImportedLegacyScore)
        m_tooltipLines.push_back(UString::format("300:%i 100:%i 50:%i Miss:%i", score.num300s, score.num100s,
                                                 score.num50s, score.numMisses));
    else
        m_tooltipLines.push_back(UString::format("300:%i 100:%i 50:%i Miss:%i SBreak:%i", score.num300s, score.num100s,
                                                 score.num50s, score.numMisses, score.numSliderBreaks));

    m_tooltipLines.push_back(UString::format("Accuracy: %.2f%%", accuracy));

    UString tooltipMods = "Mods: ";
    if(m_sScoreMods.length() > 0)
        tooltipMods.append(m_sScoreMods);
    else
        tooltipMods.append("None");

    m_tooltipLines.push_back(tooltipMods);
    if(score.experimentalModsConVars.length() > 0) {
        auto cv = UString(score.experimentalModsConVars.c_str());
        std::vector<UString> experimentalMods = cv.split(";");
        for(int i = 0; i < experimentalMods.size(); i++) {
            if(experimentalMods[i].length() > 0) {
                UString experimentalModString = "+ ";
                experimentalModString.append(experimentalMods[i]);
                m_tooltipLines.push_back(experimentalModString);
            }
        }
    }

    if(m_style == STYLE::TOP_RANKS) {
        const int weightRounded = std::round(weight * 100.0f);
        const int ppWeightedRounded = std::round(score.pp * weight);

        m_sScoreTitle = titleString;
        m_sScoreScorePPWeightedPP = UString::format("%ipp", (int)std::round(score.pp));
        m_sScoreScorePPWeightedWeight = UString::format("     weighted %i%% (%ipp)", weightRounded, ppWeightedRounded);
        m_sScoreWeight = UString::format("weighted %i%%", weightRounded);

        // m_tooltipLines.push_back("Difficulty:");
        m_tooltipLines.push_back(UString::format("Stars: %.2f (%.2f aim, %.2f speed)", score.starsTomTotal,
                                                 score.starsTomAim, score.starsTomSpeed));
        m_tooltipLines.push_back(UString::format("Speed: %.3gx", score.speedMultiplier));
        m_tooltipLines.push_back(
            UString::format("CS:%.4g AR:%.4g OD:%.4g HP:%.4g", score.CS, score.AR, score.OD, score.HP));
        // m_tooltipLines.push_back("Accuracy:");
        if(!score.isImportedLegacyScore) {
            m_tooltipLines.push_back(
                UString::format("Error: %.2fms - %.2fms avg", score.hitErrorAvgMin, score.hitErrorAvgMax));
            m_tooltipLines.push_back(UString::format("Unstable Rate: %.2f", score.unstableRate));
        } else
            m_tooltipLines.push_back("This score was imported from osu!");

        m_tooltipLines.push_back(UString::format("Version: %i", score.version));
    }

    // custom
    updateElapsedTimeString();
}

bool UISongBrowserScoreButton::isContextMenuVisible() { return (m_contextMenu != NULL && m_contextMenu->isVisible()); }

SkinImage *UISongBrowserScoreButton::getGradeImage(Osu *osu, FinishedScore::Grade grade) {
    switch(grade) {
        case FinishedScore::Grade::XH:
            return osu->getSkin()->getRankingXHsmall();
        case FinishedScore::Grade::SH:
            return osu->getSkin()->getRankingSHsmall();
        case FinishedScore::Grade::X:
            return osu->getSkin()->getRankingXsmall();
        case FinishedScore::Grade::S:
            return osu->getSkin()->getRankingSsmall();
        case FinishedScore::Grade::A:
            return osu->getSkin()->getRankingAsmall();
        case FinishedScore::Grade::B:
            return osu->getSkin()->getRankingBsmall();
        case FinishedScore::Grade::C:
            return osu->getSkin()->getRankingCsmall();
        default:
            return osu->getSkin()->getRankingDsmall();
    }
}

UString UISongBrowserScoreButton::getModsStringForDisplay(int mods) {
    UString modsString;

    if(mods & ModFlags::NoFail) modsString.append("NF,");
    if(mods & ModFlags::Easy) modsString.append("EZ,");
    if(mods & ModFlags::TouchDevice) modsString.append("TD,");
    if(mods & ModFlags::Hidden) modsString.append("HD,");
    if(mods & ModFlags::HardRock) modsString.append("HR,");
    if(mods & ModFlags::SuddenDeath) modsString.append("SD,");
    if(mods & ModFlags::Nightcore)
        modsString.append("NC,");
    else if(mods & ModFlags::DoubleTime)
        modsString.append("DT,");
    if(mods & ModFlags::Relax) modsString.append("Relax,");
    if(mods & ModFlags::HalfTime) modsString.append("HT,");
    if(mods & ModFlags::Flashlight) modsString.append("FL,");
    if(mods & ModFlags::Autoplay) modsString.append("AT,");
    if(mods & ModFlags::SpunOut) modsString.append("SO,");
    if(mods & ModFlags::Autopilot) modsString.append("AP,");
    if(mods & ModFlags::Perfect) modsString.append("PF,");
    if(mods & ModFlags::ScoreV2) modsString.append("v2,");
    if(mods & ModFlags::Target) modsString.append("Target,");
    if(mods & ModFlags::Nightmare) modsString.append("NM,");
    if(mods & ModFlags::Mirror) modsString.append("Mirror,");
    if(mods & ModFlags::FPoSu) modsString.append("FPoSu,");

    if(modsString.length() > 0) modsString = modsString.substr(0, modsString.length() - 1);

    return modsString;
}
