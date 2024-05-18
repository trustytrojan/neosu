#include "UIContextMenu.h"

#include "AnimationHandler.h"
#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"
#include "Engine.h"
#include "KeyBindings.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Osu.h"
#include "TooltipOverlay.h"

UIContextMenuButton::UIContextMenuButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text,
                                         int id)
    : CBaseUIButton(xPos, yPos, xSize, ySize, name, text) {
    m_iID = id;
}

void UIContextMenuButton::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible) return;
    CBaseUIButton::mouse_update(propagate_clicks);

    if(isMouseInside() && m_tooltipTextLines.size() > 0) {
        osu->getTooltipOverlay()->begin();
        {
            for(int i = 0; i < m_tooltipTextLines.size(); i++) {
                osu->getTooltipOverlay()->addLine(m_tooltipTextLines[i]);
            }
        }
        osu->getTooltipOverlay()->end();
    }
}

void UIContextMenuButton::setTooltipText(UString text) { m_tooltipTextLines = text.split("\n"); }

UIContextMenuTextbox::UIContextMenuTextbox(float xPos, float yPos, float xSize, float ySize, UString name, int id)
    : CBaseUITextbox(xPos, yPos, xSize, ySize, name) {
    m_iID = id;
}

UIContextMenu::UIContextMenu(float xPos, float yPos, float xSize, float ySize, UString name, CBaseUIScrollView *parent)
    : CBaseUIScrollView(xPos, yPos, xSize, ySize, name) {
    m_parent = parent;

    setPos(xPos, yPos);
    setSize(xSize, ySize);
    setName(name);

    setHorizontalScrolling(false);
    setDrawBackground(false);
    setDrawFrame(false);
    setScrollbarSizeMultiplier(0.5f);

    m_containedTextbox = NULL;

    m_iYCounter = 0;
    m_iWidthCounter = 0;

    m_bVisible = false;
    m_bVisible2 = false;
    m_clickCallback = NULL;

    m_fAnimation = 0.0f;
    m_bInvertAnimation = false;

    m_bBigStyle = false;
    m_bClampUnderflowAndOverflowAndEnableScrollingIfNecessary = false;
}

void UIContextMenu::draw(Graphics *g) {
    if(!m_bVisible || !m_bVisible2) return;

    if(m_fAnimation > 0.0f && m_fAnimation < 1.0f) {
        g->push3DScene(McRect(m_vPos.x, m_vPos.y + ((m_vSize.y / 2.0f) * (m_bInvertAnimation ? 1.0f : -1.0f)),
                              m_vSize.x, m_vSize.y));
        g->rotate3DScene((1.0f - m_fAnimation) * 90.0f * (m_bInvertAnimation ? 1.0f : -1.0f), 0, 0);
    }

    // draw background
    g->setColor(0xff222222);
    g->setAlpha(m_fAnimation);
    g->fillRect(m_vPos.x + 1, m_vPos.y + 1, m_vSize.x - 1, m_vSize.y - 1);

    // draw frame
    g->setColor(0xffffffff);
    g->setAlpha(m_fAnimation * m_fAnimation);
    g->drawRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);

    CBaseUIScrollView::draw(g);

    if(m_fAnimation > 0.0f && m_fAnimation < 1.0f) g->pop3DScene();
}

void UIContextMenu::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible || !m_bVisible2) return;
    CBaseUIScrollView::mouse_update(propagate_clicks);

    if(m_containedTextbox != NULL) {
        if(m_containedTextbox->hitEnter()) onHitEnter(m_containedTextbox);
    }

    // HACKHACK: mouse wheel handling order
    if(m_bClampUnderflowAndOverflowAndEnableScrollingIfNecessary) {
        if(isMouseInside()) engine->getMouse()->resetWheelDelta();
    }

    if(m_selfDeletionCrashWorkaroundScheduledElementDeleteHack.size() > 0) {
        for(size_t i = 0; i < m_selfDeletionCrashWorkaroundScheduledElementDeleteHack.size(); i++) {
            delete m_selfDeletionCrashWorkaroundScheduledElementDeleteHack[i];
        }

        m_selfDeletionCrashWorkaroundScheduledElementDeleteHack.clear();
    }
}

