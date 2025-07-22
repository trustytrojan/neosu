#include "CBaseUIElement.h"

#include "Engine.h"
#include "Mouse.h"
#include "Osu.h"
#include "TooltipOverlay.h"

bool CBaseUIElement::isVisibleOnScreen() {
    if(!isVisible()) return false;
    const McRect visrect{{0, 0}, engine->getScreenSize()};
    const Vector2 &visrectCenter = visrect.getCenter();
    const Vector2 elemPosNudgedIn{Vector2{this->vPos.x, this->vPos.y}.nudge(visrectCenter, -5.0f)};
    return visrect.contains(elemPosNudgedIn);
}

void CBaseUIElement::stealFocus() {
    uint8_t checkLeft = this->bHandleLeftMouse ? 0b10 : 0;
    uint8_t checkRight = this->bHandleRightMouse ? 0b01 : 0;
    this->mouseInsideCheck = checkLeft | checkRight;
    this->bActive = false;
    this->onFocusStolen();
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

    uint8_t leftClick = (this->bHandleLeftMouse && mouse->isLeftDown()) ? 0b10 : 0;
    uint8_t rightClick = (this->bHandleRightMouse && mouse->isRightDown()) ? 0b01 : 0;
    uint8_t buttonMask = (leftClick | rightClick);

    if(buttonMask && *propagate_clicks) {
        this->mouseUpCheck |= buttonMask;
        if(this->bMouseInside) {
            *propagate_clicks = !this->grabs_clicks;
        }

        // onMouseDownOutside
        if(!this->bMouseInside && !(this->mouseInsideCheck & buttonMask)) {
            this->mouseInsideCheck |= buttonMask;
            this->onMouseDownOutside(leftClick, rightClick);
        }

        // onMouseDownInside
        if(this->bMouseInside && !(this->mouseInsideCheck & buttonMask)) {
            this->bActive = true;
            this->mouseInsideCheck |= buttonMask;
            this->onMouseDownInside(leftClick, rightClick);
        }
    }

    // detect which buttons were released for mouse up events
    uint8_t releasedButtons = this->mouseUpCheck & (~buttonMask);
    if(releasedButtons && this->bActive) {
        if(this->bMouseInside)
            this->onMouseUpInside(releasedButtons & 0b10, releasedButtons & 0b01);
        else
            this->onMouseUpOutside(releasedButtons & 0b10, releasedButtons & 0b01);

        if(!this->bKeepActive) this->bActive = false;
    }

    // remove released buttons from mouseUpCheck
    this->mouseUpCheck &= buttonMask;

    // reset mouseInsideCheck if all buttons are released
    if(buttonMask == 0) {
        this->mouseInsideCheck = 0;
    }
}
