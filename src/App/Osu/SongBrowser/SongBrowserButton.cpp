// Copyright (c) 2016, PG, All rights reserved.
#include "SongBrowserButton.h"

#include <utility>

#include "SongBrowser.h"
#include "BeatmapCarousel.h"
// ---

#include "AnimationHandler.h"
#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"
#include "ConVar.h"
#include "Engine.h"
#include "Mouse.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "Skin.h"
#include "SoundEngine.h"

int SongBrowserButton::marginPixelsX = 9;
int SongBrowserButton::marginPixelsY = 9;
float SongBrowserButton::lastHoverSoundTime = 0;

// Color Button::inactiveDifficultyBackgroundColor = argb(255, 0, 150, 236); // blue

SongBrowserButton::SongBrowserButton(SongBrowser *songBrowser, BeatmapCarousel *view, UIContextMenu *contextMenu,
                                     float xPos, float yPos, float xSize, float ySize, UString name)
    : CBaseUIButton(xPos, yPos, xSize, ySize, std::move(name), ""), carousel(view) {
    this->songBrowser = songBrowser;
    this->contextMenu = contextMenu;

    this->font = osu->getSongBrowserFont();
    this->fontBold = osu->getSongBrowserFontBold();

    this->bVisible = false;
    this->bSelected = false;
    this->bHideIfSelected = false;

    this->bRightClick = false;
    this->bRightClickCheck = false;

    this->fTargetRelPosY = yPos;
    this->fScale = 1.0f;
    this->fOffsetPercent = 0.0f;
    this->fHoverOffsetAnimation = 0.0f;
    this->fCenterOffsetAnimation = 0.0f;
    this->fCenterOffsetVelocityAnimation = 0.0f;
    this->bIsSearchMatch = true;

    this->fHoverMoveAwayAnimation = 0.0f;
    this->moveAwayState = MOVE_AWAY_STATE::MOVE_CENTER;
}

SongBrowserButton::~SongBrowserButton() { this->deleteAnimations(); }

void SongBrowserButton::deleteAnimations() {
    anim->deleteExistingAnimation(&this->fCenterOffsetAnimation);
    anim->deleteExistingAnimation(&this->fCenterOffsetVelocityAnimation);
    anim->deleteExistingAnimation(&this->fHoverOffsetAnimation);
    anim->deleteExistingAnimation(&this->fHoverMoveAwayAnimation);
}

void SongBrowserButton::draw() {
    if(!this->bVisible) return;

    this->drawMenuButtonBackground();

    // debug inner bounding box
    if(cv::debug.getBool()) {
        // scaling
        const Vector2 pos = this->getActualPos();
        const Vector2 size = this->getActualSize();

        g->setColor(0xffff00ff);
        g->drawLine(pos.x, pos.y, pos.x + size.x, pos.y);
        g->drawLine(pos.x, pos.y, pos.x, pos.y + size.y);
        g->drawLine(pos.x, pos.y + size.y, pos.x + size.x, pos.y + size.y);
        g->drawLine(pos.x + size.x, pos.y, pos.x + size.x, pos.y + size.y);
    }

    // debug outer/actual bounding box
    if(cv::debug.getBool()) {
        g->setColor(0xffff0000);
        g->drawLine(this->vPos.x, this->vPos.y, this->vPos.x + this->vSize.x, this->vPos.y);
        g->drawLine(this->vPos.x, this->vPos.y, this->vPos.x, this->vPos.y + this->vSize.y);
        g->drawLine(this->vPos.x, this->vPos.y + this->vSize.y, this->vPos.x + this->vSize.x,
                    this->vPos.y + this->vSize.y);
        g->drawLine(this->vPos.x + this->vSize.x, this->vPos.y, this->vPos.x + this->vSize.x,
                    this->vPos.y + this->vSize.y);
    }
}

void SongBrowserButton::drawMenuButtonBackground() {
    g->setColor(this->bSelected ? this->getActiveBackgroundColor() : this->getInactiveBackgroundColor());
    g->pushTransform();
    {
        g->scale(this->fScale, this->fScale);
        g->translate(this->vPos.x + this->vSize.x / 2, this->vPos.y + this->vSize.y / 2);
        g->drawImage(osu->getSkin()->getMenuButtonBackground());
    }
    g->popTransform();
}

