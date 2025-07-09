#include "VSTitleBar.h"

#include <utility>

#include "AnimationHandler.h"
#include "CBaseUIButton.h"
#include "CBaseUIContainer.h"
#include "ConVar.h"
#include "Engine.h"
#include "Mouse.h"
#include "ResourceManager.h"



class VSTitleBarButton : public CBaseUIButton {
   public:
    VSTitleBarButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text)
        : CBaseUIButton(xPos, yPos, xSize, ySize, std::move(name), std::move(text)) {
        ;
    }
    ~VSTitleBarButton() override { ; }

    void draw() override {
        if(!this->bVisible) return;

        // default background gradient
        {
            const Color top = argb(255, 244, 244, 244);
            const Color bottom = argb(255, 221, 221, 221);

            g->fillGradient(this->vPos.x + 1, this->vPos.y + 1, this->vSize.x - 1, this->vSize.y, top, top, bottom,
                            bottom);
        }

        // blue seekbar overlay
        {
            const float seekBarPercent = cv_vs_percent.getFloat();
            if(seekBarPercent > 0.0f) {
                const Color middle = argb(255, 0, 50, 119);
                const Color third = argb(255, 0, 113 - 50, 207 - 50);
                const Color top = argb(255, 0, 196, 223);

                const float sizeThird = this->vSize.y / 3.0f;

                g->fillGradient(this->vPos.x + 1, this->vPos.y + 1, (this->vSize.x - 2) * seekBarPercent, sizeThird,
                                top, top, third, third);
                g->fillGradient(this->vPos.x + 1, this->vPos.y + 1 + sizeThird, (this->vSize.x - 2) * seekBarPercent,
                                std::round(sizeThird / 2.0f) + 1, third, third, middle, middle);
                g->fillGradient(this->vPos.x + 1, this->vPos.y + 1 + sizeThird + std::round(sizeThird / 2.0f),
                                (this->vSize.x - 2) * seekBarPercent, std::round(sizeThird / 2.0f) + 1, middle, middle,
                                third, third);
                g->fillGradient(this->vPos.x + 1, this->vPos.y + 1 + 2 * sizeThird,
                                (this->vSize.x - 2) * seekBarPercent, sizeThird, third, third, top, top);
            }
        }

        // bottom line
        {
            g->setColor(argb(255, 204, 204, 204));
            g->drawLine(this->vPos.x, this->vPos.y + this->vSize.y, this->vPos.x + this->vSize.x,
                        this->vPos.y + this->vSize.y);
        }

        this->drawText();
    }
};

VSTitleBar::VSTitleBar(int x, int y, int xSize, McFont *font)
    : CBaseUIElement(x, y, xSize, 44 * env->getDPIScale(), "") {
    this->font = font;

    const Color textColor = argb(215, 55, 55, 55);

    this->container = new CBaseUIContainer(0, 0, this->vSize.x, this->vSize.y, "");

    this->title = new VSTitleBarButton(0, 0, xSize - 1, this->vSize.y, "", "");
    this->title->setDrawBackground(false);
    this->title->setDrawFrame(false);
    this->title->setTextColor(textColor);
    this->title->setFont(this->font);
    this->container->addBaseUIElement(this->title);

    this->title2 = new VSTitleBarButton(0, 0, xSize - 1, this->vSize.y, "", "Ready");
    this->title2->setDrawBackground(true);
    this->title2->setDrawFrame(false);
    this->title2->setTextColor(textColor);
    this->title2->setFont(this->font);
    this->container->addBaseUIElement(this->title2);

    // vars
    this->fRot = 0.0f;
    this->iFlip = 0;

    this->bIsSeeking = false;
}

VSTitleBar::~VSTitleBar() { SAFE_DELETE(this->container); }

