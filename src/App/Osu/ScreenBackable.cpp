//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		screen + back button
//
// $NoKeywords: $
//===============================================================================//

#include "ScreenBackable.h"

#include "KeyBindings.h"
#include "Keyboard.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "Skin.h"
#include "UIBackButton.h"

ScreenBackable::ScreenBackable() : OsuScreen() {
    m_backButton = new UIBackButton(-1, 0, 0, 0, "");
    m_backButton->setClickCallback(fastdelegate::MakeDelegate(this, &ScreenBackable::onBack));

    updateLayout();
}

ScreenBackable::~ScreenBackable() { SAFE_DELETE(m_backButton); }

void ScreenBackable::draw(Graphics *g) {
    if(!m_bVisible) return;
    OsuScreen::draw(g);
    m_backButton->draw(g);
}

void ScreenBackable::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible) return;
    m_backButton->mouse_update(propagate_clicks);
    if(!*propagate_clicks) return;
    OsuScreen::mouse_update(propagate_clicks);
}

void ScreenBackable::onKeyDown(KeyboardEvent &e) {
    OsuScreen::onKeyDown(e);
    if(!m_bVisible || e.isConsumed()) return;

    if(e == KEY_ESCAPE || e == (KEYCODE)KeyBindings::GAME_PAUSE.getInt()) {
        onBack();
        e.consume();
        return;
    }
}

void ScreenBackable::updateLayout() {
    m_backButton->updateLayout();
    m_backButton->setPosY(osu->getScreenHeight() - m_backButton->getSize().y);
}

void ScreenBackable::onResolutionChange(Vector2 newResolution) { updateLayout(); }
