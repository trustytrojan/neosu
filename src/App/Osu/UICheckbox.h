#pragma once
#include "CBaseUICheckbox.h"

class UICheckbox : public CBaseUICheckbox {
   public:
    UICheckbox(float xPos, float yPos, float xSize, float ySize, UString name, UString text);

    void mouse_update(bool *propagate_clicks) override;

    void setTooltipText(UString text);

   private:
    void onFocusStolen() override;

    std::vector<UString> tooltipTextLines;

    bool bFocusStolenDelay;
};
