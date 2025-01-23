#include "UIBackButton.h"

#include "AnimationHandler.h"
#include "ConVar.h"
#include "Engine.h"
#include "Osu.h"
#include "Skin.h"
#include "SkinImage.h"
#include "SoundEngine.h"

using namespace std;

UIBackButton::UIBackButton(float xPos, float yPos, float xSize, float ySize, UString name)
    : CBaseUIButton(xPos, yPos, xSize, ySize, name, "") {
    this->fAnimation = 0.0f;
    this->fImageScale = 1.0f;
}

void UIBackButton::draw(Graphics *g) {
    if(!this->bVisible) return;

    const float scaleAnimMultiplier = 0.01f;

    // draw button image
    g->pushTransform();
    {
        g->translate(this->vSize.x / 2, -this->vSize.y / 2);
        g->scale((1.0f + this->fAnimation * scaleAnimMultiplier), (1.0f + this->fAnimation * scaleAnimMultiplier));
        g->translate(-this->vSize.x / 2, this->vSize.y / 2);
        g->setColor(0xffffffff);
        osu->getSkin()->getMenuBack2()->draw(
            g, this->vPos + (osu->getSkin()->getMenuBack2()->getSize() / 2) * this->fImageScale,
            this->fImageScale);
    }
    g->popTransform();

    // draw anim highlight overlay
    if(this->fAnimation > 0.0f) {
        g->pushTransform();
        {
            g->setColor(0xffffffff);
            g->setAlpha(this->fAnimation * 0.15f);
            g->translate(this->vSize.x / 2, -this->vSize.y / 2);
            g->scale(1.0f + this->fAnimation * scaleAnimMultiplier, 1.0f + this->fAnimation * scaleAnimMultiplier);
            g->translate(-this->vSize.x / 2, this->vSize.y / 2);
            g->translate(this->vPos.x + this->vSize.x / 2, this->vPos.y + this->vSize.y / 2);
            g->fillRect(-this->vSize.x / 2, -this->vSize.y / 2, this->vSize.x, this->vSize.y + 5);
        }
        g->popTransform();
    }
}

void UIBackButton::mouse_update(bool *propagate_clicks) {
    if(!this->bVisible) return;
    CBaseUIButton::mouse_update(propagate_clicks);
}

void UIBackButton::onMouseDownInside() {
    CBaseUIButton::onMouseDownInside();

    engine->getSound()->play(osu->getSkin()->backButtonClick);
}

void UIBackButton::onMouseInside() {
    CBaseUIButton::onMouseInside();

    anim->moveQuadOut(&this->fAnimation, 1.0f, 0.1f, 0.0f, true);
    engine->getSound()->play(osu->getSkin()->backButtonHover);
}

void UIBackButton::onMouseOutside() {
    CBaseUIButton::onMouseOutside();

    anim->moveQuadOut(&this->fAnimation, 0.0f, this->fAnimation * 0.1f, 0.0f, true);
}

void UIBackButton::updateLayout() {
    const float uiScale = cv_ui_scale.getFloat();

    Vector2 newSize = osu->getSkin()->getMenuBack2()->getSize();
    newSize.y = clamp<float>(newSize.y, 0,
                             osu->getSkin()->getMenuBack2()->getSizeBase().y *
                                 1.5f);  // clamp the height down if it exceeds 1.5x the base height
    newSize *= uiScale;
    this->fImageScale = (newSize.y / osu->getSkin()->getMenuBack2()->getSize().y);
    this->setSize(newSize);
}

void UIBackButton::resetAnimation() {
    anim->deleteExistingAnimation(&this->fAnimation);
    this->fAnimation = 0.0f;
}
