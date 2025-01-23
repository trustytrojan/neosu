#include "XInputGamepad.h"

#include "Engine.h"

XInputGamepad::XInputGamepad() : Gamepad() {
    this->iPort = -1;

    this->fLeftTrigger = 0.0f;
    this->fRightTrigger = 0.0f;

    this->bDpadUpPressed = false;
    this->bDpadDownPressed = false;
    this->bDpadLeftPressed = false;
    this->bDpadRightPressed = false;
    this->bStartPressed = false;
    this->bBackPressed = false;
    this->bLeftThumbPressed = false;
    this->bRightThumbPressed = false;
    this->bLeftShoulderPressed = false;
    this->bRightShoulderPressed = false;
    this->bAPressed = false;
    this->bBPressed = false;
    this->bXPressed = false;
    this->bYPressed = false;
}

void XInputGamepad::update() {
#ifdef _WIN32
    if(!isConnected())
        updateConnection();
    else {
        memset(&this->state, 0, sizeof(XINPUT_STATE));
        if(XInputGetState(this->iPort, &this->state) != 0L) {
            // controller has been disconnected
            debugLog(0xffffff00, "XINPUTGAMEPAD: #%i disconnected\n", this->iPort);
            onDisconnected();
            this->iPort = -1;
            return;
        } else {
            // get fresh values from the controller

            // left stick
            float normLX = fmaxf(-1, (float)this->state.Gamepad.sThumbLX / 32767);
            float normLY = fmaxf(-1, (float)this->state.Gamepad.sThumbLY / 32767);

            this->vLeftStick.x = (std::abs(normLX) < this->fLeftStickDeadZoneX
                                      ? 0
                                      : (std::abs(normLX) - this->fLeftStickDeadZoneX) * (normLX / std::abs(normLX)));
            this->vLeftStick.y = (std::abs(normLY) < this->fLeftStickDeadZoneY
                                      ? 0
                                      : (std::abs(normLY) - this->fLeftStickDeadZoneY) * (normLY / std::abs(normLY)));

            if(this->fLeftStickDeadZoneX > 0) this->vLeftStick.x *= 1 / (1 - this->fLeftStickDeadZoneX);
            if(this->fLeftStickDeadZoneY > 0) this->vLeftStick.y *= 1 / (1 - this->fLeftStickDeadZoneY);

            // right stick
            float normRX = fmaxf(-1, (float)this->state.Gamepad.sThumbRX / 32767);
            float normRY = fmaxf(-1, (float)this->state.Gamepad.sThumbRY / 32767);

            this->vRightStick.x = (std::abs(normRX) < this->fRightStickDeadZoneX
                                       ? 0
                                       : (std::abs(normRX) - this->fRightStickDeadZoneX) * (normRX / std::abs(normRX)));
            this->vRightStick.y = (std::abs(normRY) < this->fRightStickDeadZoneY
                                       ? 0
                                       : (std::abs(normRY) - this->fRightStickDeadZoneY) * (normRY / std::abs(normRY)));

            if(this->fRightStickDeadZoneX > 0) this->vRightStick.x *= 1 / (1 - this->fRightStickDeadZoneX);
            if(this->fRightStickDeadZoneY > 0) this->vRightStick.y *= 1 / (1 - this->fRightStickDeadZoneY);

            // triggers
            this->fLeftTrigger = (float)this->state.Gamepad.bLeftTrigger / 255;
            this->fRightTrigger = (float)this->state.Gamepad.bRightTrigger / 255;

            // and all buttons
            updateButtonStates();
        }
    }
#endif
}

void XInputGamepad::setVibration(float leftMotorSpeedPercent, float rightMotorSpeedPercent) {
#ifdef _WIN32
    if(this->iPort != -1) {
        XINPUT_VIBRATION vibration;
        memset(&vibration, 0, sizeof(XINPUT_VIBRATION));
        {
            vibration.wLeftMotorSpeed = (WORD)(65535.0f * std::clamp<float>(leftMotorSpeedPercent, 0.0f, 1.0f));
            vibration.wRightMotorSpeed = (WORD)(65535.0f * std::clamp<float>(rightMotorSpeedPercent, 0.0f, 1.0f));
        }
        XInputSetState(this->iPort, &vibration);
    }
#else
    (void)leftMotorSpeedPercent;
    (void)rightMotorSpeedPercent;
#endif
}

