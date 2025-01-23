//================ Copyright (c) 2018, PG, All rights reserved. =================//
//
// Purpose:		slider used for the volume overlay HUD
//
// $NoKeywords: $osuvolsl
//===============================================================================//

#include "UIVolumeSlider.h"

#include "AnimationHandler.h"
#include "Engine.h"
#include "Osu.h"
#include "ResourceManager.h"

UIVolumeSlider::UIVolumeSlider(float xPos, float yPos, float xSize, float ySize, UString name)
    : CBaseUISlider(xPos, yPos, xSize, ySize, name) {
    this->type = TYPE::MASTER;
    this->bSelected = false;

    this->bWentMouseInside = false;
    this->fSelectionAnim = 0.0f;

    this->font = engine->getResourceManager()->getFont("FONT_DEFAULT");

    engine->getResourceManager()->loadImage("ic_volume_mute_white_48dp.png", "OSU_UI_VOLUME_SLIDER_BLOCK_0");
    engine->getResourceManager()->loadImage("ic_volume_up_white_48dp.png", "OSU_UI_VOLUME_SLIDER_BLOCK_1");
    engine->getResourceManager()->loadImage("ic_music_off_48dp.png", "OSU_UI_VOLUME_SLIDER_MUSIC_0");
    engine->getResourceManager()->loadImage("ic_music_48dp.png", "OSU_UI_VOLUME_SLIDER_MUSIC_1");
    engine->getResourceManager()->loadImage("ic_effects_off_48dp.png", "OSU_UI_VOLUME_SLIDER_EFFECTS_0");
    engine->getResourceManager()->loadImage("ic_effects_48dp.png", "OSU_UI_VOLUME_SLIDER_EFFECTS_1");

    this->setFrameColor(0xff7f7f7f);
}

void UIVolumeSlider::drawBlock(Graphics *g) {
    // draw icon
    Image *img = NULL;
    if(this->getFloat() < 0.01f)
        img = engine->getResourceManager()->getImage(
            this->type == TYPE::MASTER
                ? "OSU_UI_VOLUME_SLIDER_BLOCK_0"
                : (this->type == TYPE::MUSIC ? "OSU_UI_VOLUME_SLIDER_MUSIC_0" : "OSU_UI_VOLUME_SLIDER_EFFECTS_0"));
    else
        img = engine->getResourceManager()->getImage(
            this->type == TYPE::MASTER
                ? "OSU_UI_VOLUME_SLIDER_BLOCK_1"
                : (this->type == TYPE::MUSIC ? "OSU_UI_VOLUME_SLIDER_MUSIC_1" : "OSU_UI_VOLUME_SLIDER_EFFECTS_1"));

    g->pushTransform();
    {
        const float scaleMultiplier = 0.95f + 0.2f * this->fSelectionAnim;

        g->scale((this->vBlockSize.y / img->getSize().x) * scaleMultiplier,
                 (this->vBlockSize.y / img->getSize().y) * scaleMultiplier);
        g->translate(this->vPos.x + this->vBlockPos.x + this->vBlockSize.x / 2.0f,
                     this->vPos.y + this->vBlockPos.y + this->vBlockSize.y / 2.0f + 1);
        g->setColor(0xffffffff);
        g->drawImage(img);
    }
    g->popTransform();

    // draw percentage
    g->pushTransform();
    {
        g->translate((int)(this->vPos.x + this->vSize.x + this->vSize.x * 0.0335f),
                     (int)(this->vPos.y + this->vSize.y / 2 + this->font->getHeight() / 2));
        g->setColor(0xff000000);
        g->translate(1, 1);
        g->drawString(this->font, UString::format("%i%%", (int)(std::round(this->getFloat() * 100.0f))));
        g->setColor(0xffffffff);
        g->translate(-1, -1);
        g->drawString(this->font, UString::format("%i%%", (int)(std::round(this->getFloat() * 100.0f))));
    }
    g->popTransform();
}

void UIVolumeSlider::onMouseInside() {
    CBaseUISlider::onMouseInside();

    this->bWentMouseInside = true;
}

void UIVolumeSlider::setSelected(bool selected) {
    this->bSelected = selected;

    if(this->bSelected) {
        this->setFrameColor(0xffffffff);
        anim->moveQuadOut(&this->fSelectionAnim, 1.0f, 0.1f, true);
        anim->moveQuadIn(&this->fSelectionAnim, 0.0f, 0.15f, 0.1f, false);
    } else {
        this->setFrameColor(0xff7f7f7f);
        anim->moveLinear(&this->fSelectionAnim, 0.0f, 0.15f * this->fSelectionAnim, true);
    }
}

bool UIVolumeSlider::checkWentMouseInside() {
    const bool temp = this->bWentMouseInside;
    this->bWentMouseInside = false;
    return temp;
}

float UIVolumeSlider::getMinimumExtraTextWidth() {
    return this->vSize.x * 0.0335f * 2.0f + this->font->getStringWidth("100%");
}