void UIContextMenu::onKeyUp(KeyboardEvent &e) {
    if(!m_bVisible || !m_bVisible2) return;

    CBaseUIScrollView::onKeyUp(e);
}

void UIContextMenu::onKeyDown(KeyboardEvent &e) {
    if(!m_bVisible || !m_bVisible2) return;

    CBaseUIScrollView::onKeyDown(e);

    // also force ENTER event if context menu textbox has lost focus (but context menu is still visible, e.g. if the
    // user clicks inside the context menu but outside the textbox)
    if(m_containedTextbox != NULL) {
        if(e == KEY_ENTER) {
            e.consume();
            onHitEnter(m_containedTextbox);
        }
    }

    // hide on ESC
    if(!e.isConsumed()) {
        if(e == KEY_ESCAPE || e == (KEYCODE)KeyBindings::GAME_PAUSE.getInt()) {
            e.consume();
            setVisible2(false);
        }
    }
}

void UIContextMenu::onChar(KeyboardEvent &e) {
    if(!m_bVisible || !m_bVisible2) return;

    CBaseUIScrollView::onChar(e);
}

void UIContextMenu::begin(int minWidth, bool bigStyle) {
    m_iWidthCounter = minWidth;
    m_bBigStyle = bigStyle;

    m_iYCounter = 0;
    m_clickCallback = NULL;

    setSizeX(m_iWidthCounter);

    // HACKHACK: bad design workaround.
    // HACKHACK: callbacks from the same context menu which call begin() to create a new context menu may crash because
    // begin() deletes the object the callback is currently being called from HACKHACK: so, instead we just keep a list
    // of things to delete whenever we get to the next update() tick
    {
        const std::vector<CBaseUIElement *> &oldElementsWeCanNotDeleteYet = getContainer()->getElements();
        m_selfDeletionCrashWorkaroundScheduledElementDeleteHack.insert(
            m_selfDeletionCrashWorkaroundScheduledElementDeleteHack.end(), oldElementsWeCanNotDeleteYet.begin(),
            oldElementsWeCanNotDeleteYet.end());
        getContainer()->empty();  // ensure nothing is deleted yet
    }

    clear();

    m_containedTextbox = NULL;
}

UIContextMenuButton *UIContextMenu::addButton(UString text, int id) {
    const int buttonHeight = 30 * Osu::getUIScale() * (m_bBigStyle ? 1.27f : 1.0f);
    const int margin = 9 * Osu::getUIScale();

    UIContextMenuButton *button =
        new UIContextMenuButton(margin, m_iYCounter + margin, 0, buttonHeight, text, text, id);
    {
        if(m_bBigStyle) button->setFont(osu->getSubTitleFont());

        button->setClickCallback(fastdelegate::MakeDelegate(this, &UIContextMenu::onClick));
        button->setWidthToContent(3 * Osu::getUIScale());
        button->setTextLeft(true);
        button->setDrawFrame(false);
        button->setDrawBackground(false);
    }
    getContainer()->addBaseUIElement(button);

    if(button->getSize().x + 2 * margin > m_iWidthCounter) {
        m_iWidthCounter = button->getSize().x + 2 * margin;
        setSizeX(m_iWidthCounter);
    }

    m_iYCounter += buttonHeight;
    setSizeY(m_iYCounter + 2 * margin);

    return button;
}

