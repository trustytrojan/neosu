#include "CBaseUIElement.h"

#include "Engine.h"
#include "Mouse.h"
#include "Osu.h"
#include "TooltipOverlay.h"

bool CBaseUIElement::isVisibleOnScreen(const McRect &rect) {
    return engine->getScreenRect().intersects(rect);
}

void CBaseUIElement::stealFocus() {
    this->mouseInsideCheck = static_cast<size_t>(this->bHandleLeftMouse | (this->bHandleRightMouse << 1));
    this->bActive = false;
    this->onFocusStolen();
}

void CBaseUIElement::mouse_update(bool *propagate_clicks) {
    // check if mouse is inside element
    if(this->getRect().contains(mouse->getPos())) {
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

    const std::bitset<2> buttonMask{static_cast<size_t>((this->bHandleLeftMouse && mouse->isLeftDown()) |
                                                        ((this->bHandleRightMouse && mouse->isRightDown()) << 1))};

    if(buttonMask.any() && *propagate_clicks) {
        this->mouseUpCheck |= buttonMask;
        if(this->bMouseInside) {
            *propagate_clicks = !this->grabs_clicks;
        }

        // onMouseDownOutside
        if(!this->bMouseInside && (this->mouseInsideCheck & buttonMask).none()) {
            this->mouseInsideCheck |= buttonMask;
            this->onMouseDownOutside(buttonMask[0], buttonMask[1]);
        }

        // onMouseDownInside
        if(this->bMouseInside && (this->mouseInsideCheck & buttonMask).none()) {
            this->bActive = true;
            this->mouseInsideCheck |= buttonMask;
            this->onMouseDownInside(buttonMask[0], buttonMask[1]);
        }
    }

    // detect which buttons were released for mouse up events
    const std::bitset<2> releasedButtons{this->mouseUpCheck & (~buttonMask)};
    if(releasedButtons.any() && this->bActive) {
        if(this->bMouseInside)
            this->onMouseUpInside(releasedButtons[0], releasedButtons[1]);
        else
            this->onMouseUpOutside(releasedButtons[0], releasedButtons[1]);

        if(!this->bKeepActive) this->bActive = false;
    }

    // remove released buttons from mouseUpCheck
    this->mouseUpCheck &= buttonMask;

    // reset mouseInsideCheck if all buttons are released
    if(buttonMask.none()) {
        this->mouseInsideCheck = 0b00;
    }
}
