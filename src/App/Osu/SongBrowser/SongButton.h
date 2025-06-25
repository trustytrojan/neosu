#pragma once
#include "Button.h"
#include "score.h"

class SongBrowser;
class DatabaseBeatmap;

class SongButton : public Button {
   public:
    SongButton(SongBrowser *songBrowser, CBaseUIScrollView *view, UIContextMenu *contextMenu, float xPos, float yPos,
               float xSize, float ySize, UString name, DatabaseBeatmap *databaseBeatmap);
    ~SongButton() override;

    void draw(Graphics *g) override;

    void triggerContextMenu(Vector2 pos);

    void sortChildren();

    void updateLayoutEx() override;
    virtual void updateGrade() { ; }

    DatabaseBeatmap *getDatabaseBeatmap() const override { return this->databaseBeatmap; }
    FinishedScore::Grade grade = FinishedScore::Grade::N;

   protected:
    void onSelected(bool wasSelected, bool autoSelectBottomMostChild, bool wasParentSelected) override;
    void onRightMouseUpInside() override;

    void onContextMenu(UString text, int id = -1);
    void onAddToCollectionConfirmed(UString text, int id = -1);
    void onCreateNewCollectionConfirmed(UString text, int id = -1);

    void drawBeatmapBackgroundThumbnail(Graphics *g, Image *image);
    void drawGrade(Graphics *g);
    void drawTitle(Graphics *g, float deselectedAlpha = 1.0f, bool forceSelectedStyle = false);
    void drawSubTitle(Graphics *g, float deselectedAlpha = 1.0f, bool forceSelectedStyle = false);

    float calculateGradeScale();
    float calculateGradeWidth();

    DatabaseBeatmap *databaseBeatmap;

    std::string sTitle;
    std::string sArtist;
    std::string sMapper;

    float fTextOffset;
    float fGradeOffset;
    float fTextSpacingScale;
    float fTextMarginScale;
    float fTitleScale;
    float fSubTitleScale;
    float fGradeScale;

   private:
    float fThumbnailFadeInTime;
};
