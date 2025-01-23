#pragma once
#include "Gamepad.h"

#ifdef _WIN32
#include <xinput.h>
#endif

class XInputGamepad : public Gamepad {
   public:
    XInputGamepad();
    virtual ~XInputGamepad() { ; }

    void update();

    void setVibration(float leftMotorSpeedPercent, float rightMotorSpeedPercent);

    inline Vector2 getLeftStick() { return this->vLeftStick; }
    inline Vector2 getRightStick() { return this->vRightStick; }

    inline float getLeftTrigger() { return this->fLeftTrigger; }
    inline float getRightTrigger() { return this->fRightTrigger; }

    bool isButtonPressed(GAMEPADBUTTON button);

    inline bool isConnected() { return this->iPort != -1; }
    inline int getPort() { return this->iPort + 1; }

   private:
    void updateConnection();
    void updateButtonStates();
    void checkHandleButtonChangeEvents(bool &previous, bool current, GAMEPADBUTTON b);

#ifdef _WIN32
    XINPUT_STATE state;
#endif

    int iPort;

    Vector2 vLeftStick;
    Vector2 vRightStick;

    float fLeftTrigger;
    float fRightTrigger;

    bool bDpadUpPressed;
    bool bDpadDownPressed;
    bool bDpadLeftPressed;
    bool bDpadRightPressed;
    bool bStartPressed;
    bool bBackPressed;
    bool bLeftThumbPressed;
    bool bRightThumbPressed;
    bool bLeftShoulderPressed;
    bool bRightShoulderPressed;
    bool bAPressed;
    bool bBPressed;
    bool bXPressed;
    bool bYPressed;
};
