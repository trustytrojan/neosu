#pragma once
// Copyright (c) 2016, PG, All rights reserved.
#include "CarouselButton.h"
#include "score.h"

class SongBrowser;
class DatabaseBeatmap;
class BeatmapCarousel;

class SongButton : public CarouselButton {
   public:
    SongButton(SongBrowser *songBrowser, UIContextMenu *contextMenu, float xPos, float yPos,
               float xSize, float ySize, UString name, std::shared_ptr<DatabaseBeatmap> databaseBeatmap);
    ~SongButton() override;

    void draw() override;

    void triggerContextMenu(vec2 pos);

    void sortChildren();

    void updateLayoutEx() override;
    virtual void updateGrade() { ; }

    [[nodiscard]] std::shared_ptr<DatabaseBeatmap> getDatabaseBeatmap() const override { return this->databaseBeatmap; }
    FinishedScore::Grade grade = FinishedScore::Grade::N;

   protected:
    void onSelected(bool wasSelected, bool autoSelectBottomMostChild, bool wasParentSelected) override;
    void onRightMouseUpInside() override;

    void onContextMenu(const UString &text, int id = -1);
    void onAddToCollectionConfirmed(const UString &text, int id = -1);
    void onCreateNewCollectionConfirmed(const UString &text, int id = -1);

    void drawBeatmapBackgroundThumbnail(Image *image);
    void drawGrade();
    void drawTitle(float deselectedAlpha = 1.0f, bool forceSelectedStyle = false);
    void drawSubTitle(float deselectedAlpha = 1.0f, bool forceSelectedStyle = false);

    float calculateGradeScale();
    float calculateGradeWidth();

    std::shared_ptr<DatabaseBeatmap> databaseBeatmap;

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