void XInputGamepad::updateConnection() {
#ifdef _WIN32
    int controllerId = -1;

    // get first controller
    for(DWORD i = 0; i < XUSER_MAX_COUNT && controllerId == -1; i++) {
        XINPUT_STATE state;
        memset(&state, 0, sizeof(XINPUT_STATE));

        if(XInputGetState(i, &state) == 0L) controllerId = i;
    }

    if(controllerId != this->iPort && controllerId != -1) {
        debugLog(0xffffff00, "XINPUTGAMEPAD: #%i connected\n", controllerId);
        onConnected();
    }

    this->iPort = controllerId;
#endif
}

void XInputGamepad::updateButtonStates() {
#ifdef _WIN32
    const bool dpadUpPressed = (this->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
    const bool dpadDownPressed = (this->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
    const bool dpadLeftPressed = (this->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
    const bool dpadRightPressed = (this->state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
    const bool startPressed = (this->state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
    const bool backPressed = (this->state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
    const bool leftThumbPressed = (this->state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
    const bool rightThumbPressed = (this->state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
    const bool leftShoulderPressed = (this->state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
    const bool rightShoulderPressed = (this->state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
    const bool aPressed = (this->state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
    const bool bPressed = (this->state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
    const bool xPressed = (this->state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
    const bool yPressed = (this->state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;

    checkHandleButtonChangeEvents(this->bDpadUpPressed, dpadUpPressed, GAMEPAD_DPAD_UP);
    checkHandleButtonChangeEvents(this->bDpadDownPressed, dpadDownPressed, GAMEPAD_DPAD_DOWN);
    checkHandleButtonChangeEvents(this->bDpadLeftPressed, dpadLeftPressed, GAMEPAD_DPAD_LEFT);
    checkHandleButtonChangeEvents(this->bDpadRightPressed, dpadRightPressed, GAMEPAD_DPAD_RIGHT);
    checkHandleButtonChangeEvents(this->bStartPressed, startPressed, GAMEPAD_START);
    checkHandleButtonChangeEvents(this->bBackPressed, backPressed, GAMEPAD_BACK);
    checkHandleButtonChangeEvents(this->bLeftThumbPressed, leftThumbPressed, GAMEPAD_LEFT_THUMB);
    checkHandleButtonChangeEvents(this->bRightThumbPressed, rightThumbPressed, GAMEPAD_RIGHT_THUMB);
    checkHandleButtonChangeEvents(this->bRightShoulderPressed, rightShoulderPressed, GAMEPAD_RIGHT_SHOULDER);
    checkHandleButtonChangeEvents(this->bLeftShoulderPressed, leftShoulderPressed, GAMEPAD_RIGHT_SHOULDER);
    checkHandleButtonChangeEvents(this->bAPressed, aPressed, GAMEPAD_A);
    checkHandleButtonChangeEvents(this->bBPressed, bPressed, GAMEPAD_B);
    checkHandleButtonChangeEvents(this->bXPressed, xPressed, GAMEPAD_X);
    checkHandleButtonChangeEvents(this->bYPressed, yPressed, GAMEPAD_Y);
#endif
}

void XInputGamepad::checkHandleButtonChangeEvents(bool &previous, bool current, GAMEPADBUTTON b) {
    if(previous != current) {
        previous = current;
        if(current)
            this->onButtonDown(b);
        else
            this->onButtonUp(b);
    }
}

bool XInputGamepad::isButtonPressed(GAMEPADBUTTON button) {
#ifdef _WIN32
    switch(button) {
        // DPAD
        case GAMEPAD_DPAD_UP:
            return this->bDpadUpPressed;
            break;
        case GAMEPAD_DPAD_DOWN:
            return this->bDpadDownPressed;
            break;
        case GAMEPAD_DPAD_LEFT:
            return this->bDpadLeftPressed;
            break;
        case GAMEPAD_DPAD_RIGHT:
            return this->bDpadRightPressed;
            break;

        // START/SELECT
        case GAMEPAD_START:
            return this->bStartPressed;
            break;
        case GAMEPAD_BACK:
            return this->bBackPressed;
            break;

        // STICKS
        case GAMEPAD_LEFT_THUMB:
            return this->bLeftThumbPressed;
            break;
        case GAMEPAD_RIGHT_THUMB:
            return this->bRightThumbPressed;
            break;

        // SHOULDER
        case GAMEPAD_LEFT_SHOULDER:
            return this->bLeftShoulderPressed;
            break;
        case GAMEPAD_RIGHT_SHOULDER:
            return this->bRightShoulderPressed;
            break;

        // A/B/X/Y
        case GAMEPAD_A:
            return this->bAPressed;
            break;
        case GAMEPAD_B:
            return this->bBPressed;
            break;
        case GAMEPAD_X:
            return this->bXPressed;
            break;
        case GAMEPAD_Y:
            return this->bYPressed;
            break;

        default:
            return false;
    }
#else
    (void)button;
    return false;
#endif
}
