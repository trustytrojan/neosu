//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		linux context menu interface
//
// $NoKeywords: $linuxcmenu
//===============================================================================//

// TODO: DEPRECATED

#ifdef __linux__

#ifndef LINUXCONTEXTMENU_H
#define LINUXCONTEXTMENU_H

#include "ContextMenu.h"

class LinuxContextMenu : public ContextMenu {
   public:
    LinuxContextMenu();
    ~LinuxContextMenu() override;

    void begin() override { ; }
    void addItem(UString text, int returnValue) override { ; }
    void addSeparator() override { ; }
    int end() override { return -1; }

   private:
};

#endif

#endif
