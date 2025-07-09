#include "VSControlBar.h"

#include <utility>

#include "AnimationHandler.h"
#include "CBaseUICheckbox.h"
#include "CBaseUIContainer.h"
#include "CBaseUISlider.h"
#include "ConVar.h"
#include "Engine.h"
#include "Keyboard.h"
#include "ResourceManager.h"

class VSControlBarButton : public CBaseUIButton {
   public:
    VSControlBarButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text)
        : CBaseUIButton(xPos, yPos, xSize, ySize, std::move(name), std::move(text)) {
        ;
    }
    ~VSControlBarButton() override { ; }

    void draw() override {
        if(!this->bVisible) return;

        const Color top = argb(255, 244, 244, 244);
        const Color bottom = argb(255, 221, 221, 221);

        g->fillGradient(this->vPos.x + 1, this->vPos.y + 1, this->vSize.x - 1, this->vSize.y, top, top, bottom, bottom);

        g->setColor(argb(255, 204, 204, 204));
        g->drawRect(this->vPos.x, this->vPos.y, this->vSize.x, this->vSize.y);

        this->drawText();
    }

   protected:
    void drawText() override {
        if(this->font != NULL && this->sText.length() > 0) {
            const int textPressedAdd = (this->bActive ? 1 : 0);

            g->pushClipRect(McRect(this->vPos.x + 1, this->vPos.y + 1, this->vSize.x - 1, this->vSize.y - 1));
            {
                g->setColor(this->textColor);
                g->pushTransform();
                {
                    g->translate((int)(this->vPos.x + this->vSize.x / 2.0f - this->fStringWidth / 2.0f),
                                 (int)(this->vPos.y + this->vSize.y / 2.0f + this->fStringHeight / 2.0f));
                    g->translate((int)(1 + textPressedAdd), (int)(1 + textPressedAdd));

                    if(this->textDarkColor != 0)
                        g->setColor(this->textDarkColor);
                    else
                        g->setColor(Colors::invert(this->textColor));

                    g->drawString(this->font, this->sText);
                }
                g->popTransform();
                g->pushTransform();
                {
                    g->translate(
                        (int)(this->vPos.x + this->vSize.x / 2.0f - this->fStringWidth / 2.0f + textPressedAdd),
                        (int)(this->vPos.y + this->vSize.y / 2.0f + this->fStringHeight / 2.0f + textPressedAdd));

                    if(this->textBrightColor != 0)
                        g->setColor(this->textBrightColor);
                    else
                        g->setColor(this->textColor);

                    g->drawString(this->font, this->sText);
                }
                g->popTransform();
            }
            g->popClipRect();
        }
    }
};

class VSControlBarSlider : public CBaseUISlider {
   public:
    VSControlBarSlider(float xPos, float yPos, float xSize, float ySize, UString name)
        : CBaseUISlider(xPos, yPos, xSize, ySize, std::move(name)) {
        ;
    }
    ~VSControlBarSlider() override { ; }

    void draw() override {
        CBaseUISlider::draw();
        if(!this->bVisible) return;

        g->drawQuad(
            Vector2(this->vPos.x + std::round(this->vBlockPos.x) + 1, this->vPos.y + std::round(this->vBlockPos.y) + 1),
            Vector2(this->vPos.x + std::round(this->vBlockPos.x) + 1 + this->vBlockSize.x - 1,
                    this->vPos.y + std::round(this->vBlockPos.y) + 1),
            Vector2(this->vPos.x + std::round(this->vBlockPos.x) + 1 + this->vBlockSize.x - 1,
                    this->vPos.y + std::round(this->vBlockPos.y) + 1 + this->vBlockSize.y / 2),
            Vector2(this->vPos.x + std::round(this->vBlockPos.x) + 1,
                    this->vPos.y + std::round(this->vBlockPos.y) + 1 + this->vBlockSize.y / 2),
            argb(255, 205, 218, 243), argb(255, 205, 218, 243), argb(255, 141, 188, 238), argb(255, 141, 188, 238));

        g->drawQuad(Vector2(this->vPos.x + std::round(this->vBlockPos.x) + 1,
                            this->vPos.y + std::round(this->vBlockPos.y) + 1 + std::round(this->vBlockSize.y / 2.0f)),
                    Vector2(this->vPos.x + std::round(this->vBlockPos.x) + 1 + this->vBlockSize.x - 1,
                            this->vPos.y + std::round(this->vBlockPos.y) + 1 + std::round(this->vBlockSize.y / 2.0f)),
                    Vector2(this->vPos.x + std::round(this->vBlockPos.x) + 1 + this->vBlockSize.x - 1,
                            this->vPos.y + std::round(this->vBlockPos.y) + 1 + std::round(this->vBlockSize.y / 2.0f) +
                                this->vBlockSize.y - (std::round(this->vBlockSize.y / 2.0f))),
                    Vector2(this->vPos.x + std::round(this->vBlockPos.x) + 1,
                            this->vPos.y + std::round(this->vBlockPos.y) + 1 + std::round(this->vBlockSize.y / 2.0f) +
                                this->vBlockSize.y - (std::round(this->vBlockSize.y / 2.0f))),
                    argb(255, 105, 173, 243), argb(255, 105, 173, 243), argb(255, 185, 253, 254),
                    argb(255, 185, 253, 254));
    }
};

