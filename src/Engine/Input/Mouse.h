#pragma once
#include "Cursors.h"
#include "InputDevice.h"
#include "MouseListener.h"

class Mouse final : public InputDevice {
   public:
    Mouse();
    ~Mouse() override { ; }

    void draw() override;
    void drawDebug();
    void update() override;

    void addListener(MouseListener *mouseListener, bool insertOnTop = false);
    void removeListener(MouseListener *mouseListener);

    void resetWheelDelta();

    void onPosChange(Vector2d pos);
    void onRawMove(double xDelta, double yDelta, bool absolute = false, bool virtualDesktop = false);

    void onWheelVertical(int delta);
    void onWheelHorizontal(int delta);
    void onButtonChange(ButtonIndex button, bool down);

    void setPos(Vector2d pos);
    void setCursorType(CURSORTYPE cursorType);
    void setCursorVisible(bool cursorVisible);

    void setOffset(Vector2d offset) { this->vOffset = offset; }
    void setScale(Vector2d scale) { this->vScale = scale; }

    [[nodiscard]] inline Vector2d getPos() const { return this->vPos; }
    [[nodiscard]] inline Vector2d getDelta() const { return this->vDelta; }
    [[nodiscard]] inline Vector2d getRawDelta() const { return this->vRawDelta; }
    [[nodiscard]] inline Vector2d getOffset() const { return this->vOffset; }

    [[nodiscard]] inline bool isLeftDown() const {
        return bMouseButtonDownArray[static_cast<size_t>(ButtonIndex::BUTTON_LEFT)];
    }
    [[nodiscard]] inline bool isMiddleDown() const {
        return bMouseButtonDownArray[static_cast<size_t>(ButtonIndex::BUTTON_MIDDLE)];
    }
    [[nodiscard]] inline bool isRightDown() const {
        return bMouseButtonDownArray[static_cast<size_t>(ButtonIndex::BUTTON_RIGHT)];
    }
    [[nodiscard]] inline bool isButton4Down() const {
        return bMouseButtonDownArray[static_cast<size_t>(ButtonIndex::BUTTON_X1)];
    }
    [[nodiscard]] inline bool isButton5Down() const {
        return bMouseButtonDownArray[static_cast<size_t>(ButtonIndex::BUTTON_X2)];
    }

    bool isCursorVisible();

    [[nodiscard]] inline int getWheelDeltaVertical() const { return this->iWheelDeltaVertical; }
    [[nodiscard]] inline int getWheelDeltaHorizontal() const { return this->iWheelDeltaHorizontal; }

   private:
    Vector2d vPos;
    Vector2d vPosWithoutOffset;
    Vector2d vPrevOsMousePos;
    Vector2d vDelta;

    Vector2d vRawDelta;
    Vector2d vRawDeltaActual;
    Vector2d vRawDeltaAbsolute;
    Vector2d vRawDeltaAbsoluteActual;

    // button state (using our internal button index)
    std::array<bool, static_cast<size_t>(ButtonIndex::BUTTON_COUNT)> bMouseButtonDownArray{};

    int iWheelDeltaVertical;
    int iWheelDeltaHorizontal;
    int iWheelDeltaVerticalActual;
    int iWheelDeltaHorizontalActual;

    std::vector<MouseListener *> listeners;

    // custom
    bool bSetPosWasCalledLastFrame;
    bool bAbsolute;
    bool bVirtualDesktop;
    Vector2d vActualPos;
    Vector2d vOffset;
    Vector2d vScale;
    McRect desktopRect;
};
