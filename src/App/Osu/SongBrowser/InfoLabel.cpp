#include "InfoLabel.h"

#include "SongBrowser.h"
// ---

#include "Beatmap.h"
#include "ConVar.h"
#include "DatabaseBeatmap.h"
#include "Engine.h"
#include "Environment.h"
#include "GameRules.h"
#include "Mouse.h"
#include "NotificationOverlay.h"
#include "OptionsMenu.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "TooltipOverlay.h"

InfoLabel::InfoLabel(float xPos, float yPos, float xSize, float ySize, UString name)
    : CBaseUIButton(xPos, yPos, xSize, ySize, name, "") {
    m_font = osu->getSubTitleFont();

    m_iMargin = 10;

    const float globalScaler = 1.3f;

    m_fTitleScale = 0.77f * globalScaler;
    m_fSubTitleScale = 0.6f * globalScaler;
    m_fSongInfoScale = 0.7f * globalScaler;
    m_fDiffInfoScale = 0.65f * globalScaler;
    m_fOffsetInfoScale = 0.6f * globalScaler;

    m_sArtist = "Artist";
    m_sTitle = "Title";
    m_sDiff = "Difficulty";
    m_sMapper = "Mapper";

    m_iLengthMS = 0;
    m_iMinBPM = 0;
    m_iMaxBPM = 0;
    m_iMostCommonBPM = 0;
    m_iNumObjects = 0;

    m_fCS = 5.0f;
    m_fAR = 5.0f;
    m_fOD = 5.0f;
    m_fHP = 5.0f;
    m_fStars = 5.0f;

    m_iLocalOffset = 0;
    m_iOnlineOffset = 0;

    m_iBeatmapId = -1;
}

