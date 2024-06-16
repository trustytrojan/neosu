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
    m_fAnimation = 0.0f;
    m_fImageScale = 1.0f;
}

void UIBackButton::draw(Graphics *g) {
    if(!m_bVisible) return;

    const float scaleAnimMultiplier = 0.01f;

    // draw button image
    g->pushTransform();
    {
        g->translate(m_vSize.x / 2, -m_vSize.y / 2);
        g->scale((1.0f + m_fAnimation * scaleAnimMultiplier), (1.0f + m_fAnimation * scaleAnimMultiplier));
        g->translate(-m_vSize.x / 2, m_vSize.y / 2);
        g->setColor(0xffffffff);
        osu->getSkin()->getMenuBack2()->draw(
            g, m_vPos + (osu->getSkin()->getMenuBack2()->getSize() / 2) * m_fImageScale, m_fImageScale);
    }
    g->popTransform();

    // draw anim highlight overlay
    if(m_fAnimation > 0.0f) {
        g->pushTransform();
        {
            g->setColor(0xffffffff);
            g->setAlpha(m_fAnimation * 0.15f);
            g->translate(m_vSize.x / 2, -m_vSize.y / 2);
            g->scale(1.0f + m_fAnimation * scaleAnimMultiplier, 1.0f + m_fAnimation * scaleAnimMultiplier);
            g->translate(-m_vSize.x / 2, m_vSize.y / 2);
            g->translate(m_vPos.x + m_vSize.x / 2, m_vPos.y + m_vSize.y / 2);
            g->fillRect(-m_vSize.x / 2, -m_vSize.y / 2, m_vSize.x, m_vSize.y + 5);
        }
        g->popTransform();
    }
}

void UIBackButton::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible) return;
    CBaseUIButton::mouse_update(propagate_clicks);
}

void UIBackButton::onMouseDownInside() {
    CBaseUIButton::onMouseDownInside();

    engine->getSound()->play(osu->getSkin()->m_backButtonClick);
}

void UIBackButton::onMouseInside() {
    CBaseUIButton::onMouseInside();

    anim->moveQuadOut(&m_fAnimation, 1.0f, 0.1f, 0.0f, true);
    engine->getSound()->play(osu->getSkin()->m_backButtonHover);
}

void UIBackButton::onMouseOutside() {
    CBaseUIButton::onMouseOutside();

    anim->moveQuadOut(&m_fAnimation, 0.0f, m_fAnimation * 0.1f, 0.0f, true);
}

void UIBackButton::updateLayout() {
    const float uiScale = Osu::ui_scale->getFloat();

    Vector2 newSize = osu->getSkin()->getMenuBack2()->getSize();
    newSize.y = clamp<float>(newSize.y, 0,
                             osu->getSkin()->getMenuBack2()->getSizeBase().y *
                                 1.5f);  // clamp the height down if it exceeds 1.5x the base height
    newSize *= uiScale;
    m_fImageScale = (newSize.y / osu->getSkin()->getMenuBack2()->getSize().y);
    setSize(newSize);
}

void UIBackButton::resetAnimation() {
    anim->deleteExistingAnimation(&m_fAnimation);
    m_fAnimation = 0.0f;
}