void SongBrowserButton::mouse_update(bool *propagate_clicks) {
    // Not correct, but clears most of the lag
    if(this->vPos.y + this->vSize.y < 0) return;
    if(this->vPos.y > osu->getScreenHeight()) return;

    // HACKHACK: absolutely disgusting
    // temporarily fool CBaseUIElement with modified position and size
    {
        Vector2 posBackup = this->vPos;
        Vector2 sizeBackup = this->vSize;

        this->vPos = this->getActualPos();
        this->vSize = this->getActualSize();
        {
            CBaseUIButton::mouse_update(propagate_clicks);
        }
        this->vPos = posBackup;
        this->vSize = sizeBackup;
    }

    if(!this->bVisible) return;

    // HACKHACK: this should really be part of the UI base
    // right click detection
    if(mouse->isRightDown()) {
        if(!this->bRightClickCheck) {
            this->bRightClickCheck = true;
            this->bRightClick = this->isMouseInside();
        }
    } else {
        if(this->bRightClick) {
            if(this->isMouseInside()) this->onRightMouseUpInside();
        }

        this->bRightClickCheck = false;
        this->bRightClick = false;
    }

    // animations need constant layout updates while visible
    this->updateLayoutEx();
}

void SongBrowserButton::updateLayoutEx() {
    const float uiScale = cv::ui_scale.getFloat();

    Image *menuButtonBackground = osu->getSkin()->getMenuButtonBackground();
    {
        const Vector2 minimumSize =
            Vector2(699.0f, 103.0f) * (osu->getSkin()->isMenuButtonBackground2x() ? 2.0f : 1.0f);
        const float minimumScale = Osu::getImageScaleToFitResolution(menuButtonBackground, minimumSize);
        this->fScale = Osu::getImageScale(menuButtonBackground->getSize() * minimumScale, 64.0f) * uiScale;
    }

    if(this->bVisible)  // lag prevention (animationHandler overflow)
    {
        const float centerOffsetAnimationTarget =
            1.0f - std::clamp<float>(std::abs((this->vPos.y + (this->vSize.y / 2) - this->carousel->getPos().y -
                                               this->carousel->getSize().y / 2) /
                                              (this->carousel->getSize().y / 2)),
                                     0.0f, 1.0f);
        anim->moveQuadOut(&this->fCenterOffsetAnimation, centerOffsetAnimationTarget, 0.5f, true);

        float centerOffsetVelocityAnimationTarget =
            std::clamp<float>((std::abs(this->carousel->getVelocity().y)) / 3500.0f, 0.0f, 1.0f);

        if(this->songBrowser->isRightClickScrolling()) centerOffsetVelocityAnimationTarget = 0.0f;

        if(this->carousel->isScrolling())
            anim->moveQuadOut(&this->fCenterOffsetVelocityAnimation, 0.0f, 1.0f, true);
        else
            anim->moveQuadOut(&this->fCenterOffsetVelocityAnimation, centerOffsetVelocityAnimationTarget, 1.25f, true);
    }

    this->setSize((int)(menuButtonBackground->getWidth() * this->fScale),
                  (int)(menuButtonBackground->getHeight() * this->fScale));

    const float percentCenterOffsetAnimation = 0.035f;
    const float percentVelocityOffsetAnimation = 0.35f;
    const float percentHoverOffsetAnimation = 0.075f;

    // this is the minimum offset necessary to not clip into the score scrollview (including all possible max animations
    // which can push us to the left, worst case)
    float minOffset = this->carousel->getSize().x * (percentCenterOffsetAnimation + percentHoverOffsetAnimation);
    {
        // also respect the width of the button image: push to the right until the edge of the button image can never be
        // visible even if all animations are fully active the 0.85f here heuristically pushes the buttons a bit further
        // to the right than would be necessary, to make animations work better on lower resolutions (would otherwise
        // hit the left edge too early)
        const float buttonWidthCompensation =
            std::max(this->carousel->getSize().x - this->getActualSize().x * 0.85f, 0.0f);
        minOffset += buttonWidthCompensation;
    }

    float offsetX =
        minOffset - this->carousel->getSize().x *
                        (percentCenterOffsetAnimation * this->fCenterOffsetAnimation *
                             (1.0f - this->fCenterOffsetVelocityAnimation) +
                         percentHoverOffsetAnimation * this->fHoverOffsetAnimation -
                         percentVelocityOffsetAnimation * this->fCenterOffsetVelocityAnimation + this->fOffsetPercent);
    offsetX = std::clamp<float>(
        offsetX, 0.0f,
        this->carousel->getSize().x -
            this->getActualSize().x * 0.15f);  // WARNING: hardcoded to match 0.85f above for buttonWidthCompensation

    this->setRelPosX(offsetX);
    this->setRelPosY(this->fTargetRelPosY + this->getSize().y * 0.125f * this->fHoverMoveAwayAnimation);
}