void InfoLabel::draw(Graphics *g) {
    // debug bounding box
    if(cv_debug.getBool()) {
        g->setColor(0xffff0000);
        g->drawLine(m_vPos.x, m_vPos.y, m_vPos.x + m_vSize.x, m_vPos.y);
        g->drawLine(m_vPos.x, m_vPos.y, m_vPos.x, m_vPos.y + m_vSize.y);
        g->drawLine(m_vPos.x, m_vPos.y + m_vSize.y, m_vPos.x + m_vSize.x, m_vPos.y + m_vSize.y);
        g->drawLine(m_vPos.x + m_vSize.x, m_vPos.y, m_vPos.x + m_vSize.x, m_vPos.y + m_vSize.y);
    }

    // build strings
    UString titleText = m_sArtist.c_str();
    titleText.append(" - ");
    titleText.append(m_sTitle.c_str());
    titleText.append(" [");
    titleText.append(m_sDiff.c_str());
    titleText.append("]");
    UString subTitleText = "Mapped by ";
    subTitleText.append(m_sMapper.c_str());
    const UString songInfoText = buildSongInfoString();
    const UString diffInfoText = buildDiffInfoString();
    const UString offsetInfoText = buildOffsetInfoString();

    const float globalScale = max((m_vSize.y / getMinimumHeight()) * 0.91f, 1.0f);

    const int shadowOffset = std::round(1.0f * ((float)m_font->getDPI() / 96.0f));  // NOTE: abusing font dpi

    int yCounter = m_vPos.y;

    // draw title
    g->pushTransform();
    {
        const float scale = m_fTitleScale * globalScale;

        yCounter += m_font->getHeight() * scale;

        g->scale(scale, scale);
        g->translate((int)(m_vPos.x), yCounter);

        g->translate(shadowOffset, shadowOffset);
        g->setColor(0xff000000);
        g->drawString(m_font, titleText);
        g->translate(-shadowOffset, -shadowOffset);
        g->setColor(0xffffffff);
        g->drawString(m_font, titleText);
    }
    g->popTransform();

    // draw subtitle (mapped by)
    g->pushTransform();
    {
        const float scale = m_fSubTitleScale * globalScale;

        yCounter += m_font->getHeight() * scale + m_iMargin * globalScale * 1.0f;

        g->scale(scale, scale);
        g->translate((int)(m_vPos.x), yCounter);

        g->translate(shadowOffset, shadowOffset);
        g->setColor(0xff000000);
        g->drawString(m_font, subTitleText);
        g->translate(-shadowOffset, -shadowOffset);
        g->setColor(0xffffffff);
        g->drawString(m_font, subTitleText);
    }
    g->popTransform();

    // draw song info (length, bpm, objects)
    const Color songInfoColor =
        (osu->getSelectedBeatmap()->getSpeedMultiplier() != 1.0f
             ? (osu->getSelectedBeatmap()->getSpeedMultiplier() > 1.0f ? 0xffff7f7f : 0xffadd8e6)
             : 0xffffffff);
    g->pushTransform();
    {
        const float scale = m_fSongInfoScale * globalScale * 0.9f;

        yCounter += m_font->getHeight() * scale + m_iMargin * globalScale * 1.0f;

        g->scale(scale, scale);
        g->translate((int)(m_vPos.x), yCounter);

        g->translate(shadowOffset, shadowOffset);
        g->setColor(0xff000000);
        g->drawString(m_font, songInfoText);
        g->translate(-shadowOffset, -shadowOffset);
        g->setColor(songInfoColor);
        g->drawString(m_font, songInfoText);
    }
    g->popTransform();

    // draw diff info (CS, AR, OD, HP, Stars)
    const Color diffInfoColor = osu->getModEZ() ? 0xffadd8e6 : (osu->getModHR() ? 0xffff7f7f : 0xffffffff);
    g->pushTransform();
    {
        const float scale = m_fDiffInfoScale * globalScale * 0.9f;

        yCounter += m_font->getHeight() * scale + m_iMargin * globalScale * 0.85f;

        g->scale(scale, scale);
        g->translate((int)(m_vPos.x), yCounter);

        g->translate(shadowOffset, shadowOffset);
        g->setColor(0xff000000);
        g->drawString(m_font, diffInfoText);
        g->translate(-shadowOffset, -shadowOffset);
        g->setColor(diffInfoColor);
        g->drawString(m_font, diffInfoText);
    }
    g->popTransform();

    // draw offset (local, online)
    if(m_iLocalOffset != 0 || m_iOnlineOffset != 0) {
        g->pushTransform();
        {
            const float scale = m_fOffsetInfoScale * globalScale * 0.8f;

            yCounter += m_font->getHeight() * scale + m_iMargin * globalScale * 0.85f;

            g->scale(scale, scale);
            g->translate((int)(m_vPos.x), yCounter);

            g->translate(shadowOffset, shadowOffset);
            g->setColor(0xff000000);
            g->drawString(m_font, offsetInfoText);
            g->translate(-shadowOffset, -shadowOffset);
            g->setColor(0xffffffff);
            g->drawString(m_font, offsetInfoText);
        }
        g->popTransform();
    }
}

