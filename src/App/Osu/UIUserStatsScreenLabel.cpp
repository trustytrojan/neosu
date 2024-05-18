#include "UIUserStatsScreenLabel.h"

#include "Osu.h"
#include "TooltipOverlay.h"

UIUserStatsScreenLabel::UIUserStatsScreenLabel(float xPos, float yPos, float xSize, float ySize, UString name,
                                               UString text)
    : CBaseUILabel() {}

void UIUserStatsScreenLabel::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible) return;
    CBaseUILabel::mouse_update(propagate_clicks);

    if(isMouseInside()) {
        bool isEmpty = true;
        for(size_t i = 0; i < m_tooltipTextLines.size(); i++) {
            if(m_tooltipTextLines[i].length() > 0) {
                isEmpty = false;
                break;
            }
        }

        if(!isEmpty) {
            osu->getTooltipOverlay()->begin();
            {
                for(size_t i = 0; i < m_tooltipTextLines.size(); i++) {
                    if(m_tooltipTextLines[i].length() > 0) osu->getTooltipOverlay()->addLine(m_tooltipTextLines[i]);
                }
            }
            osu->getTooltipOverlay()->end();
        }
    }
}
