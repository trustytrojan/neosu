//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		generic checkbox
//
// $NoKeywords: $osucb
//===============================================================================//

#ifndef OSUUICHECKBOX_H
#define OSUUICHECKBOX_H

#include "CBaseUICheckbox.h"

class Osu;

class UICheckbox : public CBaseUICheckbox {
   public:
    UICheckbox(Osu *osu, float xPos, float yPos, float xSize, float ySize, UString name, UString text);

    virtual void mouse_update(bool *propagate_clicks);

    void setTooltipText(UString text);

   private:
    virtual void onFocusStolen();

    Osu *m_osu;
    std::vector<UString> m_tooltipTextLines;

    bool m_bFocusStolenDelay;
};

#endif
