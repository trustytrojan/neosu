//================ Copyright (c) 2014, PG, All rights reserved. =================//
//
// Purpose:		top level gamepad interface
//
// $NoKeywords: $gamepad
//===============================================================================//

#ifndef GAMEPAD_H
#define GAMEPAD_H

#include "GamepadButtons.h"
#include "GamepadListener.h"
#include "InputDevice.h"

class Gamepad : public InputDevice {
   public:
    Gamepad();
    virtual ~Gamepad() { ; }

    void addListener(GamepadListener *gamepadListener, bool insertOnTop = false);
    void removeListener(GamepadListener *gamepadListener);

    void onButtonDown(GAMEPADBUTTON b);
    void onButtonUp(GAMEPADBUTTON b);

    void onConnected();
    void onDisconnected();

    virtual Vector2 getLeftStick() = 0;
    virtual Vector2 getRightStick() = 0;

    virtual float getLeftTrigger() = 0;
    virtual float getRightTrigger() = 0;

    virtual bool isButtonPressed(GAMEPADBUTTON button) = 0;

    virtual bool isConnected() = 0;
    virtual int getPort() = 0;

    void setLeftStickDeadZone(float leftStickDeadZoneX, float leftStickDeadZoneY) {
        this->fLeftStickDeadZoneX = leftStickDeadZoneX;
        this->fLeftStickDeadZoneY = leftStickDeadZoneY;
    }
    void setLeftStickDeadZoneX(float leftStickDeadZoneX) { this->fLeftStickDeadZoneX = leftStickDeadZoneX; }
    void setLeftStickDeadZoneY(float leftStickDeadZoneY) { this->fLeftStickDeadZoneY = leftStickDeadZoneY; }
    void setRightStickDeadZone(float rightStickDeadZoneX, float rightStickDeadZoneY) {
        this->fRightStickDeadZoneX = rightStickDeadZoneX;
        this->fRightStickDeadZoneY = rightStickDeadZoneY;
    }
    void setRightStickDeadZoneX(float rightStickDeadZoneX) { this->fRightStickDeadZoneX = rightStickDeadZoneX; }
    void setRightStickDeadZoneY(float rightStickDeadZoneY) { this->fRightStickDeadZoneY = rightStickDeadZoneY; }

   protected:
    float fLeftStickDeadZoneX;
    float fLeftStickDeadZoneY;
    float fRightStickDeadZoneX;
    float fRightStickDeadZoneY;

   private:
    std::vector<GamepadListener *> listeners;
};

#endif
