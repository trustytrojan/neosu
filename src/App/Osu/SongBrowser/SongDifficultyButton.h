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
    virtual void onClicked();

    virtual void updateGrade();

    virtual Color getInactiveBackgroundColor() const;

    inline SongButton *getParentSongButton() const { return this->parentSongButton; }

    bool isIndependentDiffButton() const;

   private:
    virtual void onSelected(bool wasSelected, bool autoSelectBottomMostChild, bool wasParentSelected);

    std::string sDiff;

    float fDiffScale;
    float fOffsetPercentAnim;

    SongButton *parentSongButton;

    bool bUpdateGradeScheduled;
    bool bPrevOffsetPercentSelectionState;
};
