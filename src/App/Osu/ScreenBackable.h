//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		screen + back button
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUSCREENBACKABLE_H
#define OSUSCREENBACKABLE_H

#include "OsuScreen.h"
#include "UIBackButton.h"

class Osu;

class ScreenBackable : public OsuScreen {
   public:
    ScreenBackable(Osu *osu);
    virtual ~ScreenBackable();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);
    virtual void onKeyDown(KeyboardEvent &e);
    virtual void onResolutionChange(Vector2 newResolution);

   protected:
    virtual void onBack() = 0;

    virtual void updateLayout();

    UIBackButton *m_backButton;
};

#endif
