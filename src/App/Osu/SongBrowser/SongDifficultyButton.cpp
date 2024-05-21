#include "SongDifficultyButton.h"

#include "ScoreButton.h"
#include "SongBrowser.h"
// ---

#include "AnimationHandler.h"
#include "BackgroundImageHandler.h"
#include "Beatmap.h"
#include "ConVar.h"
#include "Database.h"
#include "DatabaseBeatmap.h"
#include "Engine.h"
#include "Osu.h"
#include "Replay.h"
#include "ResourceManager.h"
#include "Skin.h"
#include "Timer.h"

ConVar osu_songbrowser_button_difficulty_inactive_color_a("osu_songbrowser_button_difficulty_inactive_color_a", 255,
                                                          FCVAR_DEFAULT);
ConVar osu_songbrowser_button_difficulty_inactive_color_r("osu_songbrowser_button_difficulty_inactive_color_r", 0,
                                                          FCVAR_DEFAULT);
ConVar osu_songbrowser_button_difficulty_inactive_color_g("osu_songbrowser_button_difficulty_inactive_color_g", 150,
                                                          FCVAR_DEFAULT);
ConVar osu_songbrowser_button_difficulty_inactive_color_b("osu_songbrowser_button_difficulty_inactive_color_b", 236,
                                                          FCVAR_DEFAULT);

ConVar *SongDifficultyButton::m_osu_scores_enabled = NULL;

SongDifficultyButton::SongDifficultyButton(SongBrowser *songBrowser, CBaseUIScrollView *view,
                                           UIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize,
                                           UString name, DatabaseBeatmap *diff2, SongButton *parentSongButton)
    : SongButton(songBrowser, view, contextMenu, xPos, yPos, xSize, ySize, name, NULL) {
    m_databaseBeatmap = diff2;  // NOTE: can't use parent constructor for passing this argument, as it would otherwise
                                // try to build a full button (and not just a diff button)
    m_parentSongButton = parentSongButton;

    if(m_osu_scores_enabled == NULL) m_osu_scores_enabled = convar->getConVarByName("osu_scores_enabled");

    m_sTitle = m_databaseBeatmap->getTitle();
    m_sArtist = m_databaseBeatmap->getArtist();
    m_sMapper = m_databaseBeatmap->getCreator();
    m_sDiff = m_databaseBeatmap->getDifficultyName();

    m_fDiffScale = 0.18f;
    m_fOffsetPercentAnim = (m_parentSongButton != NULL ? 1.0f : 0.0f);

    m_bUpdateGradeScheduled = true;
    m_bPrevOffsetPercentSelectionState = isIndependentDiffButton();

    // settings
    setHideIfSelected(false);

    updateLayoutEx();
}

SongDifficultyButton::~SongDifficultyButton() { anim->deleteExistingAnimation(&m_fOffsetPercentAnim); }

void SongDifficultyButton::draw(Graphics *g) {
    Button::draw(g);
    if(!m_bVisible) return;

    const bool isIndependentDiff = isIndependentDiffButton();

    Skin *skin = osu->getSkin();

    // scaling
    const Vector2 pos = getActualPos();
    const Vector2 size = getActualSize();

    // draw background image
    drawBeatmapBackgroundThumbnail(g, osu->getBackgroundImageHandler()->getLoadBackgroundImage(m_databaseBeatmap));

    if(m_bHasGrade) drawGrade(g);

    drawTitle(g, !isIndependentDiff ? 0.2f : 1.0f);
    drawSubTitle(g, !isIndependentDiff ? 0.2f : 1.0f);

    // draw diff name
    const float titleScale = (size.y * m_fTitleScale) / m_font->getHeight();
    const float subTitleScale = (size.y * m_fSubTitleScale) / m_font->getHeight();
    const float diffScale = (size.y * m_fDiffScale) / m_fontBold->getHeight();
    g->setColor(m_bSelected ? skin->getSongSelectActiveText() : skin->getSongSelectInactiveText());
    g->pushTransform();
    {
        g->scale(diffScale, diffScale);
        g->translate(pos.x + m_fTextOffset,
                     pos.y + size.y * m_fTextMarginScale + m_font->getHeight() * titleScale +
                         size.y * m_fTextSpacingScale + m_font->getHeight() * subTitleScale * 0.85f +
                         size.y * m_fTextSpacingScale + m_fontBold->getHeight() * diffScale * 0.8f);
        g->drawString(m_fontBold, m_sDiff.c_str());
    }
    g->popTransform();

    // draw stars
    // NOTE: stars can sometimes be infinity! (e.g. broken osu!.db database)
    float stars = m_databaseBeatmap->getStarsNomod();
    if(m_bSelected && osu->getSelectedBeatmap()->getSelectedDifficulty2()->m_pp_info.ok) {
        stars = osu->getSelectedBeatmap()->getSelectedDifficulty2()->m_pp_info.total_stars;
    }
    if(stars > 0) {
        const float starOffsetY = (size.y * 0.85);
        const float starWidth = (size.y * 0.2);
        const float starScale = starWidth / skin->getStar()->getHeight();
        const int numFullStars = clamp<int>((int)stars, 0, 25);
        const float partialStarScale = std::max(0.5f, clamp<float>(stars - numFullStars, 0.0f, 1.0f));  // at least 0.5x

        g->setColor(m_bSelected ? skin->getSongSelectActiveText() : skin->getSongSelectInactiveText());

        // full stars
        for(int i = 0; i < numFullStars; i++) {
            const float scale =
                std::min(starScale * 1.175f, starScale + i * 0.015f);  // more stars = getting bigger, up to a limit

            g->pushTransform();
            {
                g->scale(scale, scale);
                g->translate(pos.x + m_fTextOffset + starWidth / 2 + i * starWidth * 1.75f, pos.y + starOffsetY);
                g->drawImage(skin->getStar());
            }
            g->popTransform();
        }

        // partial star
        g->pushTransform();
        {
            g->scale(starScale * partialStarScale, starScale * partialStarScale);
            g->translate(pos.x + m_fTextOffset + starWidth / 2 + numFullStars * starWidth * 1.75f, pos.y + starOffsetY);
            g->drawImage(skin->getStar());
        }
        g->popTransform();

        // fill leftover space up to 10 stars total (background stars)
        g->setColor(0x1effffff);
        const float backgroundStarScale = 0.6f;

        g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ADDITIVE);
        {
            for(int i = (numFullStars + 1); i < 10; i++) {
                g->pushTransform();
                {
                    g->scale(starScale * backgroundStarScale, starScale * backgroundStarScale);
                    g->translate(pos.x + m_fTextOffset + starWidth / 2 + i * starWidth * 1.75f, pos.y + starOffsetY);
                    g->drawImage(skin->getStar());
                }
                g->popTransform();
            }
        }
        g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);
    }
}

