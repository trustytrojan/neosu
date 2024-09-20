#pragma once
#include "CBaseUIButton.h"

class UIButton : public CBaseUIButton {
   public:
    UIButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text);

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    void setColor(Color color) {
        m_color = color;
        m_backupColor = color;
    }
    void setUseDefaultSkin() { m_bDefaultSkin = true; }
    void setAlphaAddOnHover(float alphaAddOnHover) { m_fAlphaAddOnHover = alphaAddOnHover; }

    void setTooltipText(UString text);

    virtual void onMouseInside();
    virtual void onMouseOutside();

    void animateClickColor();
    bool is_loading = false;

    // HACKHACK: enough is enough
    bool m_bVisible2 = true;

   private:
    virtual void onClicked();
    virtual void onFocusStolen();

    bool m_bDefaultSkin;
    Color m_color;
    Color m_backupColor;
    float m_fBrightness;
    float m_fAnim;
    float m_fAlphaAddOnHover;

    std::vector<UString> m_tooltipTextLines;
    bool m_bFocusStolenDelay;
};
