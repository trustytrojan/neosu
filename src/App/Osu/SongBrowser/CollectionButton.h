#pragma once
#include "Button.h"

class CollectionButton : public Button {
   public:
    CollectionButton(SongBrowser *songBrowser, CBaseUIScrollView *view, UIContextMenu *contextMenu, float xPos,
                     float yPos, float xSize, float ySize, UString name, UString collectionName,
                     std::vector<SongButton *> children);

    virtual void draw(Graphics *g);

    void triggerContextMenu(Vector2 pos);

    virtual Color getActiveBackgroundColor() const;
    virtual Color getInactiveBackgroundColor() const;

    const std::string &getCollectionName() const { return this->sCollectionName; }

   private:
    virtual void onSelected(bool wasSelected, bool autoSelectBottomMostChild, bool wasParentSelected);
    virtual void onRightMouseUpInside();

    void onContextMenu(UString text, int id = -1);
    void onRenameCollectionConfirmed(UString text, int id = -1);
    void onDeleteCollectionConfirmed(UString text, int id = -1);

    std::string sCollectionName;

    float fTitleScale;
};
