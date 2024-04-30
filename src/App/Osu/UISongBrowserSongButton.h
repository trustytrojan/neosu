//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap + diff button
//
// $NoKeywords: $osusbsb
//===============================================================================//

#ifndef OSUUISONGBROWSERSONGBUTTON_H
#define OSUUISONGBROWSERSONGBUTTON_H

#include "UISongBrowserButton.h"
#include "score.h"

class SongBrowser;
class DatabaseBeatmap;

class UISongBrowserSongButton : public UISongBrowserButton {
   public:
    UISongBrowserSongButton(Osu *osu, SongBrowser *songBrowser, CBaseUIScrollView *view, UIContextMenu *contextMenu,
                            float xPos, float yPos, float xSize, float ySize, UString name,
                            DatabaseBeatmap *databaseBeatmap);
    virtual ~UISongBrowserSongButton();

    virtual void draw(Graphics *g);

    void triggerContextMenu(Vector2 pos);

    void sortChildren();

    virtual void updateLayoutEx();
    virtual void updateGrade() { ; }

    virtual DatabaseBeatmap *getDatabaseBeatmap() const { return m_databaseBeatmap; }

   protected:
    virtual void onSelected(bool wasSelected, bool autoSelectBottomMostChild, bool wasParentSelected);
    virtual void onRightMouseUpInside();

    void onContextMenu(UString text, int id = -1);
    void onAddToCollectionConfirmed(UString text, int id = -1);
    void onCreateNewCollectionConfirmed(UString text, int id = -1);

    void drawBeatmapBackgroundThumbnail(Graphics *g, Image *image);
    void drawGrade(Graphics *g);
    void drawTitle(Graphics *g, float deselectedAlpha = 1.0f, bool forceSelectedStyle = false);
    void drawSubTitle(Graphics *g, float deselectedAlpha = 1.0f, bool forceSelectedStyle = false);

    float calculateGradeScale();
    float calculateGradeWidth();

    DatabaseBeatmap *m_databaseBeatmap;

    std::string m_sTitle;
    std::string m_sArtist;
    std::string m_sMapper;
    FinishedScore::Grade m_grade;
    bool m_bHasGrade;

    float m_fTextOffset;
    float m_fGradeOffset;
    float m_fTextSpacingScale;
    float m_fTextMarginScale;
    float m_fTitleScale;
    float m_fSubTitleScale;
    float m_fGradeScale;

   private:
    static float thumbnailYRatio;

    float m_fThumbnailFadeInTime;
};

#endif
