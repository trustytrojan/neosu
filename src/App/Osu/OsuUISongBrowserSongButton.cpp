//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap + diff button
//
// $NoKeywords: $osusbsb
//===============================================================================//

#include "OsuUISongBrowserSongButton.h"

#include "Collections.h"
#include "ConVar.h"
#include "Engine.h"
#include "Mouse.h"
#include "Osu.h"
#include "OsuBackgroundImageHandler.h"
#include "OsuDatabaseBeatmap.h"
#include "OsuNotificationOverlay.h"
#include "OsuSkin.h"
#include "OsuSkinImage.h"
#include "OsuSongBrowser.h"
#include "OsuUIContextMenu.h"
#include "OsuUISongBrowserCollectionButton.h"
#include "OsuUISongBrowserScoreButton.h"
#include "OsuUISongBrowserSongDifficultyButton.h"
#include "ResourceManager.h"

ConVar osu_draw_songbrowser_thumbnails("osu_draw_songbrowser_thumbnails", true, FCVAR_NONE);
ConVar osu_songbrowser_thumbnail_delay("osu_songbrowser_thumbnail_delay", 0.1f, FCVAR_NONE);
ConVar osu_songbrowser_thumbnail_fade_in_duration("osu_songbrowser_thumbnail_fade_in_duration", 0.1f, FCVAR_NONE);

float OsuUISongBrowserSongButton::thumbnailYRatio = 1.333333f;

OsuUISongBrowserSongButton::OsuUISongBrowserSongButton(Osu *osu, OsuSongBrowser *songBrowser, CBaseUIScrollView *view,
                                                       OsuUIContextMenu *contextMenu, float xPos, float yPos,
                                                       float xSize, float ySize, UString name,
                                                       OsuDatabaseBeatmap *databaseBeatmap)
    : OsuUISongBrowserButton(osu, songBrowser, view, contextMenu, xPos, yPos, xSize, ySize, name) {
    m_databaseBeatmap = databaseBeatmap;

    m_grade = Score::Grade::D;
    m_bHasGrade = false;

    // settings
    setHideIfSelected(true);

    // labels
    m_fThumbnailFadeInTime = 0.0f;
    m_fTextOffset = 0.0f;
    m_fGradeOffset = 0.0f;
    m_fTextSpacingScale = 0.075f;
    m_fTextMarginScale = 0.075f;
    m_fTitleScale = 0.22f;
    m_fSubTitleScale = 0.14f;
    m_fGradeScale = 0.45f;

    // build children
    if(m_databaseBeatmap != NULL) {
        const std::vector<OsuDatabaseBeatmap *> &difficulties = m_databaseBeatmap->getDifficulties();

        // and add them
        for(int i = 0; i < difficulties.size(); i++) {
            OsuUISongBrowserSongButton *songButton = new OsuUISongBrowserSongDifficultyButton(
                m_osu, m_songBrowser, m_view, m_contextMenu, 0, 0, 0, 0, "", difficulties[i], this);

            m_children.push_back(songButton);
        }
    }

    updateLayoutEx();
}

OsuUISongBrowserSongButton::~OsuUISongBrowserSongButton() {
    for(int i = 0; i < m_children.size(); i++) {
        delete m_children[i];
    }
}

void OsuUISongBrowserSongButton::draw(Graphics *g) {
    OsuUISongBrowserButton::draw(g);
    if(!m_bVisible) return;

    if(m_vPos.y + m_vSize.y < 0) return;
    if(m_vPos.y > engine->getScreenHeight()) return;

    // draw background image
    sortChildren();
    if(m_databaseBeatmap != NULL && m_children.size() > 0) {
        // use the bottom child (hardest diff, assuming default sorting, and respecting the current search matches)
        for(int i = m_children.size() - 1; i >= 0; i--) {
            // NOTE: if no search is active, then all search matches return true by default
            if(m_children[i]->isSearchMatch()) {
                auto representative_beatmap =
                    dynamic_cast<OsuUISongBrowserSongButton *>(m_children[i])->getDatabaseBeatmap();

                m_sTitle = representative_beatmap->getTitle();
                m_sArtist = representative_beatmap->getArtist();
                m_sMapper = representative_beatmap->getCreator();

                drawBeatmapBackgroundThumbnail(
                    g, m_osu->getBackgroundImageHandler()->getLoadBackgroundImage(representative_beatmap));

                break;
            }
        }
    }

    drawTitle(g);
    drawSubTitle(g);
}

