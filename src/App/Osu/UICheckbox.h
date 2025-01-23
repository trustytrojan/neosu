#pragma once
#include "CBaseUICheckbox.h"

class UICheckbox : public CBaseUICheckbox {
   public:
    UICheckbox(float xPos, float yPos, float xSize, float ySize, UString name, UString text);

    virtual void mouse_update(bool *propagate_clicks);

    void setTooltipText(UString text);

   private:
    virtual void onFocusStolen();

    std::vector<UString> tooltipTextLines;

    bool bFocusStolenDelay;
};
