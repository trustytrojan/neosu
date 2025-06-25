#include "CBaseUIWindow.h"

#include "AnimationHandler.h"
#include "CBaseUIBoxShadow.h"
#include "CBaseUIButton.h"
#include "CBaseUIContainer.h"
#include "ConVar.h"
#include "Cursors.h"
#include "Engine.h"
#include "Environment.h"
#include "Mouse.h"
#include "RenderTarget.h"
#include "ResourceManager.h"

using namespace std;

CBaseUIWindow::CBaseUIWindow(float xPos, float yPos, float xSize, float ySize, UString name)
    : CBaseUIElement(xPos, yPos, xSize, ySize, name) {
    const float dpiScale = env->getDPIScale();

    int titleBarButtonSize = 13 * dpiScale;
    int titleBarButtonGap = 6 * dpiScale;

    // titlebar
    this->bDrawTitleBarLine = true;
    this->titleFont =
        resourceManager->loadFont("weblysleekuisb.ttf", "FONT_WINDOW_TITLE", 13.0f, true, env->getDPI());
    this->iTitleBarHeight = this->titleFont->getHeight() + 12 * dpiScale;
    if(this->iTitleBarHeight < titleBarButtonSize) this->iTitleBarHeight = titleBarButtonSize + 4 * dpiScale;

    this->titleBarContainer =
        new CBaseUIContainer(this->vPos.x, this->vPos.y, this->vSize.x, this->iTitleBarHeight, "titlebarcontainer");

    this->closeButton = new CBaseUIButton(
        this->vSize.x - titleBarButtonSize - (this->iTitleBarHeight - titleBarButtonSize) / 2.0f,
        this->iTitleBarHeight / 2.0f - titleBarButtonSize / 2.0f, titleBarButtonSize, titleBarButtonSize, "", "");
    this->closeButton->setClickCallback(fastdelegate::MakeDelegate(this, &CBaseUIWindow::close));
    this->closeButton->setDrawFrame(false);

    this->minimizeButton = new CBaseUIButton(
        this->vSize.x - titleBarButtonSize * 2 - (this->iTitleBarHeight - titleBarButtonSize) / 2.0f -
            titleBarButtonGap,
        this->iTitleBarHeight / 2.0f - titleBarButtonSize / 2.0f, titleBarButtonSize, titleBarButtonSize, "", "");
    this->minimizeButton->setVisible(false);
    this->minimizeButton->setDrawFrame(false);
    this->minimizeButton->setClickCallback(fastdelegate::MakeDelegate(this, &CBaseUIWindow::minimize));

    this->titleBarContainer->addBaseUIElement(this->minimizeButton);
    this->titleBarContainer->addBaseUIElement(this->closeButton);

    // main container
    this->container =
        new CBaseUIContainer(this->vPos.x, this->vPos.y + this->titleBarContainer->getSize().y, this->vSize.x,
                             this->vSize.y - this->titleBarContainer->getSize().y, "maincontainer");

    // colors
    this->frameColor = 0xffffffff;
    this->backgroundColor = 0xff000000;
    this->frameBrightColor = 0;
    this->frameDarkColor = 0;
    this->titleColor = 0xffffffff;

    // events
    this->vResizeLimit = Vector2(100, 90) * dpiScale;
    this->bMoving = false;
    this->bResizing = false;
    this->iResizeType = 0;  // 1 == top left, 2 == left, 3 == bottom left, 4 == bottom, 5 = bottom right, 6 == right,
                            // 7 == top right, 8 == top

    // window properties
    this->bIsOpen = false;
    this->bAnimIn = false;
    this->bResizeable = true;
    this->bCoherenceMode = false;
    this->fAnimation = 0.0f;

    this->bDrawFrame = true;
    this->bDrawBackground = true;
    this->bRoundedRectangle = false;

    // test features
    // m_rt = resourceManager->createRenderTarget(this->vPos.x, this->vPos.y, this->vSize.x+1,
    // this->vSize.y+1); float shadowRadius = ui_window_shadow_radius.getInt();
    /// m_shadow = new CBaseUIBoxShadow(0xff000000, shadowRadius, this->vPos.x-shadowRadius, this->vPos.y-shadowRadius,
    /// m_vSize.x+shadowRadius*2, this->vSize.y+shadowRadius*2+4, "windowshadow");

    this->setTitle(name);
    this->setVisible(false);

    // for very small resolutions on engine start
    if(this->vPos.y + this->vSize.y > engine->getScreenHeight()) {
        this->setSizeY(engine->getScreenHeight() - 12 * dpiScale);
    }
}