UIContextMenuTextbox *UIContextMenu::addTextbox(UString text, int id) {
    const int buttonHeight = 30 * Osu::getUIScale() * (m_bBigStyle ? 1.27f : 1.0f);
    const int margin = 9 * Osu::getUIScale();

    UIContextMenuTextbox *textbox = new UIContextMenuTextbox(margin, m_iYCounter + margin, 0, buttonHeight, text, id);
    {
        textbox->setText(text);

        if(m_bBigStyle) textbox->setFont(osu->getSubTitleFont());

        textbox->setActive(true);
    }
    getContainer()->addBaseUIElement(textbox);

    m_iYCounter += buttonHeight;
    setSizeY(m_iYCounter + 2 * margin);

    // NOTE: only one single textbox is supported currently
    m_containedTextbox = textbox;

    return textbox;
}

void UIContextMenu::end(bool invertAnimation, bool clampUnderflowAndOverflowAndEnableScrollingIfNecessary) {
    m_bInvertAnimation = invertAnimation;
    m_bClampUnderflowAndOverflowAndEnableScrollingIfNecessary = clampUnderflowAndOverflowAndEnableScrollingIfNecessary;

    const int margin = 9 * Osu::getUIScale();

    const std::vector<CBaseUIElement *> &elements = getContainer()->getElements();
    for(size_t i = 0; i < elements.size(); i++) {
        (elements[i])->setSizeX(m_iWidthCounter - 2 * margin);
    }

    // scrollview handling and edge cases
    {
        setVerticalScrolling(false);

        if(m_bClampUnderflowAndOverflowAndEnableScrollingIfNecessary) {
            if(m_vPos.y < 0) {
                const float underflow = std::abs(m_vPos.y);

                setRelPosY(m_vPos.y + underflow);
                setPosY(m_vPos.y + underflow);
                setSizeY(m_vSize.y - underflow);

                setVerticalScrolling(true);
            }

            if(m_vPos.y + m_vSize.y > osu->getScreenHeight()) {
                const float overflow = std::abs(m_vPos.y + m_vSize.y - osu->getScreenHeight());

                setSizeY(m_vSize.y - overflow - 1);

                setVerticalScrolling(true);
            }
        }

        setScrollSizeToContent();
    }

    setVisible2(true);

    m_fAnimation = 0.001f;
    anim->moveQuartOut(&m_fAnimation, 1.0f, 0.15f, true);
}

void UIContextMenu::setVisible2(bool visible2) {
    m_bVisible2 = visible2;

    if(!m_bVisible2) setSize(1, 1);  // reset size

    if(m_parent != NULL) m_parent->setScrollSizeToContent();  // and update parent scroll size
}

void UIContextMenu::onResized() { setSize(m_vSize); }

void UIContextMenu::onMoved() {
    setRelPos(m_vPos);
    setPos(m_vPos);
}

void UIContextMenu::onMouseDownOutside() { setVisible2(false); }

void UIContextMenu::onClick(CBaseUIButton *button) {
    setVisible2(false);

    if(m_clickCallback != NULL) {
        // special case: if text input exists, then override with its text
        if(m_containedTextbox != NULL)
            m_clickCallback(m_containedTextbox->getText(), ((UIContextMenuButton *)button)->getID());
        else
            m_clickCallback(button->getName(), ((UIContextMenuButton *)button)->getID());
    }
}

void UIContextMenu::onHitEnter(UIContextMenuTextbox *textbox) {
    setVisible2(false);

    if(m_clickCallback != NULL) m_clickCallback(textbox->getText(), textbox->getID());
}

void UIContextMenu::clampToBottomScreenEdge(UIContextMenu *menu) {
    if(menu->getRelPos().y + menu->getSize().y > osu->getScreenHeight()) {
        int newRelPosY = osu->getScreenHeight() - menu->getSize().y - 1;
        menu->setRelPosY(newRelPosY);
        menu->setPosY(newRelPosY);
    }
}

void UIContextMenu::clampToRightScreenEdge(UIContextMenu *menu) {
    if(menu->getRelPos().x + menu->getSize().x > osu->getScreenWidth()) {
        const int newRelPosX = osu->getScreenWidth() - menu->getSize().x - 1;
        menu->setRelPosX(newRelPosX);
        menu->setPosX(newRelPosX);
    }
}