void SongDifficultyButton::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible) return;
    SongButton::mouse_update(propagate_clicks);

    // dynamic settings (moved from constructor to here)
    const bool newOffsetPercentSelectionState = (m_bSelected || !isIndependentDiffButton());
    if(newOffsetPercentSelectionState != m_bPrevOffsetPercentSelectionState) {
        m_bPrevOffsetPercentSelectionState = newOffsetPercentSelectionState;
        anim->moveQuadOut(&m_fOffsetPercentAnim, newOffsetPercentSelectionState ? 1.0f : 0.0f,
                          0.25f * (1.0f - m_fOffsetPercentAnim), true);
    }
    setOffsetPercent(lerp<float>(0.0f, 0.075f, m_fOffsetPercentAnim));

    if(m_bUpdateGradeScheduled) {
        m_bUpdateGradeScheduled = false;
        updateGrade();
    }
}

void SongDifficultyButton::onSelected(bool wasSelected, bool autoSelectBottomMostChild, bool wasParentSelected) {
    Button::onSelected(wasSelected, autoSelectBottomMostChild, wasParentSelected);

    const bool wasParentActuallySelected = (m_parentSongButton != NULL && wasParentSelected);

    updateGrade();

    if(!wasParentActuallySelected) m_songBrowser->requestNextScrollToSongButtonJumpFix(this);

    m_songBrowser->onSelectionChange(this, true);
    m_songBrowser->onDifficultySelected(m_databaseBeatmap, wasSelected);
    m_songBrowser->scrollToSongButton(this);
}

void SongDifficultyButton::updateGrade() {
    if(!m_osu_scores_enabled->getBool()) {
        m_bHasGrade = false;
        return;
    }

    bool hasGrade = false;
    FinishedScore::Grade grade;

    osu->getSongBrowser()->getDatabase()->sortScores(m_databaseBeatmap->getMD5Hash());
    if((*osu->getSongBrowser()->getDatabase()->getScores())[m_databaseBeatmap->getMD5Hash()].size() > 0) {
        const FinishedScore &score =
            (*osu->getSongBrowser()->getDatabase()->getScores())[m_databaseBeatmap->getMD5Hash()][0];
        hasGrade = true;
        grade = LiveScore::calculateGrade(score.num300s, score.num100s, score.num50s, score.numMisses,
                                          score.modsLegacy & ModFlags::Hidden, score.modsLegacy & ModFlags::Flashlight);
    }

    m_bHasGrade = hasGrade;
    if(m_bHasGrade) m_grade = grade;
}

bool SongDifficultyButton::isIndependentDiffButton() const {
    return (m_parentSongButton == NULL || !m_parentSongButton->isSelected());
}

Color SongDifficultyButton::getInactiveBackgroundColor() const {
    if(isIndependentDiffButton())
        return SongButton::getInactiveBackgroundColor();
    else
        return COLOR(clamp<int>(osu_songbrowser_button_difficulty_inactive_color_a.getInt(), 0, 255),
                     clamp<int>(osu_songbrowser_button_difficulty_inactive_color_r.getInt(), 0, 255),
                     clamp<int>(osu_songbrowser_button_difficulty_inactive_color_g.getInt(), 0, 255),
                     clamp<int>(osu_songbrowser_button_difficulty_inactive_color_b.getInt(), 0, 255));
}