CBaseUIWindow::~CBaseUIWindow() {
    /// SAFE_DELETE(this->shadow);
    SAFE_DELETE(this->container);
    SAFE_DELETE(this->titleBarContainer);
}

void CBaseUIWindow::draw() {
    if(!this->bVisible) return;

    // TODO: structure
    /*
    if (!anim->isAnimating(&m_fAnimation))
            m_shadow->draw();
    else
    {
            m_shadow->setColor(COLOR((int)((this->fAnimation)*255.0f), 255, 255, 255));

            // HACKHACK: shadows can't render inside a 3DScene
            m_shadow->renderOffscreen();

            g->push3DScene(McRect(this->vPos.x, this->vPos.y, this->vSize.x, this->vSize.y));
                    g->rotate3DScene(0, (this->bAnimIn ? -1 : 1) * (1-m_fAnimation)*10, 0);
                    g->translate3DScene(0, 0, -(1-m_fAnimation)*100);
                    m_shadow->draw();
            g->pop3DScene();
    }
    */

    // draw window
    // if (anim->isAnimating(&m_fAnimation) && !m_bCoherenceMode)
    //	m_rt->enable();

    {
        // draw background
        if(this->bDrawBackground) {
            g->setColor(this->backgroundColor);

            if(this->bRoundedRectangle) {
                // int border = 0;
                g->fillRoundedRect(this->vPos.x, this->vPos.y, this->vSize.x + 1, this->vSize.y + 1, 6);
            } else
                g->fillRect(this->vPos.x, this->vPos.y, this->vSize.x + 1, this->vSize.y + 1);
        }

        // draw frame
        if(this->bDrawFrame) {
            if(this->frameDarkColor != 0 || this->frameBrightColor != 0)
                g->drawRect(this->vPos.x, this->vPos.y, this->vSize.x, this->vSize.y, this->frameDarkColor,
                            this->frameBrightColor, this->frameBrightColor, this->frameDarkColor);
            else {
                g->setColor(/*m_bEnabled ? 0xffffff00 : */ this->frameColor);
                g->drawRect(this->vPos.x, this->vPos.y, this->vSize.x, this->vSize.y);
            }
        }

        // draw window contents
        g->pushClipRect(McRect(this->vPos.x + 1, this->vPos.y + 2, this->vSize.x - 1, this->vSize.y - 1));
        {
            // draw main container
            g->pushClipRect(McRect(this->vPos.x + 1, this->vPos.y + 2, this->vSize.x - 1, this->vSize.y - 1));
            {
                this->container->draw();
                this->drawCustomContent();
            }
            g->popClipRect();

            // draw title bar background
            if(this->bDrawBackground && !this->bRoundedRectangle) {
                g->setColor(this->backgroundColor);
                g->fillRect(this->vPos.x, this->vPos.y, this->vSize.x, this->iTitleBarHeight);
            }

            // draw title bar line
            if(this->bDrawTitleBarLine) {
                g->setColor(this->frameColor);
                g->drawLine(this->vPos.x, this->vPos.y + this->iTitleBarHeight, this->vPos.x + this->vSize.x,
                            this->vPos.y + this->iTitleBarHeight);
            }

            // draw title
            g->setColor(this->titleColor);
            g->pushTransform();
            {
                g->translate((int)(this->vPos.x + this->vSize.x / 2.0f - this->fTitleFontWidth / 2.0f),
                             (int)(this->vPos.y + this->fTitleFontHeight / 2.0f + this->iTitleBarHeight / 2.0f));
                g->drawString(this->titleFont, this->sTitle);
            }
            g->popTransform();

            // draw title bar container
            g->pushClipRect(McRect(this->vPos.x + 1, this->vPos.y + 2, this->vSize.x - 1, this->iTitleBarHeight));
            { this->titleBarContainer->draw(); }
            g->popClipRect();

            // draw close button 'x'
            g->setColor(this->closeButton->getFrameColor());
            g->drawLine(this->closeButton->getPos().x + 1, this->closeButton->getPos().y + 1,
                        this->closeButton->getPos().x + this->closeButton->getSize().x,
                        this->closeButton->getPos().y + this->closeButton->getSize().y);
            g->drawLine(this->closeButton->getPos().x + 1,
                        this->closeButton->getPos().y + this->closeButton->getSize().y - 1,
                        this->closeButton->getPos().x + this->closeButton->getSize().x, this->closeButton->getPos().y);

            // draw minimize button '_'
            if(this->minimizeButton->isVisible()) {
                g->setColor(this->minimizeButton->getFrameColor());
                g->drawLine(this->minimizeButton->getPos().x + 2,
                            this->minimizeButton->getPos().y + this->minimizeButton->getSize().y - 2,
                            this->minimizeButton->getPos().x + this->minimizeButton->getSize().x - 1,
                            this->minimizeButton->getPos().y + this->minimizeButton->getSize().y - 2);
            }
        }
        g->popClipRect();
    }

    // TODO: structure
    if(anim->isAnimating(&this->fAnimation) && !this->bCoherenceMode) {
        /*
        m_rt->disable();


        m_rt->setColor(COLOR((int)(this->fAnimation*255.0f), 255, 255, 255));

        g->push3DScene(McRect(this->vPos.x, this->vPos.y, this->vSize.x, this->vSize.y));
                g->rotate3DScene((this->bAnimIn ? -1 : 1) * (1-m_fAnimation)*10, 0, 0);
                g->translate3DScene(0, 0, -(1-m_fAnimation)*100);
                m_rt->draw(this->vPos.x, this->vPos.y);
        g->pop3DScene();
        */
    }
}

