#pragma once
#include "CBaseUIButton.h"

class UIButton : public CBaseUIButton {
   public:
    UIButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text);

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    void setColor(Color color) {
        this->color = color;
        this->backupColor = color;
    }
    void setUseDefaultSkin() { this->bDefaultSkin = true; }
    void setAlphaAddOnHover(float alphaAddOnHover) { this->fAlphaAddOnHover = alphaAddOnHover; }

    void setTooltipText(UString text);

    virtual void onMouseInside();
    virtual void onMouseOutside();

    void animateClickColor();
    bool is_loading = false;

    // HACKHACK: enough is enough
    bool bVisible2 = true;

   private:
    virtual void onClicked();
    virtual void onFocusStolen();

    bool bDefaultSkin;
    Color color;
    Color backupColor;
    float fBrightness;
    float fAnim;
    float fAlphaAddOnHover;

    std::vector<UString> tooltipTextLines;
    bool bFocusStolenDelay;
};
