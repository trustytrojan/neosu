//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		context menu, dropdown style
//
// $NoKeywords: $
//===============================================================================//

#include "OsuUIContextMenu.h"

#include "Engine.h"
#include "Mouse.h"
#include "AnimationHandler.h"
#include "Keyboard.h"

#include "CBaseUIContainer.h"
#include "CBaseUIScrollView.h"

#include "Osu.h"
#include "OsuKeyBindings.h"
#include "OsuTooltipOverlay.h"



OsuUIContextMenuButton::OsuUIContextMenuButton(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name, UString text, int id) : CBaseUIButton(xPos, yPos, xSize, ySize, name, text)
{
	m_osu = osu;
	m_iID = id;
}

void OsuUIContextMenuButton::mouse_update(bool *propagate_clicks)
{
	if (!m_bVisible) return;
	CBaseUIButton::mouse_update(propagate_clicks);

	if (isMouseInside() && m_tooltipTextLines.size() > 0)
	{
		m_osu->getTooltipOverlay()->begin();
		{
			for (int i=0; i<m_tooltipTextLines.size(); i++)
			{
				m_osu->getTooltipOverlay()->addLine(m_tooltipTextLines[i]);
			}
		}
		m_osu->getTooltipOverlay()->end();
	}
}

void OsuUIContextMenuButton::setTooltipText(UString text)
{
	m_tooltipTextLines = text.split("\n");
}



OsuUIContextMenuTextbox::OsuUIContextMenuTextbox(float xPos, float yPos, float xSize, float ySize, UString name, int id) : CBaseUITextbox(xPos, yPos, xSize, ySize, name)
{
	m_iID = id;
}



OsuUIContextMenu::OsuUIContextMenu(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name, CBaseUIScrollView *parent) : CBaseUIScrollView(xPos, yPos, xSize, ySize, name)
{
	m_osu = osu;
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

void OsuUIContextMenu::draw(Graphics *g)
{
	if (!m_bVisible || !m_bVisible2) return;

	if (m_fAnimation > 0.0f && m_fAnimation < 1.0f)
	{
		g->push3DScene(McRect(m_vPos.x, m_vPos.y + ((m_vSize.y/2.0f) * (m_bInvertAnimation ? 1.0f : -1.0f)), m_vSize.x, m_vSize.y));
		g->rotate3DScene((1.0f - m_fAnimation)*90.0f * (m_bInvertAnimation ? 1.0f : -1.0f), 0, 0);
	}

	// draw background
	g->setColor(0xff222222);
	g->setAlpha(m_fAnimation);
	g->fillRect(m_vPos.x+1, m_vPos.y+1, m_vSize.x-1, m_vSize.y-1);

	// draw frame
	g->setColor(0xffffffff);
	g->setAlpha(m_fAnimation*m_fAnimation);
	g->drawRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);

	CBaseUIScrollView::draw(g);

	if (m_fAnimation > 0.0f && m_fAnimation < 1.0f)
		g->pop3DScene();
}

void OsuUIContextMenu::mouse_update(bool *propagate_clicks)
{
	if (!m_bVisible || !m_bVisible2) return;
	CBaseUIScrollView::mouse_update(propagate_clicks);

	if (m_containedTextbox != NULL)
	{
		if (m_containedTextbox->hitEnter())
			onHitEnter(m_containedTextbox);
	}

	// HACKHACK: mouse wheel handling order
	if (m_bClampUnderflowAndOverflowAndEnableScrollingIfNecessary)
	{
		if (isMouseInside())
			engine->getMouse()->resetWheelDelta();
	}

	if (m_selfDeletionCrashWorkaroundScheduledElementDeleteHack.size() > 0)
	{
		for (size_t i=0; i<m_selfDeletionCrashWorkaroundScheduledElementDeleteHack.size(); i++)
		{
			delete m_selfDeletionCrashWorkaroundScheduledElementDeleteHack[i];
		}

		m_selfDeletionCrashWorkaroundScheduledElementDeleteHack.clear();
	}
}

