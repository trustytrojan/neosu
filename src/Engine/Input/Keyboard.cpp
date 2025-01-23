//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		state class and listeners (FIFO)
//
// $NoKeywords: $key
//===============================================================================//

#include "Keyboard.h"

#include "Engine.h"

Keyboard::Keyboard() : InputDevice() {
    this->bControlDown = false;
    this->bAltDown = false;
    this->bShiftDown = false;
    this->bSuperDown = false;
}

void Keyboard::addListener(KeyboardListener *keyboardListener, bool insertOnTop) {
    if(keyboardListener == NULL) {
        engine->showMessageError("Keyboard Error", "addListener(NULL)!");
        return;
    }

    if(insertOnTop)
        this->listeners.insert(this->listeners.begin(), keyboardListener);
    else
        this->listeners.push_back(keyboardListener);
}

void Keyboard::removeListener(KeyboardListener *keyboardListener) {
    for(size_t i = 0; i < this->listeners.size(); i++) {
        if(this->listeners[i] == keyboardListener) {
            this->listeners.erase(this->listeners.begin() + i);
            i--;
        }
    }
}

void Keyboard::reset() {
    this->bControlDown = false;
    this->bAltDown = false;
    this->bShiftDown = false;
    this->bSuperDown = false;
}

void Keyboard::onKeyDown(KEYCODE keyCode) {
    switch(keyCode) {
        case KEY_CONTROL:
            this->bControlDown = true;
            break;
        case 65511:  // linux
        case KEY_ALT:
            this->bAltDown = true;
            break;
        case KEY_SHIFT:
            this->bShiftDown = true;
            break;
        case KEY_SUPER:
            this->bSuperDown = true;
            break;
    }

    KeyboardEvent e(keyCode);

    for(size_t i = 0; i < this->listeners.size(); i++) {
        this->listeners[i]->onKeyDown(e);
        if(e.isConsumed()) break;
    }
}

void Keyboard::onKeyUp(KEYCODE keyCode) {
    switch(keyCode) {
        case KEY_CONTROL:
            this->bControlDown = false;
            break;
        case 65511:  // linux
        case KEY_ALT:
            this->bAltDown = false;
            break;
        case KEY_SHIFT:
            this->bShiftDown = false;
            break;
        case KEY_SUPER:
            this->bSuperDown = false;
            break;
    }

    KeyboardEvent e(keyCode);

    for(size_t i = 0; i < this->listeners.size(); i++) {
        this->listeners[i]->onKeyUp(e);
        if(e.isConsumed()) break;
    }
}

void Keyboard::onChar(KEYCODE charCode) {
    KeyboardEvent e(charCode);

    for(size_t i = 0; i < this->listeners.size(); i++) {
        this->listeners[i]->onChar(e);
        if(e.isConsumed()) break;
    }
}
