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

    void setOffset(Vector2d offset) { this->vOffset = offset; }
    void setScale(Vector2d scale) { this->vScale = scale; }

    [[nodiscard]] constexpr Vector2d getPos() const { return this->vPos; }
    [[nodiscard]] constexpr Vector2d getDelta() const { return this->vDelta; }
    [[nodiscard]] constexpr Vector2d getRawDelta() const { return this->vRawDelta; }
    [[nodiscard]] constexpr Vector2d getOffset() const { return this->vOffset; }

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

    int iWheelDeltaVertical{0};
    int iWheelDeltaHorizontal{0};
    int iWheelDeltaVerticalActual{0};
    int iWheelDeltaHorizontalActual{0};

    std::vector<MouseListener *> listeners;

    // custom
    bool bSetPosWasCalledLastFrame{false};
    bool bAbsolute{false};
    bool bVirtualDesktop{false};
    Vector2d vActualPos;
    Vector2d vOffset{0, 0};
    Vector2d vScale{1, 1};
    McRect desktopRect;
};