void OsuUIContextMenu::onKeyUp(KeyboardEvent &e)
{
	if (!m_bVisible || !m_bVisible2) return;

	CBaseUIScrollView::onKeyUp(e);
}

void OsuUIContextMenu::onKeyDown(KeyboardEvent &e)
{
	if (!m_bVisible || !m_bVisible2) return;

	CBaseUIScrollView::onKeyDown(e);

	// also force ENTER event if context menu textbox has lost focus (but context menu is still visible, e.g. if the user clicks inside the context menu but outside the textbox)
	if (m_containedTextbox != NULL)
	{
		if (e == KEY_ENTER)
		{
			e.consume();
			onHitEnter(m_containedTextbox);
		}
	}

	// hide on ESC
	if (!e.isConsumed())
	{
		if (e == KEY_ESCAPE || e == (KEYCODE)OsuKeyBindings::GAME_PAUSE.getInt())
		{
			e.consume();
			setVisible2(false);
		}
	}
}

void OsuUIContextMenu::onChar(KeyboardEvent &e)
{
	if (!m_bVisible || !m_bVisible2) return;

	CBaseUIScrollView::onChar(e);
}

void OsuUIContextMenu::begin(int minWidth, bool bigStyle)
{
	m_iWidthCounter = minWidth;
	m_bBigStyle = bigStyle;

	m_iYCounter = 0;
	m_clickCallback = NULL;

	setSizeX(m_iWidthCounter);

	// HACKHACK: bad design workaround.
	// HACKHACK: callbacks from the same context menu which call begin() to create a new context menu may crash because begin() deletes the object the callback is currently being called from
	// HACKHACK: so, instead we just keep a list of things to delete whenever we get to the next update() tick
	{
		const std::vector<CBaseUIElement*> &oldElementsWeCanNotDeleteYet = getContainer()->getElements();
		m_selfDeletionCrashWorkaroundScheduledElementDeleteHack.insert(m_selfDeletionCrashWorkaroundScheduledElementDeleteHack.end(), oldElementsWeCanNotDeleteYet.begin(), oldElementsWeCanNotDeleteYet.end());
		getContainer()->empty(); // ensure nothing is deleted yet
	}

	clear();

	m_containedTextbox = NULL;
}

OsuUIContextMenuButton *OsuUIContextMenu::addButton(UString text, int id)
{
	const int buttonHeight = 30 * Osu::getUIScale(m_osu) * (m_bBigStyle ? 1.27f : 1.0f);
	const int margin = 9 * Osu::getUIScale(m_osu);

	OsuUIContextMenuButton *button = new OsuUIContextMenuButton(m_osu, margin, m_iYCounter + margin, 0, buttonHeight, text, text, id);
	{
		if (m_bBigStyle)
			button->setFont(m_osu->getSubTitleFont());

		button->setClickCallback( fastdelegate::MakeDelegate(this, &OsuUIContextMenu::onClick) );
		button->setWidthToContent(3 * Osu::getUIScale(m_osu));
		button->setTextLeft(true);
		button->setDrawFrame(false);
		button->setDrawBackground(false);
	}
	getContainer()->addBaseUIElement(button);

	if (button->getSize().x+2*margin > m_iWidthCounter)
	{
		m_iWidthCounter = button->getSize().x + 2*margin;
		setSizeX(m_iWidthCounter);
	}

	m_iYCounter += buttonHeight;
	setSizeY(m_iYCounter + 2*margin);

	return button;
}

