//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		a simple image button
//
// $NoKeywords: $
//===============================================================================//

#include "CBaseUIImageButton.h"

#include "Engine.h"
#include "ResourceManager.h"

CBaseUIImageButton::CBaseUIImageButton(std::string imageResourceName, float xPos, float yPos, float xSize, float ySize,
                                       UString name)
    : CBaseUIButton(xPos, yPos, xSize, ySize, name, "") {
    this->setImageResourceName(imageResourceName);

    this->fRot = 0.0f;
    this->vScale = Vector2(1, 1);
    this->bScaleToFit = false;
    this->bKeepAspectRatio = true;
}

void CBaseUIImageButton::draw(Graphics *g) {
    if(!this->bVisible) return;

    // draw image
    Image *image = engine->getResourceManager()->getImage(this->sImageResourceName);
    if(image != NULL) {
        g->setColor(0xffffffff);
        g->pushTransform();

        // scale
        g->scale(this->vScale.x, this->vScale.y);

        // rotate
        if(this->fRot != 0.0f) g->rotate(this->fRot);

        // center and draw
        g->translate(this->vPos.x + (int)(this->vSize.x / 2), this->vPos.y + (int)(this->vSize.y / 2));
        g->drawImage(image);

        g->popTransform();
    }
}

CBaseUIImageButton *CBaseUIImageButton::setImageResourceName(std::string imageResourceName) {
    this->sImageResourceName = imageResourceName;

    Image *image = engine->getResourceManager()->getImage(this->sImageResourceName);
    if(image != NULL) this->setSize(Vector2(image->getWidth(), image->getHeight()));

    return this;
}

void CBaseUIImageButton::onResized() {
    CBaseUIButton::onResized();

    Image *image = engine->getResourceManager()->getImage(this->sImageResourceName);
    if(this->bScaleToFit && image != NULL) {
        if(!this->bKeepAspectRatio) {
            this->vScale = Vector2(this->vSize.x / image->getWidth(), this->vSize.y / image->getHeight());
            this->vSize.x = (int)(image->getWidth() * this->vScale.x);
            this->vSize.y = (int)(image->getHeight() * this->vScale.y);
        } else {
            float scaleFactor = this->vSize.x / image->getWidth() < this->vSize.y / image->getHeight()
                                    ? this->vSize.x / image->getWidth()
                                    : this->vSize.y / image->getHeight();
            this->vScale = Vector2(scaleFactor, scaleFactor);
            this->vSize.x = (int)(image->getWidth() * this->vScale.x);
            this->vSize.y = (int)(image->getHeight() * this->vScale.y);
        }
    }
}
