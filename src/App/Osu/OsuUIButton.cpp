//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		generic button (context menu items, mod selection screen, etc.)
//
// $NoKeywords: $osubt
//===============================================================================//

#include "OsuUIButton.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "AnimationHandler.h"

#include "Osu.h"
#include "OsuSkin.h"
#include "OsuTooltipOverlay.h"

OsuUIButton::OsuUIButton(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name, UString text) : CBaseUIButton(xPos, yPos, xSize, ySize, name, text)
{
	m_osu = osu;

	m_bDefaultSkin = false;
	m_color = 0xffffffff;
	m_backupColor = m_color;
	m_fBrightness = 0.85f;
	m_fAnim = 0.0f;
	m_fAlphaAddOnHover = 0.0f;

	m_bFocusStolenDelay = false;
}

void OsuUIButton::draw(Graphics *g)
{
	if (!m_bVisible) return;

	Image *buttonLeft = m_bDefaultSkin ? m_osu->getSkin()->getDefaultButtonLeft() : m_osu->getSkin()->getButtonLeft();
	Image *buttonMiddle = m_bDefaultSkin ? m_osu->getSkin()->getDefaultButtonMiddle() : m_osu->getSkin()->getButtonMiddle();
	Image *buttonRight = m_bDefaultSkin ? m_osu->getSkin()->getDefaultButtonRight() : m_osu->getSkin()->getButtonRight();

	float leftScale = m_osu->getImageScaleToFitResolution(buttonLeft, m_vSize);
	float leftWidth = buttonLeft->getWidth()*leftScale;

	float rightScale = m_osu->getImageScaleToFitResolution(buttonRight, m_vSize);
	float rightWidth = buttonRight->getWidth()*rightScale;

	float middleWidth = m_vSize.x - leftWidth - rightWidth;

	auto color = is_loading ? 0xff333333 : m_color;
	char red = std::max((unsigned int)(COLOR_GET_Ri(color)*m_fBrightness), (unsigned int)(m_fAnim*255.0f));
	char green = std::max((unsigned int)(COLOR_GET_Gi(color)*m_fBrightness), (unsigned int)(m_fAnim*255.0f));
	char blue = std::max((unsigned int)(COLOR_GET_Bi(color)*m_fBrightness), (unsigned int)(m_fAnim*255.0f));
	g->setColor(COLOR(clamp<int>(COLOR_GET_Ai(color) + (isMouseInside() ? (int)(m_fAlphaAddOnHover*255.0f) : 0), 0, 255), red, green, blue));

	buttonLeft->bind();
	{
		g->drawQuad((int)m_vPos.x, (int)m_vPos.y, (int)leftWidth, (int)m_vSize.y);
	}
	buttonLeft->unbind();

	buttonMiddle->bind();
	{
		g->drawQuad((int)m_vPos.x + (int)leftWidth, (int)m_vPos.y, (int)middleWidth, (int)m_vSize.y);
	}
	buttonMiddle->unbind();

	buttonRight->bind();
	{
		g->drawQuad((int)m_vPos.x + (int)leftWidth + (int)middleWidth, (int)m_vPos.y, (int)rightWidth, (int)m_vSize.y);
	}
	buttonRight->unbind();

	if(is_loading) {
		// TODO @kiwec: scaling is incorrect depending on screen resolution?
		const float scale = Osu::getImageScale(m_osu, m_osu->getSkin()->getLoadingSpinner(), m_vSize.y * 0.8);
		g->setColor(0xffffffff);
		g->pushTransform();
		g->rotate(engine->getTime()*180, 0, 0, 1);
		g->scale(scale, scale);
		g->translate(m_vPos.x + m_vSize.x/2.0f, m_vPos.y + m_vSize.y/2.0f);
		g->drawImage(m_osu->getSkin()->getLoadingSpinner());
		g->popTransform();
	} else {
		drawText(g);
	}
}

void OsuUIButton::mouse_update(bool *propagate_clicks)
{
	if (!m_bVisible) return;
	CBaseUIButton::mouse_update(propagate_clicks);

	if (isMouseInside() && m_tooltipTextLines.size() > 0 && !m_bFocusStolenDelay)
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

	m_bFocusStolenDelay = false;
}

void OsuUIButton::onMouseInside()
{
	m_fBrightness = 1.0f;
}

void OsuUIButton::onMouseOutside()
{
	m_fBrightness = 0.85f;
}

void OsuUIButton::onClicked()
{
	if(is_loading) return;

	CBaseUIButton::onClicked();

	animateClickColor();
}

void OsuUIButton::onFocusStolen()
{
	CBaseUIButton::onFocusStolen();

	m_bMouseInside = false;
	m_bFocusStolenDelay = true;
}

void OsuUIButton::animateClickColor()
{
	m_fAnim = 1.0f;
	anim->moveLinear(&m_fAnim, 0.0f, 0.5f, true);
}

void OsuUIButton::setTooltipText(UString text)
{
	m_tooltipTextLines = text.split("\n");
}
