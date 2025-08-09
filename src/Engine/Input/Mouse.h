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
    // callback for SDL's raw mouse motion (see Environment.cpp)
    static void raw_motion_cb(void *userdata, uint64_t ts, SDL_Window *window, uint32_t mouseid, float *x, float *y);

   private:
    // helper struct for the rawMotionCB, per-mouse
    struct MotionCBData {
        MotionCBData() = default;
        ~MotionCBData() = default;

        MotionCBData &operator=(const MotionCBData &) = delete;
        MotionCBData &operator=(MotionCBData &&) = delete;
        MotionCBData(const MotionCBData &) = delete;
        MotionCBData(MotionCBData &&) = delete;

        // accumulate
        // += to handle overflowing/wrapping the array, which could happen if there's a lagspike
        inline void add(float *x, float *y) {
            this->count++;
            this->accumulated_motion += Vector2d{*x, *y};
        }

        // release (during Mouse::update())
        Vector2 consume();

        // get
        [[nodiscard]] inline double get_current_sens() const { return this->current_sensitivity; }

        // set (by Mouse)
        inline void set_current_sens(float sens) { this->current_sensitivity = sens; }

        // debugging
        [[nodiscard]] inline u16 get_count() const { return this->count; }

       private:
        // use doubles to maintain precision until the final cast back to floats (avoid drift)
        Vector2d accumulated_motion{};
        double current_sensitivity{1.};
        u16 count{0};  // event counter until the data has been consumed
    };

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
    void onMotion(Vector2 rel, Vector2 abs, bool preTransformed);
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

    // input mode control
    [[nodiscard]] inline const bool &isInAbsoluteMode() const { return this->bNeedsCursorLock; }
    inline void setAbsoluteMode(bool absolute) { this->bNeedsCursorLock = absolute; }

    [[nodiscard]] inline const bool &isRawInput() const {
        return this->bIsRawInput;
    }  // "desired" rawinput state, NOT actual OS raw input state!

    [[nodiscard]] inline const MotionCBData *getMotionCallbackData() const { return &this->mcbdata; }

   private:
    // callbacks
    void onSensitivityChanged(float newSens);
    void onRawInputChanged(float newVal);

    void raw_update_tick();

    // position state
    Vector2 vPos;                // position with offset applied
    Vector2 vPosWithoutOffsets;  // position without offset
    Vector2 vDelta{};            // movement delta in the current frame
    Vector2 vRawDelta{};  // movement delta in the current frame, without consideration for clipping or sensitivity
    Vector2 vActualPos;   // final cursor position after all transformations

    // mode tracking
    bool bLastFrameHadMotion{false};  // whether setPos was called in the previous frame
    bool bNeedsCursorLock{false};   // whether we need to manually center the cursor to prevent it from leaving/hitting the
                              // edges of the window (when raw input is off and sensitivity < 1)
    bool bIsRawInput{false};  // whether raw input is active
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

    MotionCBData mcbdata;
};

#endif
