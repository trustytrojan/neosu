//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		top level gamepad interface
//
// $NoKeywords: $gamepad
//===============================================================================//

#include "Gamepad.h"

#include "Engine.h"

Gamepad::Gamepad() : InputDevice() {
    this->fLeftStickDeadZoneX = this->fRightStickDeadZoneX = 0.28f;
    this->fLeftStickDeadZoneY = this->fRightStickDeadZoneY = 0.28f;
}

void Gamepad::addListener(GamepadListener *gamepadListener, bool insertOnTop) {
    if(gamepadListener == NULL) {
        engine->showMessageError("Gamepad Error", "addListener( NULL )!");
        return;
    }

    if(insertOnTop)
        this->listeners.insert(this->listeners.begin(), gamepadListener);
    else
        this->listeners.push_back(gamepadListener);
}

void Gamepad::removeListener(GamepadListener *gamepadListener) {
    for(size_t i = 0; i < this->listeners.size(); i++) {
        if(this->listeners[i] == gamepadListener) {
            this->listeners.erase(this->listeners.begin() + i);
            i--;
        }
    }
}

void Gamepad::onButtonDown(GAMEPADBUTTON b) {
    for(size_t i = 0; i < this->listeners.size(); i++) {
        this->listeners[i]->onButtonDown(b);
    }
}

void Gamepad::onButtonUp(GAMEPADBUTTON b) {
    for(size_t i = 0; i < this->listeners.size(); i++) {
        this->listeners[i]->onButtonUp(b);
    }
}

void Gamepad::onConnected() {
    for(size_t i = 0; i < this->listeners.size(); i++) {
        this->listeners[i]->onConnected();
    }
}

void Gamepad::onDisconnected() {
    for(size_t i = 0; i < this->listeners.size(); i++) {
        this->listeners[i]->onDisconnected();
    }
}