class VSControlBarCheckbox : public CBaseUICheckbox {
   public:
    VSControlBarCheckbox(float xPos, float yPos, float xSize, float ySize, UString name, UString text)
        : CBaseUICheckbox(xPos, yPos, xSize, ySize, std::move(name), std::move(text)) {
        ;
    }
    ~VSControlBarCheckbox() override { ; }

    void draw() override {
        if(!this->bVisible) return;

        const Color top = (this->bChecked ? argb(255, 178, 237, 171) : argb(255, 244, 244, 244));
        const Color bottom = (this->bChecked ? argb(255, 117, 211, 114) : argb(255, 221, 221, 221));

        g->fillGradient(this->vPos.x + 1, this->vPos.y + 1, this->vSize.x - 1, this->vSize.y, top, top, bottom, bottom);

        g->setColor(argb(255, 204, 204, 204));
        g->drawRect(this->vPos.x, this->vPos.y, this->vSize.x, this->vSize.y);

        this->drawText();
    }
};

VSControlBar::VSControlBar(int x, int y, int xSize, int ySize, McFont *font) : CBaseUIElement(x, y, xSize, ySize, "") {
    cv_vs_volume.setCallback(fastdelegate::MakeDelegate(this, &VSControlBar::onVolumeChanged));

    const float dpiScale = env->getDPIScale();
    const int height = 22 * dpiScale;
    const Color textColor = argb(215, 55, 55, 55);

    this->container = new CBaseUIContainer(0, 0, engine->getScreenWidth(), engine->getScreenHeight(), "");

    this->play = new VSControlBarButton(this->vSize.x / 2.0f - height, 0, height * 2, (height * 2), "", ">");
    this->play->setTextColor(textColor);
    this->play->setFont(font);
    this->container->addBaseUIElement(this->play);

    this->prev = new VSControlBarButton(this->play->getRelPos().x - this->play->getSize().x, 0, height * 2,
                                        (height * 2), "", "<<");
    this->prev->setTextColor(textColor);
    this->prev->setFont(font);
    this->container->addBaseUIElement(this->prev);

    this->next = new VSControlBarButton(this->play->getRelPos().x + this->play->getSize().x, 0, height * 2,
                                        (height * 2), "", ">>");
    this->next->setTextColor(textColor);
    this->next->setFont(font);
    this->container->addBaseUIElement(this->next);

    this->volume = new VSControlBarSlider(0, 0, this->prev->getRelPos().x, height * 2, "");
    this->volume->setOrientation(true);
    this->volume->setDrawBackground(false);
    this->volume->setFrameColor(argb(255, 204, 204, 204));
    this->volume->setBackgroundColor(0xffffffff);
    this->volume->setBounds(0.0f, 1.0f);
    this->volume->setInitialValue(cv_vs_volume.getFloat());
    this->volume->setLiveUpdate(true);
    this->container->addBaseUIElement(this->volume);

    this->info = new VSControlBarButton(
        this->next->getRelPos().x + this->next->getSize().x, 0,
        this->vSize.x - 2 * height - (this->next->getRelPos().x + this->next->getSize().x), height * 2, "", "");
    this->info->setTextColor(textColor);
    this->info->setFont(font);
    this->info->setTextLeft(true);
    this->container->addBaseUIElement(this->info);

    this->shuffle = new VSControlBarCheckbox(this->vSize.x - height, 0, height, height, "", "");
    this->shuffle->setChangeCallback(fastdelegate::MakeDelegate(this, &VSControlBar::onShuffleCheckboxChanged));
    this->shuffle->setTextColor(textColor);
    this->shuffle->setFont(font);
    this->container->addBaseUIElement(this->shuffle);

    this->repeat = new VSControlBarCheckbox(this->vSize.x - 2 * height, 0, height, height, "", "r");
    this->repeat->setChangeCallback(fastdelegate::MakeDelegate(this, &VSControlBar::onRepeatCheckboxChanged));
    this->repeat->setTextColor(textColor);
    this->repeat->setFont(font);
    this->container->addBaseUIElement(this->repeat);

    this->eq = new VSControlBarCheckbox(this->vSize.x - 2 * height, height, height, height, "", "");
    this->eq->setTextColor(textColor);
    this->eq->setFont(font);
    this->container->addBaseUIElement(this->eq);

    this->settings = new VSControlBarCheckbox(this->vSize.x - height, height, height, height, "", "");
    this->settings->setTextColor(textColor);
    this->settings->setFont(font);
    this->container->addBaseUIElement(this->settings);
}