void InfoLabel::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible) return;
    CBaseUIButton::mouse_update(propagate_clicks);

    // detail info tooltip when hovering over diff info
    if(isMouseInside() && !osu->getOptionsMenu()->isMouseInside()) {
        Beatmap *beatmap = osu->getSelectedBeatmap();
        if(beatmap != NULL) {
            const float speedMultiplierInv = (1.0f / beatmap->getSpeedMultiplier());

            const float approachTimeRoundedCompensated = ((int)beatmap->getApproachTime()) * speedMultiplierInv;
            const float hitWindow300RoundedCompensated = ((int)beatmap->getHitWindow300() - 0.5f) * speedMultiplierInv;
            const float hitWindow100RoundedCompensated = ((int)beatmap->getHitWindow100() - 0.5f) * speedMultiplierInv;
            const float hitWindow50RoundedCompensated = ((int)beatmap->getHitWindow50() - 0.5f) * speedMultiplierInv;
            const float hitobjectRadiusRoundedCompensated =
                (GameRules::getRawHitCircleDiameter(beatmap->getCS()) / 2.0f);

            osu->getTooltipOverlay()->begin();
            {
                osu->getTooltipOverlay()->addLine(
                    UString::format("Approach time: %.2fms", approachTimeRoundedCompensated));
                osu->getTooltipOverlay()->addLine(UString::format("300: +-%.2fms", hitWindow300RoundedCompensated));
                osu->getTooltipOverlay()->addLine(UString::format("100: +-%.2fms", hitWindow100RoundedCompensated));
                osu->getTooltipOverlay()->addLine(UString::format(" 50: +-%.2fms", hitWindow50RoundedCompensated));
                osu->getTooltipOverlay()->addLine(
                    UString::format("Spinner difficulty: %.2f", GameRules::getSpinnerSpinsPerSecond(beatmap)));
                osu->getTooltipOverlay()->addLine(
                    UString::format("Hit object radius: %.2f", hitobjectRadiusRoundedCompensated));

                if(beatmap->getSelectedDifficulty2() != NULL) {
                    int numObjects = beatmap->getSelectedDifficulty2()->getNumObjects();
                    int numCircles = beatmap->getSelectedDifficulty2()->getNumCircles();
                    int numSliders = beatmap->getSelectedDifficulty2()->getNumSliders();
                    unsigned long lengthMS = beatmap->getSelectedDifficulty2()->getLengthMS();

                    const float opm =
                        (lengthMS > 0 ? ((float)numObjects / (float)(lengthMS / 1000.0f / 60.0f)) : 0.0f) *
                        beatmap->getSpeedMultiplier();
                    const float cpm =
                        (lengthMS > 0 ? ((float)numCircles / (float)(lengthMS / 1000.0f / 60.0f)) : 0.0f) *
                        beatmap->getSpeedMultiplier();
                    const float spm =
                        (lengthMS > 0 ? ((float)numSliders / (float)(lengthMS / 1000.0f / 60.0f)) : 0.0f) *
                        beatmap->getSpeedMultiplier();

                    osu->getTooltipOverlay()->addLine(UString::format("Circles: %i, Sliders: %i, Spinners: %i",
                                                                      numCircles, numSliders,
                                                                      max(0, numObjects - numCircles - numSliders)));
                    osu->getTooltipOverlay()->addLine(
                        UString::format("OPM: %i, CPM: %i, SPM: %i", (int)opm, (int)cpm, (int)spm));
                    osu->getTooltipOverlay()->addLine(UString::format("ID: %i, SetID: %i",
                                                                      beatmap->getSelectedDifficulty2()->getID(),
                                                                      beatmap->getSelectedDifficulty2()->getSetID()));
                    osu->getTooltipOverlay()->addLine(
                        UString::format("MD5: %s", beatmap->getSelectedDifficulty2()->getMD5Hash().toUtf8()));
                }
            }
            osu->getTooltipOverlay()->end();
        }
    }
}

void InfoLabel::setFromBeatmap(Beatmap *beatmap, DatabaseBeatmap *diff2) {
    m_iBeatmapId = diff2->getID();

    setArtist(diff2->getArtist());
    setTitle(diff2->getTitle());
    setDiff(diff2->getDifficultyName());
    setMapper(diff2->getCreator());

    setLengthMS(diff2->getLengthMS());
    setBPM(diff2->getMinBPM(), diff2->getMaxBPM(), diff2->getMostCommonBPM());
    setNumObjects(diff2->getNumObjects());

    setCS(diff2->getCS());
    setAR(diff2->getAR());
    setOD(diff2->getOD());
    setHP(diff2->getHP());
    setStars(diff2->getStarsNomod());

    setLocalOffset(diff2->getLocalOffset());
    setOnlineOffset(diff2->getOnlineOffset());
}

