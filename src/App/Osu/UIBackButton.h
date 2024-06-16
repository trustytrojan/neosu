#pragma once
#include "CBaseUIButton.h"

class UIBackButton : public CBaseUIButton {
   public:
    UIBackButton(float xPos, float yPos, float xSize, float ySize, UString name);

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    virtual void onMouseDownInside();
    virtual void onMouseInside();
    virtual void onMouseOutside();

    virtual void updateLayout();

    void resetAnimation();

   private:
    float m_fAnimation;
    float m_fImageScale;
};
