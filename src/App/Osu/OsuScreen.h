#pragma once
#include "CBaseUIContainer.h"
#include "cbase.h"

class KeyboardEvent;

class OsuScreen : public CBaseUIContainer {
   public:
    OsuScreen() { m_bVisible = false; }

    virtual void onResolutionChange(Vector2 newResolution) { (void)newResolution; }
};