void CBaseUIWindow::mouse_update(bool *propagate_clicks) {
    if(!this->bVisible) return;
    CBaseUIElement::mouse_update(propagate_clicks);

    // after the close animation is finished, set invisible
    if(this->fAnimation == 0.0f && this->bVisible) this->setVisible(false);

    // window logic comes first
    if(!this->titleBarContainer->isBusy() && !this->container->isBusy() && this->bMouseInside && this->bEnabled)
        this->updateWindowLogic();

    // the main two containers
    this->titleBarContainer->mouse_update(propagate_clicks);
    this->container->mouse_update(propagate_clicks);

    // moving
    if(this->bMoving) this->setPos(this->vLastPos + (mouse->getPos() - this->vMousePosBackup));

    // resizing
    if(this->bResizing) {
        switch(this->iResizeType) {
            case 1:
                this->setPos(clamp<float>(this->vLastPos.x + (mouse->getPos().x - this->vMousePosBackup.x),
                                          -this->vSize.x, this->vLastPos.x + this->vLastSize.x - this->vResizeLimit.x),
                             clamp<float>(this->vLastPos.y + (mouse->getPos().y - this->vMousePosBackup.y),
                                          -this->vSize.y, this->vLastPos.y + this->vLastSize.y - this->vResizeLimit.y));
                this->setSize(
                    clamp<float>(this->vLastSize.x + (this->vMousePosBackup.x - mouse->getPos().x),
                                 this->vResizeLimit.x, engine->getScreenWidth()),
                    clamp<float>(this->vLastSize.y + (this->vMousePosBackup.y - mouse->getPos().y),
                                 this->vResizeLimit.y, engine->getScreenHeight()));
                break;

            case 2:
                this->setPosX(
                    clamp<float>(this->vLastPos.x + (mouse->getPos().x - this->vMousePosBackup.x),
                                 -this->vSize.x, this->vLastPos.x + this->vLastSize.x - this->vResizeLimit.x));
                this->setSizeX(
                    clamp<float>(this->vLastSize.x + (this->vMousePosBackup.x - mouse->getPos().x),
                                 this->vResizeLimit.x, engine->getScreenWidth()));
                break;

            case 3:
                this->setPosX(
                    clamp<float>(this->vLastPos.x + (mouse->getPos().x - this->vMousePosBackup.x),
                                 -this->vSize.x, this->vLastPos.x + this->vLastSize.x - this->vResizeLimit.x));
                this->setSizeX(
                    clamp<float>(this->vLastSize.x + (this->vMousePosBackup.x - mouse->getPos().x),
                                 this->vResizeLimit.x, engine->getScreenWidth()));
                this->setSizeY(
                    clamp<float>(this->vLastSize.y + (mouse->getPos().y - this->vMousePosBackup.y),
                                 this->vResizeLimit.y, engine->getScreenHeight()));
                break;

            case 4:
                this->setSizeY(
                    clamp<float>(this->vLastSize.y + (mouse->getPos().y - this->vMousePosBackup.y),
                                 this->vResizeLimit.y, engine->getScreenHeight()));
                break;

            case 5:
                this->setSize(
                    clamp<float>(this->vLastSize.x + (mouse->getPos().x - this->vMousePosBackup.x),
                                 this->vResizeLimit.x, engine->getScreenWidth()),
                    clamp<float>(this->vLastSize.y + (mouse->getPos().y - this->vMousePosBackup.y),
                                 this->vResizeLimit.y, engine->getScreenHeight()));
                break;

            case 6:
                this->setSizeX(
                    clamp<float>(this->vLastSize.x + (mouse->getPos().x - this->vMousePosBackup.x),
                                 this->vResizeLimit.x, engine->getScreenWidth()));
                break;

            case 7:
                this->setPosY(
                    clamp<float>(this->vLastPos.y + (mouse->getPos().y - this->vMousePosBackup.y),
                                 -this->vSize.y, this->vLastPos.y + this->vLastSize.y - this->vResizeLimit.y));
                this->setSizeY(
                    clamp<float>(this->vLastSize.y + (this->vMousePosBackup.y - mouse->getPos().y),
                                 this->vResizeLimit.y, engine->getScreenHeight()));
                this->setSizeX(
                    clamp<float>(this->vLastSize.x + (mouse->getPos().x - this->vMousePosBackup.x),
                                 this->vResizeLimit.x, engine->getScreenWidth()));
                break;

            case 8:
                this->setPosY(
                    clamp<float>(this->vLastPos.y + (mouse->getPos().y - this->vMousePosBackup.y),
                                 -this->vSize.y, this->vLastPos.y + this->vLastSize.y - this->vResizeLimit.y));
                this->setSizeY(
                    clamp<float>(this->vLastSize.y + (this->vMousePosBackup.y - mouse->getPos().y),
                                 this->vResizeLimit.y, engine->getScreenHeight()));
                break;
        }
    }
}