UString InfoLabel::buildSongInfoString() {
    unsigned long lengthMS = m_iLengthMS;
    auto speed = osu->getSelectedBeatmap()->getSpeedMultiplier();

    const unsigned long fullSeconds = (lengthMS * (1.0 / speed)) / 1000.0;
    const int minutes = fullSeconds / 60;
    const int seconds = fullSeconds % 60;

    const int minBPM = m_iMinBPM * speed;
    const int maxBPM = m_iMaxBPM * speed;
    const int mostCommonBPM = m_iMostCommonBPM * speed;

    int numObjects = m_iNumObjects;
    if(m_iMinBPM == m_iMaxBPM) {
        return UString::format("Length: %02i:%02i BPM: %i Objects: %i", minutes, seconds, maxBPM, numObjects);
    } else {
        return UString::format("Length: %02i:%02i BPM: %i-%i (%i) Objects: %i", minutes, seconds, minBPM, maxBPM,
                               mostCommonBPM, numObjects);
    }
}

UString InfoLabel::buildDiffInfoString() {
    bool pp_available = false;
    float CS = m_fCS;
    float AR = m_fAR;
    float OD = m_fOD;
    float HP = m_fHP;
    float stars = m_fStars;
    float modStars = 0.f;
    float modPp = 0.f;

    Beatmap *beatmap = osu->getSelectedBeatmap();
    if(beatmap != NULL) {
        CS = beatmap->getCS();
        AR = beatmap->getApproachRateForSpeedMultiplier();
        OD = beatmap->getOverallDifficultyForSpeedMultiplier();
        HP = beatmap->getHP();

        auto diff2 = beatmap->getSelectedDifficulty2();
        if(diff2) {
            modStars = diff2->m_pp_info.total_stars;
            modPp = diff2->m_pp_info.pp;
            if(diff2->m_pp_info.pp != -1.0) {
                pp_available = true;
            }
        }
    }

    const float starComparisonEpsilon = 0.01f;
    const bool starsAndModStarsAreEqual = (std::abs(stars - modStars) < starComparisonEpsilon);

    UString finalString;
    if(pp_available) {
        if(starsAndModStarsAreEqual) {
            finalString = UString::format("CS:%.3g AR:%.3g OD:%.3g HP:%.3g Stars:%.3g (%ipp)", CS, AR, OD, HP, stars,
                                          (int)(std::round(modPp)));
        } else {
            finalString = UString::format("CS:%.3g AR:%.3g OD:%.3g HP:%.3g Stars:%.3g -> %.3g (%ipp)", CS, AR, OD, HP,
                                          stars, modStars, (int)(std::round(modPp)));
        }
    } else {
        finalString = UString::format("CS:%.3g AR:%.3g OD:%.3g HP:%.3g Stars:%.3g * (??? pp)", CS, AR, OD, HP, stars);
    }

    return finalString;
}

UString InfoLabel::buildOffsetInfoString() {
    return UString::format("Your Offset: %ld ms / Online Offset: %ld ms", m_iLocalOffset, m_iOnlineOffset);
}

float InfoLabel::getMinimumWidth() {
    float titleWidth = 0;
    float subTitleWidth = 0;
    float songInfoWidth = m_font->getStringWidth(buildSongInfoString()) * m_fSongInfoScale;
    float diffInfoWidth = m_font->getStringWidth(buildDiffInfoString()) * m_fDiffInfoScale;
    float offsetInfoWidth = m_font->getStringWidth(buildOffsetInfoString()) * m_fOffsetInfoScale;

    return max(max(max(max(titleWidth, subTitleWidth), songInfoWidth), diffInfoWidth), offsetInfoWidth);
}

float InfoLabel::getMinimumHeight() {
    float titleHeight = m_font->getHeight() * m_fTitleScale;
    float subTitleHeight = m_font->getHeight() * m_fSubTitleScale;
    float songInfoHeight = m_font->getHeight() * m_fSongInfoScale;
    float diffInfoHeight = m_font->getHeight() * m_fDiffInfoScale;
    /// float offsetInfoHeight = m_font->getHeight() * m_fOffsetInfoScale;

    return titleHeight + subTitleHeight + songInfoHeight + diffInfoHeight /* + offsetInfoHeight*/ +
           m_iMargin * 3;  // this is commented on purpose (also, it should be m_iMargin*4 but the 3 is also on purpose)
}
