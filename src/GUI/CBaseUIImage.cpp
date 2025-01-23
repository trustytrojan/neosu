//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		a simple image class
//
// $NoKeywords: $
//===============================================================================//

#include "CBaseUIImage.h"

#include "Engine.h"
#include "ResourceManager.h"

CBaseUIImage::CBaseUIImage(std::string imageResourceName, float xPos, float yPos, float xSize, float ySize,
                           UString name)
    : CBaseUIElement(xPos, yPos, xSize, ySize, name) {
    this->bScaleToFit = true;  // must be up here because it's used in setImage()
    this->setImage(engine->getResourceManager()->getImage(imageResourceName));

    this->fRot = 0.0f;
    this->vScale.x = 1.0f;
    this->vScale.y = 1.0f;

    // if our image is null, autosize to the element size
    if(this->image == NULL) {
        this->vSize.x = xSize;
        this->vSize.y = ySize;
    }

    this->frameColor = COLOR(255, 255, 255, 255);
    this->backgroundColor = COLOR(255, 0, 0, 0);
    this->color = 0xffffffff;

    this->bDrawFrame = false;
    this->bDrawBackground = false;
}

void CBaseUIImage::draw(Graphics *g) {
    if(!this->bVisible) return;

    // draw background
    if(this->bDrawBackground) {
        g->setColor(this->backgroundColor);
        g->fillRect(this->vPos.x + 1, this->vPos.y + 1, this->vSize.x - 1, this->vSize.y - 1);
    }

    // draw image
    if(this->image != NULL) {
        g->setColor(this->color);
        g->pushTransform();

        // scale
        if(this->bScaleToFit) g->scale(this->vScale.x, this->vScale.y);

        // rotate
        if(this->fRot != 0.0f) g->rotate(this->fRot);

        // center and draw
        g->translate(this->vPos.x + (this->vSize.x / 2) + (!this->bScaleToFit ? 1 : 0),
                     this->vPos.y + (this->vSize.y / 2) + (!this->bScaleToFit ? 1 : 0));
        g->drawImage(this->image);

        g->popTransform();
    }

    // draw frame
    if(this->bDrawFrame) {
        g->setColor(this->frameColor);
        g->drawRect(this->vPos.x, this->vPos.y, this->vSize.x, this->vSize.y);
    }
}

void CBaseUIImage::setImage(Image *img) {
    this->image = img;

    if(this->image != NULL) {
        if(this->bScaleToFit) {
            this->vSize.x = this->image->getWidth();
            this->vSize.y = this->image->getHeight();
        }

        this->vScale.x = this->vSize.x / (float)this->image->getWidth();
        this->vScale.y = this->vSize.y / (float)this->image->getHeight();
    } else {
        this->vScale.x = 1;
        this->vScale.y = 1;
    }
}