void CBaseUIWindow::onKeyDown(KeyboardEvent &e) {
    if(!this->bVisible) return;
    this->container->onKeyDown(e);
}

void CBaseUIWindow::onKeyUp(KeyboardEvent &e) {
    if(!this->bVisible) return;
    this->container->onKeyUp(e);
}

void CBaseUIWindow::onChar(KeyboardEvent &e) {
    if(!this->bVisible) return;
    this->container->onChar(e);
}

CBaseUIWindow *CBaseUIWindow::setTitle(UString text) {
    this->sTitle = text;
    this->updateTitleBarMetrics();
    return this;
}

void CBaseUIWindow::updateWindowLogic() {
    if(!mouse->isLeftDown()) {
        this->bMoving = false;
        this->bResizing = false;
    }

    // handle resize & move cursor
    if(!this->titleBarContainer->isBusy() && !this->container->isBusy() && !this->bResizing && !this->bMoving) {
        if(!mouse->isLeftDown()) this->udpateResizeAndMoveLogic(false);
    }
}

void CBaseUIWindow::udpateResizeAndMoveLogic(bool captureMouse) {
    if(this->bCoherenceMode) return;  // NOTE: resizing in coherence mode is handled in main_Windows.cpp

    // backup
    this->vLastSize = this->vSize;
    this->vMousePosBackup = mouse->getPos();
    this->vLastPos = this->vPos;

    if(this->bResizeable) {
        // reset
        this->iResizeType = 0;

        int resizeHandleSize = 5;
        McRect resizeTopLeft = McRect(this->vPos.x, this->vPos.y, resizeHandleSize, resizeHandleSize);
        McRect resizeLeft = McRect(this->vPos.x, this->vPos.y, resizeHandleSize, this->vSize.y);
        McRect resizeBottomLeft =
            McRect(this->vPos.x, this->vPos.y + this->vSize.y - resizeHandleSize, resizeHandleSize, resizeHandleSize);
        McRect resizeBottom =
            McRect(this->vPos.x, this->vPos.y + this->vSize.y - resizeHandleSize, this->vSize.x, resizeHandleSize);
        McRect resizeBottomRight =
            McRect(this->vPos.x + this->vSize.x - resizeHandleSize, this->vPos.y + this->vSize.y - resizeHandleSize,
                   resizeHandleSize, resizeHandleSize);
        McRect resizeRight =
            McRect(this->vPos.x + this->vSize.x - resizeHandleSize, this->vPos.y, resizeHandleSize, this->vSize.y);
        McRect resizeTopRight =
            McRect(this->vPos.x + this->vSize.x - resizeHandleSize, this->vPos.y, resizeHandleSize, resizeHandleSize);
        McRect resizeTop = McRect(this->vPos.x, this->vPos.y, this->vSize.x, resizeHandleSize);

        if(resizeTopLeft.contains(this->vMousePosBackup)) {
            if(captureMouse) this->iResizeType = 1;

            mouse->setCursorType(CURSORTYPE::CURSOR_SIZE_VH);
        } else if(resizeBottomLeft.contains(this->vMousePosBackup)) {
            if(captureMouse) this->iResizeType = 3;

            mouse->setCursorType(CURSORTYPE::CURSOR_SIZE_HV);
        } else if(resizeBottomRight.contains(this->vMousePosBackup)) {
            if(captureMouse) this->iResizeType = 5;

            mouse->setCursorType(CURSORTYPE::CURSOR_SIZE_VH);
        } else if(resizeTopRight.contains(this->vMousePosBackup)) {
            if(captureMouse) this->iResizeType = 7;

            mouse->setCursorType(CURSORTYPE::CURSOR_SIZE_HV);
        } else if(resizeLeft.contains(this->vMousePosBackup)) {
            if(captureMouse) this->iResizeType = 2;

            mouse->setCursorType(CURSORTYPE::CURSOR_SIZE_H);
        } else if(resizeRight.contains(this->vMousePosBackup)) {
            if(captureMouse) this->iResizeType = 6;

            mouse->setCursorType(CURSORTYPE::CURSOR_SIZE_H);
        } else if(resizeBottom.contains(this->vMousePosBackup)) {
            if(captureMouse) this->iResizeType = 4;

            mouse->setCursorType(CURSORTYPE::CURSOR_SIZE_V);
        } else if(resizeTop.contains(this->vMousePosBackup)) {
            if(captureMouse) this->iResizeType = 8;

            mouse->setCursorType(CURSORTYPE::CURSOR_SIZE_V);
        }
    }

    // handle resizing
    if(this->iResizeType > 0)
        this->bResizing = true;
    else if(captureMouse) {
        // handle moving
        McRect titleBarGrab = McRect(this->vPos.x, this->vPos.y, this->vSize.x, this->iTitleBarHeight);
        if(titleBarGrab.contains(this->vMousePosBackup)) this->bMoving = true;
    }

    // resizing and moving have priority over window contents
    if(this->bResizing || this->bMoving) this->container->stealFocus();
}

