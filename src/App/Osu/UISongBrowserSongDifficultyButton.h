//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		beatmap difficulty button (child of UISongBrowserSongButton)
//
// $NoKeywords: $osusbsdb
//===============================================================================//

#ifndef OSUUISONGBROWSERSONGDIFFICULTYBUTTON_H
#define OSUUISONGBROWSERSONGDIFFICULTYBUTTON_H

#include "UISongBrowserSongButton.h"

class ConVar;

class UISongBrowserSongDifficultyButton : public UISongBrowserSongButton {
   public:
    UISongBrowserSongDifficultyButton(Osu *osu, SongBrowser *songBrowser, CBaseUIScrollView *view,
                                      UIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize,
                                      UString name, DatabaseBeatmap *diff2, UISongBrowserSongButton *parentSongButton);
    virtual ~UISongBrowserSongDifficultyButton();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    virtual void updateGrade();

    virtual Color getInactiveBackgroundColor() const;

    inline UISongBrowserSongButton *getParentSongButton() const { return m_parentSongButton; }

    bool isIndependentDiffButton() const;

   private:
    static ConVar *m_osu_scores_enabled;
    static ConVar *m_osu_songbrowser_dynamic_star_recalc_ref;

    virtual void onSelected(bool wasSelected, bool autoSelectBottomMostChild, bool wasParentSelected);

    std::string m_sDiff;

    float m_fDiffScale;
    float m_fOffsetPercentAnim;

    UISongBrowserSongButton *m_parentSongButton;

    bool m_bUpdateGradeScheduled;
    bool m_bPrevOffsetPercentSelectionState;
};

#endif
