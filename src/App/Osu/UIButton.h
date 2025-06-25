#pragma once
#include "CBaseUIButton.h"

class UIButton : public CBaseUIButton {
   public:
    UIButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text);

    void draw(Graphics *g) override;
    void mouse_update(bool *propagate_clicks) override;

    void setColor(Color color) {
        this->color = color;
        this->backupColor = color;
    }
    void setUseDefaultSkin() { this->bDefaultSkin = true; }
    void setAlphaAddOnHover(float alphaAddOnHover) { this->fAlphaAddOnHover = alphaAddOnHover; }

    void setTooltipText(UString text);

    void onMouseInside() override;
    void onMouseOutside() override;

    void animateClickColor();
    bool is_loading = false;

    // HACKHACK: enough is enough
    bool bVisible2 = true;

   private:
    void onClicked() override;
    void onFocusStolen() override;

    bool bDefaultSkin;
    Color color;
    Color backupColor;
    float fBrightness;
    float fAnim;
    float fAlphaAddOnHover;

    std::vector<UString> tooltipTextLines;
    bool bFocusStolenDelay;
};
