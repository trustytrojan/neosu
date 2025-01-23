//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		song browser selection button (mode, mods, random, etc)
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUUISELECTIONBUTTON_H
#define OSUUISELECTIONBUTTON_H

#include "CBaseUIButton.h"

class SkinImage;

class UISelectionButton : public CBaseUIButton {
   public:
    UISelectionButton(std::function<SkinImage *()> getImageFunc, std::function<SkinImage *()> getImageOverFunc,
                      float xPos, float yPos, float xSize, float ySize, UString name);

    void draw(Graphics *g);

    virtual void onMouseInside();
    virtual void onMouseOutside();

    virtual void onResized();

    void keyboardPulse();

   private:
    float fAnimation;

    std::function<SkinImage *()> getImageFunc;
    std::function<SkinImage *()> getImageOverFunc;
};

#endif
