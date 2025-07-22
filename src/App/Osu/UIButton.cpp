#include "UIButton.h"

#include <utility>

#include "AnimationHandler.h"
#include "Engine.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "Skin.h"
#include "SoundEngine.h"
#include "TooltipOverlay.h"



static float button_sound_cooldown = 0.f;

UIButton::UIButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text)
    : CBaseUIButton(xPos, yPos, xSize, ySize, std::move(name), std::move(text)) {
    this->bDefaultSkin = false;
    this->color = 0xffffffff;
    this->backupColor = this->color;
    this->fBrightness = 0.85f;
    this->fAnim = 0.0f;
    this->fAlphaAddOnHover = 0.0f;

    this->bFocusStolenDelay = false;
}

void UIButton::draw() {
    if(!this->bVisible || !this->bVisible2) return;

    Image *buttonLeft = this->bDefaultSkin ? osu->getSkin()->getDefaultButtonLeft() : osu->getSkin()->getButtonLeft();
    Image *buttonMiddle =
        this->bDefaultSkin ? osu->getSkin()->getDefaultButtonMiddle() : osu->getSkin()->getButtonMiddle();
    Image *buttonRight =
        this->bDefaultSkin ? osu->getSkin()->getDefaultButtonRight() : osu->getSkin()->getButtonRight();

    float leftScale = osu->getImageScaleToFitResolution(buttonLeft, this->vSize);
    float leftWidth = buttonLeft->getWidth() * leftScale;

    float rightScale = osu->getImageScaleToFitResolution(buttonRight, this->vSize);
    float rightWidth = buttonRight->getWidth() * rightScale;

    float middleWidth = this->vSize.x - leftWidth - rightWidth;

    auto color = this->is_loading ? rgba(0x33, 0x33, 0x33, 0xff) : this->color;
    const auto red =
        static_cast<Channel>(std::max(static_cast<float>(color.R()) * this->fBrightness, this->fAnim * 255.0f));
    const auto green =
        static_cast<Channel>(std::max(static_cast<float>(color.G()) * this->fBrightness, this->fAnim * 255.0f));
    const auto blue =
        static_cast<Channel>(std::max(static_cast<float>(color.B()) * this->fBrightness, this->fAnim * 255.0f));
    g->setColor(
        argb(std::clamp<int>(color.A() + (isMouseInside() ? (int)(this->fAlphaAddOnHover * 255.0f) : 0), 0, 255), red,
             green, blue));

    buttonLeft->bind();
    {
        g->drawQuad((int)this->vPos.x, (int)this->vPos.y, (int)leftWidth, (int)this->vSize.y);
    }
    buttonLeft->unbind();

    buttonMiddle->bind();
    {
        g->drawQuad((int)this->vPos.x + (int)leftWidth, (int)this->vPos.y, (int)middleWidth, (int)this->vSize.y);
    }
    buttonMiddle->unbind();

    buttonRight->bind();
    {
        g->drawQuad((int)this->vPos.x + (int)leftWidth + (int)middleWidth, (int)this->vPos.y, (int)rightWidth,
                    (int)this->vSize.y);
    }
    buttonRight->unbind();

    if(this->is_loading) {
        const float scale = (this->vSize.y * 0.8) / osu->getSkin()->getLoadingSpinner()->getSize().y;
        g->setColor(0xffffffff);
        g->pushTransform();
        g->rotate(engine->getTime() * 180, 0, 0, 1);
        g->scale(scale, scale);
        g->translate(this->vPos.x + this->vSize.x / 2.0f, this->vPos.y + this->vSize.y / 2.0f);
        g->drawImage(osu->getSkin()->getLoadingSpinner());
        g->popTransform();
    } else {
        this->drawText();
    }
}

void UIButton::mouse_update(bool *propagate_clicks) {
    if(!this->bVisible || !this->bVisible2) return;
    CBaseUIButton::mouse_update(propagate_clicks);

    if(this->isMouseInside() && this->tooltipTextLines.size() > 0 && !this->bFocusStolenDelay) {
        osu->getTooltipOverlay()->begin();
        {
            for(int i = 0; i < this->tooltipTextLines.size(); i++) {
                osu->getTooltipOverlay()->addLine(this->tooltipTextLines[i]);
            }
        }
        osu->getTooltipOverlay()->end();
    }

    this->bFocusStolenDelay = false;
}

void UIButton::onMouseInside() {
    this->fBrightness = 1.0f;

    if(button_sound_cooldown + 0.05f < engine->getTime()) {
        soundEngine->play(osu->getSkin()->hoverButton);
        button_sound_cooldown = engine->getTime();
    }
}

void UIButton::onMouseOutside() { this->fBrightness = 0.85f; }

void UIButton::onClicked(bool left, bool right) {
    CBaseUIButton::onClicked(left, right);

    if(this->is_loading) return;

    this->animateClickColor();

    soundEngine->play(osu->getSkin()->clickButton);
}

void UIButton::onFocusStolen() {
    CBaseUIButton::onFocusStolen();

    this->bMouseInside = false;
    this->bFocusStolenDelay = true;
}

void UIButton::animateClickColor() {
    this->fAnim = 1.0f;
    anim->moveLinear(&this->fAnim, 0.0f, 0.5f, true);
}

void UIButton::setTooltipText(const UString& text) { this->tooltipTextLines = text.split("\n"); }
