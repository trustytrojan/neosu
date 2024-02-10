//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		screen + back button
//
// $NoKeywords: $
//===============================================================================//

#include "OsuScreenBackable.h"

#include "Keyboard.h"
#include "ResourceManager.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuKeyBindings.h"

#include "OsuUIBackButton.h"

OsuScreenBackable::OsuScreenBackable(Osu *osu) : OsuScreen(osu)
{
	m_backButton = new OsuUIBackButton(m_osu, -1, 0, 0, 0, "");
	m_backButton->setClickCallback( fastdelegate::MakeDelegate(this, &OsuScreenBackable::onBack) );

	updateLayout();
}

OsuScreenBackable::~OsuScreenBackable()
{
	SAFE_DELETE(m_backButton);
}

void OsuScreenBackable::draw(Graphics *g)
{
	if (!m_bVisible) return;
	OsuScreen::draw(g);
	m_backButton->draw(g);
}

void OsuScreenBackable::mouse_update(bool *propagate_clicks)
{
	if(!m_bVisible) return;
	m_backButton->mouse_update(propagate_clicks);
	if(!*propagate_clicks) return;
	OsuScreen::mouse_update(propagate_clicks);
}

void OsuScreenBackable::onKeyDown(KeyboardEvent &e)
{
	OsuScreen::onKeyDown(e);
	if (!m_bVisible || e.isConsumed()) return;

	if (e == KEY_ESCAPE || e == (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt()) {
		onBack();
		e.consume();
		return;
	}
}

void OsuScreenBackable::updateLayout()
{
	m_backButton->updateLayout();
	m_backButton->setPosY(m_osu->getScreenHeight() - m_backButton->getSize().y);
}

void OsuScreenBackable::onResolutionChange(Vector2 newResolution)
{
	updateLayout();
}