void OsuUISongBrowserSongButton::drawBeatmapBackgroundThumbnail(Graphics *g, Image *image) {
    if(!osu_draw_songbrowser_thumbnails.getBool() || m_osu->getSkin()->getVersion() < 2.2f) return;

    float alpha = 1.0f;
    if(osu_songbrowser_thumbnail_fade_in_duration.getFloat() > 0.0f) {
        if(image == NULL || !image->isReady())
            m_fThumbnailFadeInTime = engine->getTime();
        else if(m_fThumbnailFadeInTime > 0.0f && engine->getTime() > m_fThumbnailFadeInTime) {
            alpha = clamp<float>(
                (engine->getTime() - m_fThumbnailFadeInTime) / osu_songbrowser_thumbnail_fade_in_duration.getFloat(),
                0.0f, 1.0f);
            alpha = 1.0f - (1.0f - alpha) * (1.0f - alpha);
        }
    }

    if(image == NULL || !image->isReady()) return;

    // scaling
    const Vector2 pos = getActualPos();
    const Vector2 size = getActualSize();

    const float beatmapBackgroundScale =
        Osu::getImageScaleToFillResolution(image, Vector2(size.y * thumbnailYRatio, size.y)) * 1.05f;

    Vector2 centerOffset = Vector2(std::round((size.y * thumbnailYRatio) / 2.0f), std::round(size.y / 2.0f));
    McRect clipRect = McRect(pos.x - 2, pos.y + 1, (int)(size.y * thumbnailYRatio) + 5, size.y + 2);

    g->setColor(0xffffffff);
    g->setAlpha(alpha);
    g->pushTransform();
    {
        g->scale(beatmapBackgroundScale, beatmapBackgroundScale);
        g->translate(pos.x + (int)centerOffset.x, pos.y + (int)centerOffset.y);
        g->pushClipRect(clipRect);
        { g->drawImage(image); }
        g->popClipRect();
    }
    g->popTransform();

    // debug cliprect bounding box
    if(Osu::debug->getBool()) {
        Vector2 clipRectPos = Vector2(clipRect.getX(), clipRect.getY() - 1);
        Vector2 clipRectSize = Vector2(clipRect.getWidth(), clipRect.getHeight());

        g->setColor(0xffffff00);
        g->drawLine(clipRectPos.x, clipRectPos.y, clipRectPos.x + clipRectSize.x, clipRectPos.y);
        g->drawLine(clipRectPos.x, clipRectPos.y, clipRectPos.x, clipRectPos.y + clipRectSize.y);
        g->drawLine(clipRectPos.x, clipRectPos.y + clipRectSize.y, clipRectPos.x + clipRectSize.x,
                    clipRectPos.y + clipRectSize.y);
        g->drawLine(clipRectPos.x + clipRectSize.x, clipRectPos.y, clipRectPos.x + clipRectSize.x,
                    clipRectPos.y + clipRectSize.y);
    }
}

void OsuUISongBrowserSongButton::drawGrade(Graphics *g) {
    // scaling
    const Vector2 pos = getActualPos();
    const Vector2 size = getActualSize();

    OsuSkinImage *grade = OsuUISongBrowserScoreButton::getGradeImage(m_osu, m_grade);
    g->pushTransform();
    {
        const float scale = calculateGradeScale();

        g->setColor(0xffffffff);
        grade->drawRaw(g, Vector2(pos.x + m_fGradeOffset + grade->getSizeBaseRaw().x * scale / 2, pos.y + size.y / 2),
                       scale);
    }
    g->popTransform();
}

void OsuUISongBrowserSongButton::drawTitle(Graphics *g, float deselectedAlpha, bool forceSelectedStyle) {
    // scaling
    const Vector2 pos = getActualPos();
    const Vector2 size = getActualSize();

    const float titleScale = (size.y * m_fTitleScale) / m_font->getHeight();
    g->setColor((m_bSelected || forceSelectedStyle) ? m_osu->getSkin()->getSongSelectActiveText()
                                                    : m_osu->getSkin()->getSongSelectInactiveText());
    if(!(m_bSelected || forceSelectedStyle)) g->setAlpha(deselectedAlpha);

    g->pushTransform();
    {
        UString title = m_sTitle.c_str();
        g->scale(titleScale, titleScale);
        g->translate(pos.x + m_fTextOffset, pos.y + size.y * m_fTextMarginScale + m_font->getHeight() * titleScale);
        g->drawString(m_font, title);
    }
    g->popTransform();
}