VSControlBar::~VSControlBar() { SAFE_DELETE(this->container); }

void VSControlBar::draw() {
    if(!this->bVisible) return;

    // draw background gradient
    {
        const Color top = argb(255, 244, 244, 244);
        const Color bottom = argb(255, 221, 221, 221);

        g->fillGradient(this->vPos.x, this->vPos.y, this->vSize.x, this->vSize.y, top, top, bottom, bottom);
    }

    this->container->draw();
}

void VSControlBar::mouse_update(bool *propagate_clicks) {
    if(!this->bVisible) return;
    CBaseUIElement::mouse_update(propagate_clicks);

    this->container->mouse_update(propagate_clicks);
}

void VSControlBar::onRepeatCheckboxChanged(CBaseUICheckbox *box) { cv_vs_repeat.setValue((float)box->isChecked()); }

void VSControlBar::onShuffleCheckboxChanged(CBaseUICheckbox *box) { cv_vs_shuffle.setValue((float)box->isChecked()); }

void VSControlBar::onVolumeChanged(UString oldValue, UString newValue) {
    (void)oldValue;
    (void)newValue;
    this->volume->setValue(cv_vs_volume.getFloat());
}

void VSControlBar::onMoved() { this->container->setPos(this->vPos); }

void VSControlBar::onFocusStolen() {
    // forward
    this->container->stealFocus();
}

void VSControlBar::onEnabled() {
    // forward
    this->container->setEnabled(true);
}

void VSControlBar::onDisabled() {
    // forward
    this->container->setEnabled(false);
}

void VSControlBar::onResized() {
    this->play->setRelPosX(this->vSize.x / 2 - this->play->getSize().x / 2);
    this->prev->setRelPosX(this->play->getRelPos().x - this->prev->getSize().x);
    this->next->setRelPosX(this->play->getRelPos().x + this->play->getSize().x);
    this->info->setRelPosX(this->next->getRelPos().x + this->next->getSize().x);
    this->volume->setSizeX(this->prev->getRelPos().x);

    this->shuffle->setRelPosX(this->vSize.x - this->shuffle->getSize().x);
    this->repeat->setRelPosX(this->vSize.x - 2 * this->repeat->getSize().x);
    this->eq->setRelPos(this->vSize.x - 2 * this->eq->getSize().x, this->eq->getSize().y);
    this->settings->setRelPos(this->vSize.x - this->settings->getSize().x, this->settings->getSize().y);

    this->info->setSizeX(this->repeat->getRelPos().x - (this->next->getRelPos().x + this->next->getSize().x) +
                         1);  // +1 fudge

    this->container->setSize(this->vSize);
}
