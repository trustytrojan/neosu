//================ Copyright (c) 2022, PG, All rights reserved. =================//
//
// Purpose:		currently only used for showing the pp/star algorithm versions
//
// $NoKeywords: $
//===============================================================================//

#include "UIUserStatsScreenLabel.h"

#include "Osu.h"
#include "TooltipOverlay.h"

UIUserStatsScreenLabel::UIUserStatsScreenLabel(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name,
                                               UString text)
    : CBaseUILabel() {
    m_osu = osu;
}

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
            m_osu->getTooltipOverlay()->begin();
            {
                for(size_t i = 0; i < m_tooltipTextLines.size(); i++) {
                    if(m_tooltipTextLines[i].length() > 0) m_osu->getTooltipOverlay()->addLine(m_tooltipTextLines[i]);
                }
            }
            m_osu->getTooltipOverlay()->end();
        }
    }
}