SongBrowserButton *SongBrowserButton::setVisible(bool visible) {
    CBaseUIButton::setVisible(visible);

    this->deleteAnimations();

    if(this->bVisible) {
        // scrolling pinch effect
        this->fCenterOffsetAnimation = 1.0f;
        this->fHoverOffsetAnimation = 0.0f;

        float centerOffsetVelocityAnimationTarget =
            std::clamp<float>((std::abs(this->carousel->getVelocity().y)) / 3500.0f, 0.0f, 1.0f);

        if(this->songBrowser->isRightClickScrolling()) centerOffsetVelocityAnimationTarget = 0.0f;

        this->fCenterOffsetVelocityAnimation = centerOffsetVelocityAnimationTarget;

        // force early layout update
        this->updateLayoutEx();
    }

    return this;
}

void SongBrowserButton::select(bool fireCallbacks, bool autoSelectBottomMostChild, bool wasParentSelected) {
    const bool wasSelected = this->bSelected;
    this->bSelected = true;

    // callback
    if(fireCallbacks) this->onSelected(wasSelected, autoSelectBottomMostChild, wasParentSelected);
}

void SongBrowserButton::deselect() { this->bSelected = false; }

void SongBrowserButton::resetAnimations() { this->setMoveAwayState(MOVE_AWAY_STATE::MOVE_CENTER, false); }

void SongBrowserButton::onClicked(bool left, bool right) {
    soundEngine->play(osu->getSkin()->selectDifficulty);

    CBaseUIButton::onClicked(left, right);

    this->select(true, true);
}

void SongBrowserButton::onMouseInside() {
    CBaseUIButton::onMouseInside();

    // hover sound
    if(engine->getTime() > lastHoverSoundTime + 0.05f)  // to avoid earraep
    {
        if(engine->hasFocus()) soundEngine->play(osu->getSkin()->getMenuHover());

        lastHoverSoundTime = engine->getTime();
    }

    // hover anim
    anim->moveQuadOut(&this->fHoverOffsetAnimation, 1.0f, 1.0f * (1.0f - this->fHoverOffsetAnimation), true);

    // move the rest of the buttons away from hovered-over one
    const std::vector<CBaseUIElement *> &elements = this->carousel->getContainer()->getElements();
    bool foundCenter = false;
    for(auto element : elements) {
        auto *b = dynamic_cast<SongBrowserButton *>(element);
        if(b != nullptr)  // sanity
        {
            if(b == this) {
                foundCenter = true;
                b->setMoveAwayState(MOVE_AWAY_STATE::MOVE_CENTER);
            } else
                b->setMoveAwayState(foundCenter ? MOVE_AWAY_STATE::MOVE_DOWN : MOVE_AWAY_STATE::MOVE_UP);
        }
    }
}

