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

    m_vScale = Vector2(1, 1);
    m_fScaleMultiplier = 1.1f;

    m_fAlpha = 1.0f;
}

void UIPauseMenuButton::draw(Graphics *g) {
    if(!m_bVisible) return;

    // draw image
    Image *image = getImageFunc();
    if(image != NULL) {
        g->setColor(COLORf(m_fAlpha, 1.0f, 1.0f, 1.0f));
        g->pushTransform();
        {
            // scale
            g->scale(m_vScale.x, m_vScale.y);

            // center and draw
            g->translate(m_vPos.x + (int)(m_vSize.x / 2), m_vPos.y + (int)(m_vSize.y / 2));
            g->drawImage(image);
        }
        g->popTransform();
    }
}

void UIPauseMenuButton::setBaseScale(float xScale, float yScale) {
    m_vBaseScale.x = xScale;
    m_vBaseScale.y = yScale;

    m_vScale = m_vBaseScale;
}

void UIPauseMenuButton::onMouseInside() {
    CBaseUIButton::onMouseInside();

    if(engine->hasFocus()) engine->getSound()->play(osu->getSkin()->getMenuHover());

    const float animationDuration = 0.09f;
    anim->moveLinear(&m_vScale.x, m_vBaseScale.x * m_fScaleMultiplier, animationDuration, true);
    anim->moveLinear(&m_vScale.y, m_vBaseScale.y * m_fScaleMultiplier, animationDuration, true);

    if(getName() == UString("Resume")) {
        engine->getSound()->play(osu->getSkin()->m_hoverPauseContinue);
    } else if(getName() == UString("Retry")) {
        engine->getSound()->play(osu->getSkin()->m_hoverPauseRetry);
    } else if(getName() == UString("Quit")) {
        engine->getSound()->play(osu->getSkin()->m_hoverPauseBack);
    }
}

void UIPauseMenuButton::onMouseOutside() {
    CBaseUIButton::onMouseOutside();

    const float animationDuration = 0.09f;
    anim->moveLinear(&m_vScale.x, m_vBaseScale.x, animationDuration, true);
    anim->moveLinear(&m_vScale.y, m_vBaseScale.y, animationDuration, true);
}