OsuUIContextMenuTextbox *OsuUIContextMenu::addTextbox(UString text, int id)
{
	const int buttonHeight = 30 * Osu::getUIScale(m_osu) * (m_bBigStyle ? 1.27f : 1.0f);
	const int margin = 9 * Osu::getUIScale(m_osu);

	OsuUIContextMenuTextbox *textbox = new OsuUIContextMenuTextbox(margin, m_iYCounter + margin, 0, buttonHeight, text, id);
	{
		textbox->setText(text);

		if (m_bBigStyle)
			textbox->setFont(m_osu->getSubTitleFont());

		textbox->setActive(true);
	}
	getContainer()->addBaseUIElement(textbox);

	m_iYCounter += buttonHeight;
	setSizeY(m_iYCounter + 2*margin);

	// NOTE: only one single textbox is supported currently
	m_containedTextbox = textbox;

	return textbox;
}

void OsuUIContextMenu::end(bool invertAnimation, bool clampUnderflowAndOverflowAndEnableScrollingIfNecessary)
{
	m_bInvertAnimation = invertAnimation;
	m_bClampUnderflowAndOverflowAndEnableScrollingIfNecessary = clampUnderflowAndOverflowAndEnableScrollingIfNecessary;

	const int margin = 9 * Osu::getUIScale(m_osu);

	const std::vector<CBaseUIElement*> &elements = getContainer()->getElements();
	for (size_t i=0; i<elements.size(); i++)
	{
		(elements[i])->setSizeX(m_iWidthCounter - 2*margin);
	}

	// scrollview handling and edge cases
	{
		setVerticalScrolling(false);

		if (m_bClampUnderflowAndOverflowAndEnableScrollingIfNecessary)
		{
			if (m_vPos.y < 0)
			{
				const float underflow = std::abs(m_vPos.y);

				setRelPosY(m_vPos.y + underflow);
				setPosY(m_vPos.y + underflow);
				setSizeY(m_vSize.y - underflow);

				setVerticalScrolling(true);
			}

			if (m_vPos.y + m_vSize.y > m_osu->getScreenHeight())
			{
				const float overflow = std::abs(m_vPos.y + m_vSize.y - m_osu->getScreenHeight());

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

void OsuUIContextMenu::setVisible2(bool visible2)
{
	m_bVisible2 = visible2;

	if (!m_bVisible2)
		setSize(1, 1); // reset size

	if (m_parent != NULL)
		m_parent->setScrollSizeToContent(); // and update parent scroll size
}

void OsuUIContextMenu::onResized()
{
	setSize(m_vSize);
}

void OsuUIContextMenu::onMoved()
{
	setRelPos(m_vPos);
	setPos(m_vPos);
}

void OsuUIContextMenu::onMouseDownOutside()
{
	setVisible2(false);
}

void OsuUIContextMenu::onClick(CBaseUIButton *button)
{
	setVisible2(false);

	if (m_clickCallback != NULL)
	{
		// special case: if text input exists, then override with its text
		if (m_containedTextbox != NULL)
			m_clickCallback(m_containedTextbox->getText(), ((OsuUIContextMenuButton*)button)->getID());
		else
			m_clickCallback(button->getName(), ((OsuUIContextMenuButton*)button)->getID());
	}
}

void OsuUIContextMenu::onHitEnter(OsuUIContextMenuTextbox *textbox)
{
	setVisible2(false);

	if (m_clickCallback != NULL)
		m_clickCallback(textbox->getText(), textbox->getID());
}

void OsuUIContextMenu::clampToBottomScreenEdge(OsuUIContextMenu *menu)
{
	if (menu->getRelPos().y + menu->getSize().y > menu->m_osu->getScreenHeight())
	{
		int newRelPosY = menu->m_osu->getScreenHeight() - menu->getSize().y - 1;
		menu->setRelPosY(newRelPosY);
		menu->setPosY(newRelPosY);
	}
}

void OsuUIContextMenu::clampToRightScreenEdge(OsuUIContextMenu *menu)
{
	if (menu->getRelPos().x + menu->getSize().x > menu->m_osu->getScreenWidth())
	{
		const int newRelPosX = menu->m_osu->getScreenWidth() - menu->getSize().x - 1;
		menu->setRelPosX(newRelPosX);
		menu->setPosX(newRelPosX);
	}
}
