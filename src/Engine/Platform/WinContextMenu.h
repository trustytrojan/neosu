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

#ifdef NOMINMAX
#undef NOMINMAX
#endif

#define NOMINMAX
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
#define NOCRYPT
#define NOMETAFILE
#define MMNOSOUND

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "ContextMenu.h"

class WinContextMenu : public ContextMenu {
   public:
    WinContextMenu();
    virtual ~WinContextMenu();

    void begin();
    void addItem(UString text, int returnValue);
    void addSeparator();
    int end();

   private:
    HMENU menu;
};

#endif

#endif
