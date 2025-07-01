#include "CBaseUIElement.h"

#include "Engine.h"
#include "Mouse.h"
#include "Osu.h"
#include "TooltipOverlay.h"

bool CBaseUIElement::isVisibleOnScreen() {
    if(!isVisible()) return false;
    const McRect visrect{{0, 0}, engine->getScreenSize()};
    const Vector2 visrectCenter{visrect.getCenter()};
    const Vector2 elemPosNudgedIn{Vector2{this->vPos.x, this->vPos.y}.nudge(visrectCenter, -5.0f)};
    return visrect.contains(elemPosNudgedIn);
}

void CBaseUIElement::mouse_update(bool *propagate_clicks) {
    // check if mouse is inside element
    McRect temp = McRect(this->vPos.x + 1, this->vPos.y + 1, this->vSize.x - 1, this->vSize.y - 1);
    if(temp.contains(mouse->getPos())) {
        if(!this->bMouseInside) {
            this->bMouseInside = true;
            if(this->bVisible && this->bEnabled) this->onMouseInside();
        }
    } else {
        if(this->bMouseInside) {
            this->bMouseInside = false;
            if(this->bVisible && this->bEnabled) this->onMouseOutside();
        }
    }

    if(!this->bVisible) return;

    if(!this->bEnabled) {
        if(this->bMouseInside && this->disabled_reason != NULL) {
            osu->getTooltipOverlay()->begin();
            osu->getTooltipOverlay()->addLine(this->disabled_reason);
            osu->getTooltipOverlay()->end();
        }

        return;
    }

    if(mouse->isLeftDown() && *propagate_clicks) {
        this->bMouseUpCheck = true;
        if(this->bMouseInside) {
            *propagate_clicks = !this->grabs_clicks;
        }

        // onMouseDownOutside
        if(!this->bMouseInside && !this->bMouseInsideCheck) {
            this->bMouseInsideCheck = true;
            this->onMouseDownOutside();
        }

        // onMouseDownInside
        if(this->bMouseInside && !this->bMouseInsideCheck) {
            this->bActive = true;
            this->bMouseInsideCheck = true;
            this->onMouseDownInside();
        }
    } else {
        if(this->bMouseUpCheck) {
            if(this->bActive) {
                if(this->bMouseInside)
                    this->onMouseUpInside();
                else
                    this->onMouseUpOutside();

                if(!this->bKeepActive) this->bActive = false;
            }
        }

        this->bMouseInsideCheck = false;
        this->bMouseUpCheck = false;
    }
}
