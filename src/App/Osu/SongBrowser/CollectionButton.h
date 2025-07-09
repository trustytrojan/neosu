#pragma once
#include "Button.h"

class CollectionButton : public Button {
   public:
    CollectionButton(SongBrowser *songBrowser, CBaseUIScrollView *view, UIContextMenu *contextMenu, float xPos,
                     float yPos, float xSize, float ySize, UString name, const UString& collectionName,
                     std::vector<SongButton *> children);

    void draw() override;

    void triggerContextMenu(Vector2 pos);

    [[nodiscard]] Color getActiveBackgroundColor() const override;
    [[nodiscard]] Color getInactiveBackgroundColor() const override;

    [[nodiscard]] const std::string &getCollectionName() const { return this->sCollectionName; }

   private:
    void onSelected(bool wasSelected, bool autoSelectBottomMostChild, bool wasParentSelected) override;
    void onRightMouseUpInside() override;

    void onContextMenu(UString text, int id = -1);
    void onRenameCollectionConfirmed(UString text, int id = -1);
    void onDeleteCollectionConfirmed(UString text, int id = -1);

    std::string sCollectionName;

    float fTitleScale;
};
