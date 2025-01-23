#include "UISlider.h"

#include "AnimationHandler.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "Skin.h"

UISlider::UISlider(float xPos, float yPos, float xSize, float ySize, UString name)
    : CBaseUISlider(xPos, yPos, xSize, ySize, name) {
    this->setBlockSize(20, 20);
}

void UISlider::draw(Graphics *g) {
    if(!this->bVisible) return;

    Image *img = osu->getSkin()->getCircleEmpty();
    if(img == NULL) {
        CBaseUISlider::draw(g);
        return;
    }

    int lineAdd = 1;

    float line1Start = this->vPos.x + (this->vBlockSize.x - 1) / 2 + 1;
    float line1End = this->vPos.x + this->vBlockPos.x + lineAdd;
    float line2Start = this->vPos.x + this->vBlockPos.x + this->vBlockSize.x - lineAdd;
    float line2End = this->vPos.x + this->vSize.x - (this->vBlockSize.x - 1) / 2;

    // draw sliding line
    g->setColor(this->frameColor);
    if(line1End > line1Start)
        g->drawLine((int)(line1Start), (int)(this->vPos.y + this->vSize.y / 2.0f + 1), (int)(line1End),
                    (int)(this->vPos.y + this->vSize.y / 2.0f + 1));
    if(line2End > line2Start)
        g->drawLine((int)(line2Start), (int)(this->vPos.y + this->vSize.y / 2.0f + 1), (int)(line2End),
                    (int)(this->vPos.y + this->vSize.y / 2.0f + 1));

    // draw sliding block
    Vector2 blockCenter = this->vPos + this->vBlockPos + this->vBlockSize / 2;
    Vector2 scale = Vector2(this->vBlockSize.x / img->getWidth(), this->vBlockSize.y / img->getHeight());

    g->setColor(this->frameColor);
    g->pushTransform();
    {
        g->scale(scale.x, scale.y);
        g->translate(blockCenter.x, blockCenter.y + 1);
        g->drawImage(osu->getSkin()->getCircleEmpty());
    }
    g->popTransform();
}