void CBaseUIWindow::close() {
    if(anim->isAnimating(&this->fAnimation)) return;

    this->bAnimIn = false;
    this->fAnimation = 1.0f;
    anim->moveQuadInOut(&this->fAnimation, 0.0f, cv_ui_window_animspeed.getFloat());

    this->onClosed();
}

void CBaseUIWindow::open() {
    if(anim->isAnimating(&this->fAnimation) || this->bVisible) return;

    this->setVisible(true);

    if(!this->bCoherenceMode) {
        this->bAnimIn = true;
        this->fAnimation = 0.001f;
        anim->moveQuadOut(&this->fAnimation, 1.0f, cv_ui_window_animspeed.getFloat());
    } else
        this->fAnimation = 1.0f;
}

void CBaseUIWindow::minimize() {
    if(this->bCoherenceMode) env->minimize();
}

CBaseUIWindow *CBaseUIWindow::setSizeToContent(int horizontalBorderSize, int verticalBorderSize) {
    const std::vector<CBaseUIElement *> &elements = this->container->getElements();
    if(elements.size() < 1) return this;

    Vector2 newSize = Vector2(horizontalBorderSize, verticalBorderSize);

    for(size_t i = 0; i < elements.size(); i++) {
        const CBaseUIElement *el = elements[i];

        int xReach = el->getRelPos().x + el->getSize().x + horizontalBorderSize;
        int yReach = el->getRelPos().y + el->getSize().y + verticalBorderSize;
        if(xReach > newSize.x) newSize.x = xReach;
        if(yReach > newSize.y) newSize.y = yReach;
    }
    newSize.y = newSize.y + this->titleBarContainer->getSize().y;

    this->setSize(newSize);

    return this;
}

