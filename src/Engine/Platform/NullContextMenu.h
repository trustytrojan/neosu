//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		empty implementation of ContextMenu
//
// $NoKeywords: $ncmenu
//===============================================================================//

#ifndef NULLCONTEXTMENU_H
#define NULLCONTEXTMENU_H

#include "ContextMenu.h"

class NullContextMenu : public ContextMenu {
   public:
    NullContextMenu() { ; }
    ~NullContextMenu() override { ; }

    void begin() override { ; }
    void addItem(UString text, int returnValue) override {
        (void)text;
        (void)returnValue;
    }
    void addSeparator() override { ; }
    int end() override { return -1; }
};

#endif