void OsuUISongBrowserSongButton::drawSubTitle(Graphics *g, float deselectedAlpha, bool forceSelectedStyle) {
    // scaling
    const Vector2 pos = getActualPos();
    const Vector2 size = getActualSize();

    const float titleScale = (size.y * m_fTitleScale) / m_font->getHeight();
    const float subTitleScale = (size.y * m_fSubTitleScale) / m_font->getHeight();
    g->setColor((m_bSelected || forceSelectedStyle) ? m_osu->getSkin()->getSongSelectActiveText()
                                                    : m_osu->getSkin()->getSongSelectInactiveText());
    if(!(m_bSelected || forceSelectedStyle)) g->setAlpha(deselectedAlpha);

    g->pushTransform();
    {
        UString subTitleString = m_sArtist.c_str();
        subTitleString.append(" // ");
        subTitleString.append(m_sMapper.c_str());

        g->scale(subTitleScale, subTitleScale);
        g->translate(pos.x + m_fTextOffset, pos.y + size.y * m_fTextMarginScale + m_font->getHeight() * titleScale +
                                                size.y * m_fTextSpacingScale +
                                                m_font->getHeight() * subTitleScale * 0.85f);
        g->drawString(m_font, subTitleString);
    }
    g->popTransform();
}

void OsuUISongBrowserSongButton::sortChildren() {
    std::sort(m_children.begin(), m_children.end(), OsuSongBrowser::SortByDifficulty());
}

void OsuUISongBrowserSongButton::updateLayoutEx() {
    OsuUISongBrowserButton::updateLayoutEx();

    // scaling
    const Vector2 size = getActualSize();

    m_fTextOffset = 0.0f;
    m_fGradeOffset = 0.0f;

    if(m_bHasGrade) m_fTextOffset += calculateGradeWidth();

    if(m_osu->getSkin()->getVersion() < 2.2f) {
        m_fTextOffset += size.x * 0.02f * 2.0f;
        if(m_bHasGrade) m_fGradeOffset += calculateGradeWidth() / 2;
    } else {
        m_fTextOffset += size.y * thumbnailYRatio + size.x * 0.02f;
        m_fGradeOffset += size.y * thumbnailYRatio + size.x * 0.0125f;
    }
}

void OsuUISongBrowserSongButton::onSelected(bool wasSelected, bool autoSelectBottomMostChild, bool wasParentSelected) {
    OsuUISongBrowserButton::onSelected(wasSelected, autoSelectBottomMostChild, wasParentSelected);

    // resort children (since they might have been updated in the meantime)
    sortChildren();

    // update grade on child
    for(int c = 0; c < m_children.size(); c++) {
        OsuUISongBrowserSongDifficultyButton *child = (OsuUISongBrowserSongDifficultyButton *)m_children[c];
        child->updateGrade();
    }

    m_songBrowser->onSelectionChange(this, false);

    // now, automatically select the bottom child (hardest diff, assuming default sorting, and respecting the current
    // search matches)
    if(autoSelectBottomMostChild) {
        for(int i = m_children.size() - 1; i >= 0; i--) {
            if(m_children[i]
                   ->isSearchMatch())  // NOTE: if no search is active, then all search matches return true by default
            {
                m_children[i]->select(true, false, wasSelected);
                break;
            }
        }
    }
}

void OsuUISongBrowserSongButton::onRightMouseUpInside() { triggerContextMenu(engine->getMouse()->getPos()); }

void OsuUISongBrowserSongButton::triggerContextMenu(Vector2 pos) {
    if(m_contextMenu != NULL) {
        m_contextMenu->setPos(pos);
        m_contextMenu->setRelPos(pos);
        m_contextMenu->begin(0, true);
        {
            if(m_databaseBeatmap != NULL && m_databaseBeatmap->getDifficulties().size() < 1)
                m_contextMenu->addButton("[+]         Add to Collection", 1);

            m_contextMenu->addButton("[+Set]   Add to Collection", 2);

            if(m_osu->getSongBrowser()->getGroupingMode() == OsuSongBrowser::GROUP::GROUP_COLLECTIONS) {
                CBaseUIButton *spacer = m_contextMenu->addButton("---");
                spacer->setTextLeft(false);
                spacer->setEnabled(false);
                spacer->setTextColor(0xff888888);
                spacer->setTextDarkColor(0xff000000);

                if(m_databaseBeatmap == NULL || m_databaseBeatmap->getDifficulties().size() < 1) {
                    m_contextMenu->addButton("[-]          Remove from Collection", 3);
                }

                m_contextMenu->addButton("[-Set]    Remove from Collection", 4);
            }
        }
        m_contextMenu->end(false, false);
        m_contextMenu->setClickCallback(fastdelegate::MakeDelegate(this, &OsuUISongBrowserSongButton::onContextMenu));
        OsuUIContextMenu::clampToRightScreenEdge(m_contextMenu);
        OsuUIContextMenu::clampToBottomScreenEdge(m_contextMenu);
    }
}

