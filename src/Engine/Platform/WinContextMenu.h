//================ Copyright (c) 2012, PG, All rights reserved. =================//
//
// Purpose:		windows context menu interface
//
// $NoKeywords: $wincmenu
//===============================================================================//

// TODO: DEPRECATED

#ifdef _WIN32

#ifndef WINCONTEXTMENU_H
#define WINCONTEXTMENU_H

#include "ContextMenu.h"
#include <Windows.h>

class WinContextMenu : public ContextMenu
{
public:
	WinContextMenu();
	virtual ~WinContextMenu();

	void begin();
	void addItem(UString text, int returnValue);
	void addSeparator();
	int end();

private:
	HMENU m_menu;
};

#endif

#endif
