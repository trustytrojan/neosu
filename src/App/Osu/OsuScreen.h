#pragma once
#include "CBaseUIContainer.h"

class KeyboardEvent;

class OsuScreen : public CBaseUIContainer {
   public:
    OsuScreen() { this->bVisible = false; }

    virtual void onResolutionChange(Vector2 newResolution) { (void)newResolution; }
};
