#pragma once
#include "SongButton.h"

class ConVar;

class SongDifficultyButton : public SongButton {
   public:
    SongDifficultyButton(SongBrowser *songBrowser, CBaseUIScrollView *view, UIContextMenu *contextMenu, float xPos,
                         float yPos, float xSize, float ySize, UString name, DatabaseBeatmap *diff2,
                         SongButton *parentSongButton);
    virtual ~SongDifficultyButton();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    virtual void updateGrade();

    virtual Color getInactiveBackgroundColor() const;

    inline SongButton *getParentSongButton() const { return m_parentSongButton; }

    bool isIndependentDiffButton() const;

   private:
    static ConVar *m_osu_scores_enabled;
    static ConVar *m_osu_songbrowser_dynamic_star_recalc_ref;

    virtual void onSelected(bool wasSelected, bool autoSelectBottomMostChild, bool wasParentSelected);

    std::string m_sDiff;

    float m_fDiffScale;
    float m_fOffsetPercentAnim;

    SongButton *m_parentSongButton;

    bool m_bUpdateGradeScheduled;
    bool m_bPrevOffsetPercentSelectionState;
};