void SongBrowserButton::onMouseOutside() {
    CBaseUIButton::onMouseOutside();

    // reverse hover anim
    anim->moveQuadOut(&this->fHoverOffsetAnimation, 0.0f, 1.0f * this->fHoverOffsetAnimation, true);

    // only reset all other elements' state if we still should do so (possible frame delay of onMouseOutside coming
    // together with the next element already getting onMouseInside!)
    if(this->moveAwayState == MOVE_AWAY_STATE::MOVE_CENTER) {
        const std::vector<CBaseUIElement *> &elements = this->carousel->getContainer()->getElements();
        for(auto element : elements) {
            auto *b = dynamic_cast<SongBrowserButton *>(element);
            if(b != nullptr)  // sanity check
                b->setMoveAwayState(MOVE_AWAY_STATE::MOVE_CENTER);
        }
    }
}

void SongBrowserButton::setTargetRelPosY(float targetRelPosY) {
    this->fTargetRelPosY = targetRelPosY;
    this->setRelPosY(this->fTargetRelPosY);
}

Vector2 SongBrowserButton::getActualOffset() const {
    const float hd2xMultiplier = osu->getSkin()->isMenuButtonBackground2x() ? 2.0f : 1.0f;
    const float correctedMarginPixelsY =
        (2 * marginPixelsY + osu->getSkin()->getMenuButtonBackground()->getHeight() / hd2xMultiplier - 103.0f) / 2.0f;
    return Vector2((int)(marginPixelsX * this->fScale * hd2xMultiplier),
                   (int)(correctedMarginPixelsY * this->fScale * hd2xMultiplier));
}

void SongBrowserButton::setMoveAwayState(SongBrowserButton::MOVE_AWAY_STATE moveAwayState, bool animate) {
    this->moveAwayState = moveAwayState;

    // if we are not visible, destroy possibly existing animation
    if(!this->isVisible() || !animate) anim->deleteExistingAnimation(&this->fHoverMoveAwayAnimation);

    // only submit a new animation if we are visible, otherwise we would overwhelm the animationhandler with a shitload
    // of requests every time for every button (if we are not visible then we can just directly set the new value)
    switch(this->moveAwayState) {
        case MOVE_AWAY_STATE::MOVE_CENTER: {
            if(!this->isVisible() || !animate)
                this->fHoverMoveAwayAnimation = 0.0f;
            else
                anim->moveQuartOut(&this->fHoverMoveAwayAnimation, 0, 0.7f, this->isMouseInside() ? 0.0f : 0.05f,
                                   true);  // add a tiny bit of delay to avoid jerky movement if the cursor is briefly
                                           // between songbuttons while moving
        } break;

        case MOVE_AWAY_STATE::MOVE_UP: {
            if(!this->isVisible() || !animate)
                this->fHoverMoveAwayAnimation = -1.0f;
            else
                anim->moveQuartOut(&this->fHoverMoveAwayAnimation, -1.0f, 0.7f, true);
        } break;

        case MOVE_AWAY_STATE::MOVE_DOWN: {
            if(!this->isVisible() || !animate)
                this->fHoverMoveAwayAnimation = 1.0f;
            else
                anim->moveQuartOut(&this->fHoverMoveAwayAnimation, 1.0f, 0.7f, true);
        } break;
    }
}

Color SongBrowserButton::getActiveBackgroundColor() const {
    return argb(std::clamp<int>(cv::songbrowser_button_active_color_a.getInt(), 0, 255),
                std::clamp<int>(cv::songbrowser_button_active_color_r.getInt(), 0, 255),
                std::clamp<int>(cv::songbrowser_button_active_color_g.getInt(), 0, 255),
                std::clamp<int>(cv::songbrowser_button_active_color_b.getInt(), 0, 255));
}

Color SongBrowserButton::getInactiveBackgroundColor() const {
    return argb(std::clamp<int>(cv::songbrowser_button_inactive_color_a.getInt(), 0, 255),
                std::clamp<int>(cv::songbrowser_button_inactive_color_r.getInt(), 0, 255),
                std::clamp<int>(cv::songbrowser_button_inactive_color_g.getInt(), 0, 255),
                std::clamp<int>(cv::songbrowser_button_inactive_color_b.getInt(), 0, 255));
}
