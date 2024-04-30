//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		collection button (also used for grouping)
//
// $NoKeywords: $osusbcb
//===============================================================================//

#ifndef OSUUISONGBROWSERCOLLECTIONBUTTON_H
#define OSUUISONGBROWSERCOLLECTIONBUTTON_H

#include "UISongBrowserButton.h"

class UISongBrowserCollectionButton : public UISongBrowserButton {
   public:
    UISongBrowserCollectionButton(Osu *osu, SongBrowser *songBrowser, CBaseUIScrollView *view,
                                  UIContextMenu *contextMenu, float xPos, float yPos, float xSize, float ySize,
                                  UString name, UString collectionName, std::vector<UISongBrowserButton *> children);

    virtual void draw(Graphics *g);

    void triggerContextMenu(Vector2 pos);

    virtual Color getActiveBackgroundColor() const;
    virtual Color getInactiveBackgroundColor() const;

    const std::string &getCollectionName() const { return m_sCollectionName; }

   private:
    virtual void onSelected(bool wasSelected, bool autoSelectBottomMostChild, bool wasParentSelected);
    virtual void onRightMouseUpInside();

    void onContextMenu(UString text, int id = -1);
    void onRenameCollectionConfirmed(UString text, int id = -1);
    void onDeleteCollectionConfirmed(UString text, int id = -1);

    std::string m_sCollectionName;

    float m_fTitleScale;
};

#endif
