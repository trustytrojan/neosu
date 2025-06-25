#pragma once
#include "Gamepad.h"

#ifdef _WIN32
#include <xinput.h>
#endif

class XInputGamepad : public Gamepad {
   public:
    XInputGamepad();
    ~XInputGamepad() override { ; }

    void update() override;

    void setVibration(float leftMotorSpeedPercent, float rightMotorSpeedPercent);

    inline Vector2 getLeftStick() override { return this->vLeftStick; }
    inline Vector2 getRightStick() override { return this->vRightStick; }

    inline float getLeftTrigger() override { return this->fLeftTrigger; }
    inline float getRightTrigger() override { return this->fRightTrigger; }

    bool isButtonPressed(GAMEPADBUTTON button) override;

    inline bool isConnected() override { return this->iPort != -1; }
    inline int getPort() override { return this->iPort + 1; }

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
