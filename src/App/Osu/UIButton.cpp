#include "UIButton.h"

#include "AnimationHandler.h"
#include "Engine.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "Skin.h"
#include "SoundEngine.h"
#include "TooltipOverlay.h"

using namespace std;

static float button_sound_cooldown = 0.f;

UIButton::UIButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text)
    : CBaseUIButton(xPos, yPos, xSize, ySize, name, text) {
    m_bDefaultSkin = false;
    m_color = 0xffffffff;
    m_backupColor = m_color;
    m_fBrightness = 0.85f;
    m_fAnim = 0.0f;
    m_fAlphaAddOnHover = 0.0f;

    m_bFocusStolenDelay = false;
}

void UIButton::draw(Graphics *g) {
    if(!m_bVisible) return;

    Image *buttonLeft = m_bDefaultSkin ? osu->getSkin()->getDefaultButtonLeft() : osu->getSkin()->getButtonLeft();
    Image *buttonMiddle = m_bDefaultSkin ? osu->getSkin()->getDefaultButtonMiddle() : osu->getSkin()->getButtonMiddle();
    Image *buttonRight = m_bDefaultSkin ? osu->getSkin()->getDefaultButtonRight() : osu->getSkin()->getButtonRight();

    float leftScale = osu->getImageScaleToFitResolution(buttonLeft, m_vSize);
    float leftWidth = buttonLeft->getWidth() * leftScale;

    float rightScale = osu->getImageScaleToFitResolution(buttonRight, m_vSize);
    float rightWidth = buttonRight->getWidth() * rightScale;

    float middleWidth = m_vSize.x - leftWidth - rightWidth;

    auto color = is_loading ? 0xff333333 : m_color;
    char red = max((unsigned int)(COLOR_GET_Ri(color) * m_fBrightness), (unsigned int)(m_fAnim * 255.0f));
    char green = max((unsigned int)(COLOR_GET_Gi(color) * m_fBrightness), (unsigned int)(m_fAnim * 255.0f));
    char blue = max((unsigned int)(COLOR_GET_Bi(color) * m_fBrightness), (unsigned int)(m_fAnim * 255.0f));
    g->setColor(
        COLOR(clamp<int>(COLOR_GET_Ai(color) + (isMouseInside() ? (int)(m_fAlphaAddOnHover * 255.0f) : 0), 0, 255), red,
              green, blue));

    buttonLeft->bind();
    { g->drawQuad((int)m_vPos.x, (int)m_vPos.y, (int)leftWidth, (int)m_vSize.y); }
    buttonLeft->unbind();

    buttonMiddle->bind();
    { g->drawQuad((int)m_vPos.x + (int)leftWidth, (int)m_vPos.y, (int)middleWidth, (int)m_vSize.y); }
    buttonMiddle->unbind();

    buttonRight->bind();
    { g->drawQuad((int)m_vPos.x + (int)leftWidth + (int)middleWidth, (int)m_vPos.y, (int)rightWidth, (int)m_vSize.y); }
    buttonRight->unbind();

    if(is_loading) {
        const float scale = (m_vSize.y * 0.8) / osu->getSkin()->getLoadingSpinner()->getSize().y;
        g->setColor(0xffffffff);
        g->pushTransform();
        g->rotate(engine->getTime() * 180, 0, 0, 1);
        g->scale(scale, scale);
        g->translate(m_vPos.x + m_vSize.x / 2.0f, m_vPos.y + m_vSize.y / 2.0f);
        g->drawImage(osu->getSkin()->getLoadingSpinner());
        g->popTransform();
    } else {
        drawText(g);
    }
}

void UIButton::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible) return;
    CBaseUIButton::mouse_update(propagate_clicks);

    if(isMouseInside() && m_tooltipTextLines.size() > 0 && !m_bFocusStolenDelay) {
        osu->getTooltipOverlay()->begin();
        {
            for(int i = 0; i < m_tooltipTextLines.size(); i++) {
                osu->getTooltipOverlay()->addLine(m_tooltipTextLines[i]);
            }
        }
        osu->getTooltipOverlay()->end();
    }

    m_bFocusStolenDelay = false;
}

void UIButton::onMouseInside() {
    m_fBrightness = 1.0f;

    if(button_sound_cooldown + 0.05f < engine->getTime()) {
        engine->getSound()->play(osu->getSkin()->m_hoverButton);
        button_sound_cooldown = engine->getTime();
    }
}

void UIButton::onMouseOutside() { m_fBrightness = 0.85f; }

void UIButton::onClicked() {
    if(is_loading) return;

    CBaseUIButton::onClicked();

    animateClickColor();

    engine->getSound()->play(osu->getSkin()->m_clickButton);
}

void UIButton::onFocusStolen() {
    CBaseUIButton::onFocusStolen();

    m_bMouseInside = false;
    m_bFocusStolenDelay = true;
}

void UIButton::animateClickColor() {
    m_fAnim = 1.0f;
    anim->moveLinear(&m_fAnim, 0.0f, 0.5f, true);
}

void UIButton::setTooltipText(UString text) { m_tooltipTextLines = text.split("\n"); }
