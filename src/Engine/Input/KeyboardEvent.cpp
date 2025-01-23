//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		a wrapper for consumable keyboard events
//
// $NoKeywords: $key
//===============================================================================//

#include "KeyboardEvent.h"

KeyboardEvent::KeyboardEvent(KEYCODE keyCode) {
    this->keyCode = keyCode;
    this->bConsumed = false;
}

void KeyboardEvent::consume() { this->bConsumed = true; }
