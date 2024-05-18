#include "UICheckbox.h"

#include "Engine.h"
#include "Osu.h"
#include "TooltipOverlay.h"

UICheckbox::UICheckbox(float xPos, float yPos, float xSize, float ySize, UString name, UString text)
    : CBaseUICheckbox(xPos, yPos, xSize, ySize, name, text) {
    m_bFocusStolenDelay = false;
}

void UICheckbox::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible) return;
    CBaseUICheckbox::mouse_update(propagate_clicks);

    if(isMouseInside() && m_tooltipTextLines.size() > 0 && !m_bFocusStolenDelay) {
        osu->getTooltipOverlay()->begin();
        {
            for(int i = 0; i < m_tooltipTextLines.size(); i++) {
                osu->getTooltipOverlay()->addLine(m_tooltipTextLines[i]);
            }
        }
        osu->getTooltipOverlay()->end();
    }

    m_bFocusStolenDelay = false;
}

void UICheckbox::onFocusStolen() {
    CBaseUICheckbox::onFocusStolen();

    m_bMouseInside = false;
    m_bFocusStolenDelay = true;
}

void UICheckbox::setTooltipText(UString text) { m_tooltipTextLines = text.split("\n"); }