void VSTitleBar::draw() {
    if(this->iFlip != 0) {
        // 3d flip effect
        if((this->fRot > -45.0f && this->iFlip == 1) || (this->iFlip == 2 && this->fRot < 45.0f)) {
            g->push3DScene(McRect(this->vPos.x, this->vPos.y, this->title->getSize().x, this->title->getSize().y));
            {
                g->offset3DScene(0, 0, this->title->getSize().y / 2);
                g->rotate3DScene(this->fRot + (this->iFlip == 1 ? 90 : -90), 0, 0);
                this->drawTitle1();
            }
            g->pop3DScene();

            g->push3DScene(McRect(this->vPos.x, this->vPos.y, this->title2->getSize().x, this->title2->getSize().y));
            {
                g->offset3DScene(0, 0, this->title2->getSize().y / 2);
                g->rotate3DScene(this->fRot, 0, 0);
                this->drawTitle2();
            }
            g->pop3DScene();
        } else {
            g->push3DScene(McRect(this->vPos.x, this->vPos.y, this->title2->getSize().x, this->title2->getSize().y));
            {
                g->offset3DScene(0, 0, this->title2->getSize().y / 2);
                g->rotate3DScene(this->fRot, 0, 0);

                this->drawTitle2();
            }
            g->pop3DScene();

            g->push3DScene(McRect(this->vPos.x, this->vPos.y, this->title->getSize().x, this->title->getSize().y));
            {
                g->offset3DScene(0, 0, this->title->getSize().y / 2);
                g->rotate3DScene(this->fRot + (this->iFlip == 1 ? 90 : -90), 0, 0);

                this->drawTitle1();
            }
            g->pop3DScene();
        }
    } else
        this->drawTitle2();
}

void VSTitleBar::drawTitle1() {
    this->title->draw();
    if(cv_vs_percent.getFloat() > 0) {
        this->title->setTextColor(0xffffffff);
        g->pushClipRect(
            McRect(this->vPos.x, this->vPos.y, cv_vs_percent.getFloat() * this->vSize.x, this->title2->getSize().y));
        { this->title->draw(); }
        g->popClipRect();
        this->title->setTextColor(argb(255, 55, 55, 55));
    }
}

void VSTitleBar::drawTitle2() {
    this->title2->draw();
    if(cv_vs_percent.getFloat() > 0) {
        this->title2->setTextColor(0xffffffff);
        g->pushClipRect(
            McRect(this->vPos.x, this->vPos.y, cv_vs_percent.getFloat() * this->vSize.x, this->title2->getSize().y));
        { this->title2->draw(); }
        g->popClipRect();
        this->title2->setTextColor(argb(255, 55, 55, 55));
    }
}

void VSTitleBar::mouse_update(bool *propagate_clicks) {
    if(!this->bVisible) return;
    CBaseUIElement::mouse_update(propagate_clicks);

    this->container->mouse_update(propagate_clicks);

    // handle 3d flip logic
    if(this->iFlip != 0) {
        if(std::abs(this->fRot) == 90.0f) {
            anim->deleteExistingAnimation(&this->fRot);
            this->fRot = 0.0f;
            this->iFlip = 0;

            // text switch
            UString backupt = this->title2->getText();
            { this->title2->setText(this->title->getText()); }
            this->title->setText(backupt);
        }
    }

    // handle scrubbing
    if(this->title2->isActive() && this->bActive) {
        this->bIsSeeking = true;
        const float percent =
            std::clamp<float>((mouse->getPos().x + 1 - this->vPos.x) / this->title->getSize().x, 0.0f, 1.0f);
        cv_vs_percent.setValue(percent);
    } else {
        // fire seek callback once scrubbing stops
        if(this->bIsSeeking) {
            this->bIsSeeking = false;
            if(this->seekCallback != NULL && !mouse->isRightDown()) this->seekCallback();
        }
    }
}

void VSTitleBar::onResized() {
    this->container->setSize(this->vSize);

    this->title->setSize(this->vSize.x - 1, this->vSize.y);
    this->title2->setSize(this->vSize.x - 1, this->vSize.y);
}

void VSTitleBar::onMoved() {
    // forward
    this->container->setPos(this->vPos);
}

void VSTitleBar::onFocusStolen() {
    // forward
    this->container->onFocusStolen();
}

void VSTitleBar::setTitle(UString title, bool reverse) {
    this->title->setText(std::move(title));
    if(anim->isAnimating(&this->fRot)) return;

    this->iFlip = 1;
    if(reverse) this->iFlip = 2;

    anim->moveQuadInOut(&this->fRot, reverse == false ? -90.0f : 90.0f, 0.45f);
}