CBaseUIWindow *CBaseUIWindow::enableCoherenceMode() {
    this->bCoherenceMode = true;

    this->minimizeButton->setVisible(true);
    this->setPos(0, 0);
    this->setSize(engine->getScreenWidth() - 1, engine->getScreenHeight() - 1);
    /// env->setWindowSize(this->vSize.x+1, this->vSize.y+1);

    return this;
}

void CBaseUIWindow::onMouseDownInside() {
    this->bBusy = true;
    bool wtf = true;
    this->titleBarContainer->mouse_update(&wtf);  // why is this called here lol?
    if(!this->titleBarContainer->isBusy()) this->udpateResizeAndMoveLogic(true);
}

void CBaseUIWindow::onMouseUpInside() {
    this->bBusy = false;
    this->bResizing = false;
    this->bMoving = false;
}

void CBaseUIWindow::onMouseUpOutside() {
    this->bBusy = false;
    this->bResizing = false;
    this->bMoving = false;
}

void CBaseUIWindow::updateTitleBarMetrics() {
    this->closeButton->setRelPos(this->vSize.x - this->closeButton->getSize().x -
                                     (this->iTitleBarHeight - this->closeButton->getSize().x) / 2.0f,
                                 this->iTitleBarHeight / 2.0f - this->closeButton->getSize().y / 2.0f);
    this->minimizeButton->setRelPos(this->vSize.x - this->minimizeButton->getSize().x * 2 -
                                        (this->iTitleBarHeight - this->minimizeButton->getSize().x) / 2.0f - 6,
                                    this->iTitleBarHeight / 2.0f - this->minimizeButton->getSize().y / 2.0f);

    this->fTitleFontWidth = this->titleFont->getStringWidth(this->sTitle);
    this->fTitleFontHeight = this->titleFont->getHeight();
    this->titleBarContainer->setSize(this->vSize.x, this->iTitleBarHeight);
}

void CBaseUIWindow::onMoved() {
    this->titleBarContainer->setPos(this->vPos);
    this->container->setPos(this->vPos.x, this->vPos.y + this->titleBarContainer->getSize().y);

    this->updateTitleBarMetrics();

    // if (!m_bCoherenceMode)
    //	m_rt->setPos(this->vPos);
    /// m_shadow->setPos(this->vPos.x-m_shadow->getRadius(), this->vPos.y-m_shadow->getRadius());
}

void CBaseUIWindow::onResized() {
    this->updateTitleBarMetrics();

    this->container->setSize(this->vSize.x, this->vSize.y - this->titleBarContainer->getSize().y);

    // if (!m_bCoherenceMode)
    //	m_rt->rebuild(this->vPos.x, this->vPos.y, this->vSize.x+1, this->vSize.y+1);
    /// m_shadow->setSize(this->vSize.x+m_shadow->getRadius()*2, this->vSize.y+m_shadow->getRadius()*2+4);
}

void CBaseUIWindow::onResolutionChange(Vector2 newResolution) {
    if(this->bCoherenceMode) this->setSize(newResolution.x - 1, newResolution.y - 1);
}

void CBaseUIWindow::onEnabled() {
    this->container->setEnabled(true);
    this->titleBarContainer->setEnabled(true);
}

void CBaseUIWindow::onDisabled() {
    this->bBusy = false;
    this->container->setEnabled(false);
    this->titleBarContainer->setEnabled(false);
}

void CBaseUIWindow::onClosed() {
    if(this->bCoherenceMode) engine->shutdown();
}

bool CBaseUIWindow::isBusy() {
    return (this->bBusy || this->titleBarContainer->isBusy() || this->container->isBusy()) && this->bVisible;
}

bool CBaseUIWindow::isActive() {
    return (this->titleBarContainer->isActive() || this->container->isActive()) && this->bVisible;
}
