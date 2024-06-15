#pragma once

#include "CBaseUILabel.h"
#include "UString.h"

class ChatLink : public CBaseUILabel {
   public:
    ChatLink(float xPos = 0, float yPos = 0, float xSize = 0, float ySize = 0, UString link = "", UString label = "");

    virtual void mouse_update(bool *propagate_clicks);
    virtual void onMouseUpInside();

    UString m_link;
};
