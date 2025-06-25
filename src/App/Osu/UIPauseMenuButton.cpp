#include "UIPauseMenuButton.h"

#include "AnimationHandler.h"
#include "Engine.h"
#include "Osu.h"
#include "Skin.h"
#include "SoundEngine.h"

using namespace std;

UIPauseMenuButton::UIPauseMenuButton(std::function<Image *()> getImageFunc, float xPos, float yPos, float xSize,
                                     float ySize, UString name)
    : CBaseUIButton(xPos, yPos, xSize, ySize, name) {
    this->getImageFunc = getImageFunc;

    this->vScale = Vector2(1, 1);
    this->fScaleMultiplier = 1.1f;

    this->fAlpha = 1.0f;
}

void UIPauseMenuButton::draw() {
    if(!this->bVisible) return;

    // draw image
    Image *image = this->getImageFunc();
    if(image != NULL) {
        g->setColor(COLORf(this->fAlpha, 1.0f, 1.0f, 1.0f));
        g->pushTransform();
        {
            // scale
            g->scale(this->vScale.x, this->vScale.y);

            // center and draw
            g->translate(this->vPos.x + (int)(this->vSize.x / 2), this->vPos.y + (int)(this->vSize.y / 2));
            g->drawImage(image);
        }
        g->popTransform();
    }
}

void UIPauseMenuButton::setBaseScale(float xScale, float yScale) {
    this->vBaseScale.x = xScale;
    this->vBaseScale.y = yScale;

    this->vScale = this->vBaseScale;
}

void UIPauseMenuButton::onMouseInside() {
    CBaseUIButton::onMouseInside();

    if(engine->hasFocus()) soundEngine->play(osu->getSkin()->getMenuHover());

    const float animationDuration = 0.09f;
    anim->moveLinear(&this->vScale.x, this->vBaseScale.x * this->fScaleMultiplier, animationDuration, true);
    anim->moveLinear(&this->vScale.y, this->vBaseScale.y * this->fScaleMultiplier, animationDuration, true);

    if(this->getName() == UString("Resume")) {
        soundEngine->play(osu->getSkin()->hoverPauseContinue);
    } else if(this->getName() == UString("Retry")) {
        soundEngine->play(osu->getSkin()->hoverPauseRetry);
    } else if(this->getName() == UString("Quit")) {
        soundEngine->play(osu->getSkin()->hoverPauseBack);
    }
}

void UIPauseMenuButton::onMouseOutside() {
    CBaseUIButton::onMouseOutside();

    const float animationDuration = 0.09f;
    anim->moveLinear(&this->vScale.x, this->vBaseScale.x, animationDuration, true);
    anim->moveLinear(&this->vScale.y, this->vBaseScale.y, animationDuration, true);
}
