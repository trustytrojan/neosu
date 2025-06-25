#pragma once
#include "CBaseUIButton.h"

class UIBackButton : public CBaseUIButton {
   public:
    UIBackButton(float xPos, float yPos, float xSize, float ySize, UString name);

    void draw() override;
    void mouse_update(bool *propagate_clicks) override;

    void onMouseDownInside() override;
    void onMouseInside() override;
    void onMouseOutside() override;

    virtual void updateLayout();

    void resetAnimation();

   private:
    float fAnimation;
    float fImageScale;
};