void OsuUISongBrowserSongButton::onContextMenu(UString text, int id) {
    if(id == 1 || id == 2) {
        // 1 = add map to collection
        // 2 = add set to collection
        m_contextMenu->begin(0, true);
        {
            m_contextMenu->addButton("[+]   Create new Collection?", -id * 2);

            for(auto collection : collections) {
                if(!collection->maps.empty()) {
                    CBaseUIButton *spacer = m_contextMenu->addButton("---");
                    spacer->setTextLeft(false);
                    spacer->setEnabled(false);
                    spacer->setTextColor(0xff888888);
                    spacer->setTextDarkColor(0xff000000);
                    break;
                }
            }

            auto map_hash = m_databaseBeatmap->getMD5Hash();
            for(auto collection : collections) {
                if(collection->maps.empty()) continue;

                bool can_add_to_collection = true;

                if(id == 1) {
                    auto it = std::find(collection->maps.begin(), collection->maps.end(), map_hash);
                    if(it != collection->maps.end()) {
                        // Map already is present in the collection
                        can_add_to_collection = false;
                    }
                }

                if(id == 2) {
                    // XXX: Don't mark as valid if the set is fully present in the collection
                }

                auto collectionButton = m_contextMenu->addButton(UString(collection->name.c_str()), id);
                if(!can_add_to_collection) {
                    collectionButton->setEnabled(false);
                    collectionButton->setTextColor(0xff555555);
                    collectionButton->setTextDarkColor(0xff000000);
                }
            }
        }
        m_contextMenu->end(false, true);
        m_contextMenu->setClickCallback(
            fastdelegate::MakeDelegate(this, &OsuUISongBrowserSongButton::onAddToCollectionConfirmed));
        OsuUIContextMenu::clampToRightScreenEdge(m_contextMenu);
        OsuUIContextMenu::clampToBottomScreenEdge(m_contextMenu);
    } else if(id == 3 || id == 4) {
        // 3 = remove map from collection
        // 4 = remove set from collection
        m_osu->getSongBrowser()->onSongButtonContextMenu(this, text, id);
    }
}

void OsuUISongBrowserSongButton::onAddToCollectionConfirmed(UString text, int id) {
    if(id == -2 || id == -4) {
        m_contextMenu->begin(0, true);
        {
            CBaseUIButton *label = m_contextMenu->addButton("Enter Collection Name:");
            label->setTextLeft(false);
            label->setEnabled(false);

            CBaseUIButton *spacer = m_contextMenu->addButton("---");
            spacer->setTextLeft(false);
            spacer->setEnabled(false);
            spacer->setTextColor(0xff888888);
            spacer->setTextDarkColor(0xff000000);

            m_contextMenu->addTextbox("", id);

            spacer = m_contextMenu->addButton("---");
            spacer->setTextLeft(false);
            spacer->setEnabled(false);
            spacer->setTextColor(0xff888888);
            spacer->setTextDarkColor(0xff000000);

            label = m_contextMenu->addButton("(Press ENTER to confirm.)", id);
            label->setTextLeft(false);
            label->setTextColor(0xff555555);
            label->setTextDarkColor(0xff000000);
        }
        m_contextMenu->end(false, false);
        m_contextMenu->setClickCallback(
            fastdelegate::MakeDelegate(this, &OsuUISongBrowserSongButton::onCreateNewCollectionConfirmed));
        OsuUIContextMenu::clampToRightScreenEdge(m_contextMenu);
        OsuUIContextMenu::clampToBottomScreenEdge(m_contextMenu);
    } else {
        // just forward it
        m_osu->getSongBrowser()->onSongButtonContextMenu(this, text, id);
    }
}

void OsuUISongBrowserSongButton::onCreateNewCollectionConfirmed(UString text, int id) {
    if(id == -2 || id == -4) {
        // just forward it
        m_osu->getSongBrowser()->onSongButtonContextMenu(this, text, id);
    }
}

float OsuUISongBrowserSongButton::calculateGradeScale() {
    const Vector2 size = getActualSize();

    OsuSkinImage *grade = OsuUISongBrowserScoreButton::getGradeImage(m_osu, m_grade);

    return Osu::getImageScaleToFitResolution(grade->getSizeBaseRaw(), Vector2(size.x, size.y * m_fGradeScale));
}

float OsuUISongBrowserSongButton::calculateGradeWidth() {
    OsuSkinImage *grade = OsuUISongBrowserScoreButton::getGradeImage(m_osu, m_grade);

    return grade->getSizeBaseRaw().x * calculateGradeScale();
}
