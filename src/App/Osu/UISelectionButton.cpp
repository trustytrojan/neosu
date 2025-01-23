//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		song browser selection button (mode, mods, random, etc)
//
// $NoKeywords: $
//===============================================================================//

#include "UISelectionButton.h"

#include "AnimationHandler.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "SkinImage.h"

UISelectionButton::UISelectionButton(std::function<SkinImage *()> getImageFunc,
                                     std::function<SkinImage *()> getImageOverFunc, float xPos, float yPos, float xSize,
                                     float ySize, UString name)
    : CBaseUIButton(xPos, yPos, xSize, ySize, name) {
    this->getImageFunc = getImageFunc;
    this->getImageOverFunc = getImageOverFunc;

    this->fAnimation = 0.0f;
}

void UISelectionButton::draw(Graphics *g) {
    if(!this->bVisible) return;

    // draw image
    SkinImage *image = this->getImageFunc();
    if(image != NULL) {
        const Vector2 imageSize = image->getImageSizeForCurrentFrame();

        const float scale =
            (this->vSize.x / imageSize.x < this->vSize.y / imageSize.y ? this->vSize.x / imageSize.x
                                                                           : this->vSize.y / imageSize.y);

        g->setColor(0xffffffff);
        image->drawRaw(
            g, Vector2(this->vPos.x + (int)(this->vSize.x / 2), this->vPos.y + (int)(this->vSize.y / 2)),
            scale);
    }

    // draw over image
    if(this->fAnimation > 0.0f) {
        SkinImage *overImage = this->getImageOverFunc();
        if(overImage != NULL) {
            const Vector2 imageSize = overImage->getImageSizeForCurrentFrame();

            const float scale =
                (this->vSize.x / imageSize.x < this->vSize.y / imageSize.y ? this->vSize.x / imageSize.x
                                                                               : this->vSize.y / imageSize.y);

            g->setColor(0xffffffff);
            g->setAlpha(this->fAnimation);

            overImage->drawRaw(
                g, Vector2(this->vPos.x + (int)(this->vSize.x / 2), this->vPos.y + (int)(this->vSize.y / 2)),
                scale);
        }
    }

    CBaseUIButton::drawText(g);
}

void UISelectionButton::onMouseInside() {
    CBaseUIButton::onMouseInside();

    anim->moveLinear(&this->fAnimation, 1.0f, 0.1f, true);
}

void UISelectionButton::onMouseOutside() {
    CBaseUIButton::onMouseOutside();

    anim->moveLinear(&this->fAnimation, 0.0f, this->fAnimation * 0.1f, true);
}

void UISelectionButton::onResized() {
    CBaseUIButton::onResized();

    // NOTE: we get the height set to the current bottombarheight, so use that with the aspect ratio to get the correct
    // relative "hardcoded" width

    SkinImage *image = this->getImageFunc();
    if(image != NULL) {
        float aspectRatio = image->getSizeBaseRaw().x / image->getSizeBaseRaw().y;
        aspectRatio += 0.025f;  // very slightly overscale to make most skins fit better with the bottombar blue line
        this->vSize.x = aspectRatio * this->vSize.y;
    }
}

void UISelectionButton::keyboardPulse() {
    if(this->isMouseInside()) return;

    this->fAnimation = 1.0f;
    anim->moveLinear(&this->fAnimation, 0.0f, 0.1f, 0.05f, true);
}
