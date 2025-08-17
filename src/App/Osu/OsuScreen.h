#pragma once
// Copyright (c) 2016, PG, All rights reserved.
#include "CBaseUIContainer.h"

class KeyboardEvent;

class OsuScreen : public CBaseUIContainer {
   public:
    OsuScreen() { this->bVisible = false; }

    virtual void onResolutionChange(vec2 newResolution) { (void)newResolution; }
};
