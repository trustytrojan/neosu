//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		baseclass for any drawable screen state object of the game
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUSCREEN_H
#define OSUSCREEN_H

#include "CBaseUIContainer.h"
#include "cbase.h"

class Osu;
class KeyboardEvent;

class OsuScreen : public CBaseUIContainer {
   public:
    OsuScreen(Osu *osu) {
        m_osu = osu;
        m_bVisible = false;
    }
    virtual ~OsuScreen() { ; }

    virtual void onResolutionChange(Vector2 newResolution) { (void)newResolution; }

    Osu *m_osu;
};

#endif
