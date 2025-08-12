// Copyright (c) 2015, PG & 2025, WH, All rights reserved.
#pragma once
#ifndef MOUSE_H
#define MOUSE_H

#include "Cursors.h"
#include "InputDevice.h"
#include "MouseListener.h"

#include <array>

class Mouse final : public InputDevice {
   public:
    Mouse();
    ~Mouse() override { ; }

    void draw() override;
    void update() override;

    void drawDebug();

    // event handling
    void addListener(MouseListener *mouseListener, bool insertOnTop = false);
    void removeListener(MouseListener *mouseListener);

    // input handling
    void onPosChange(Vector2 pos);
    void onWheelVertical(int delta);
    void onWheelHorizontal(int delta);
    void onButtonChange(ButtonIndex button, bool down);

    // position/coordinate handling
    void setPos(Vector2 pos);
    void setOffset(Vector2 offset);
    void setScale(Vector2 scale) { this->vScale = scale; }

    // state getters
    [[nodiscard]] inline const Vector2 &getPos() const { return this->vPos; }
    [[nodiscard]] inline const Vector2 &getRealPos() const { return this->vPosWithoutOffsets; }
    [[nodiscard]] inline const Vector2 &getActualPos() const { return this->vActualPos; }
    [[nodiscard]] inline const Vector2 &getDelta() const { return this->vDelta; }
    [[nodiscard]] inline const Vector2 &getRawDelta() const { return this->vRawDelta; }

    [[nodiscard]] inline const Vector2 &getOffset() const { return this->vOffset; }
    [[nodiscard]] inline const Vector2 &getScale() const { return this->vScale; }
    [[nodiscard]] inline const float &getSensitivity() const { return this->fSensitivity; }

    // button state accessors
    [[nodiscard]] constexpr bool isLeftDown() const {
        return this->bMouseButtonDownArray[static_cast<size_t>(ButtonIndex::BUTTON_LEFT)];
    }
    [[nodiscard]] constexpr bool isMiddleDown() const {
        return this->bMouseButtonDownArray[static_cast<size_t>(ButtonIndex::BUTTON_MIDDLE)];
    }
    [[nodiscard]] constexpr bool isRightDown() const {
        return this->bMouseButtonDownArray[static_cast<size_t>(ButtonIndex::BUTTON_RIGHT)];
    }
    [[nodiscard]] constexpr bool isButton4Down() const {
        return this->bMouseButtonDownArray[static_cast<size_t>(ButtonIndex::BUTTON_X1)];
    }
    [[nodiscard]] constexpr bool isButton5Down() const {
        return this->bMouseButtonDownArray[static_cast<size_t>(ButtonIndex::BUTTON_X2)];
    }

    [[nodiscard]] inline const int &getWheelDeltaVertical() const { return this->iWheelDeltaVertical; }
    [[nodiscard]] inline const int &getWheelDeltaHorizontal() const { return this->iWheelDeltaHorizontal; }

    void resetWheelDelta();

    [[nodiscard]] inline const bool &isRawInputWanted() const {
        return this->bIsRawInputDesired;
    }  // "desired" rawinput state, NOT actual OS raw input state!

   private:
    // callbacks
    void onSensitivityChanged(float newSens);
    void onRawInputChanged(float newVal);

    // position state
    Vector2 vPos;                // position with offset applied
    Vector2 vPosWithoutOffsets;  // position without offset
    Vector2 vDelta{};            // movement delta in the current frame
    Vector2 vRawDelta{};  // movement delta in the current frame, without consideration for clipping or sensitivity
    Vector2 vActualPos;   // final cursor position after all transformations

    // mode tracking
    bool bIsRawInputDesired{false};  // whether the user wants raw (relative) input
    float fSensitivity{1.0f};

    // button state (using our internal button index)
    std::array<bool, static_cast<size_t>(ButtonIndex::BUTTON_COUNT)> bMouseButtonDownArray{};

    // wheel state
    int iWheelDeltaVertical{0};
    int iWheelDeltaHorizontal{0};
    int iWheelDeltaVerticalActual{0};
    int iWheelDeltaHorizontalActual{0};

    // listeners
    std::vector<MouseListener *> listeners;

    // transform parameters
    Vector2 vOffset{0, 0};  // offset applied to coordinates
    Vector2 vScale{1, 1};   // scale applied to coordinates
};

#endif
