#pragma once
#include "Cursors.h"
#include "InputDevice.h"
#include "MouseListener.h"

class Mouse : public InputDevice {
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

    void onLeftChange(bool leftDown);
    void onMiddleChange(bool middleDown);
    void onRightChange(bool rightDown);
    void onButton4Change(bool button4down);
    void onButton5Change(bool button5down);

    void setPos(Vector2d pos);
    void setCursorType(CURSORTYPE cursorType);
    void setCursorVisible(bool cursorVisible);

    void setOffset(Vector2d offset) { this->vOffset = offset; }
    void setScale(Vector2d scale) { this->vScale = scale; }

    [[nodiscard]] inline Vector2d getPos() const { return this->vPos; }
    [[nodiscard]] inline Vector2d getDelta() const { return this->vDelta; }
    [[nodiscard]] inline Vector2d getRawDelta() const { return this->vRawDelta; }
    [[nodiscard]] inline Vector2d getOffset() const { return this->vOffset; }

    [[nodiscard]] inline bool isLeftDown() const { return this->bMouseLeftDown; }
    [[nodiscard]] inline bool isMiddleDown() const { return this->bMouseMiddleDown; }
    [[nodiscard]] inline bool isRightDown() const { return this->bMouseRightDown; }
    [[nodiscard]] inline bool isButton4Down() const { return this->bMouse4Down; }
    [[nodiscard]] inline bool isButton5Down() const { return this->bMouse5Down; }

    bool isCursorVisible();

    [[nodiscard]] inline int getWheelDeltaVertical() const { return this->iWheelDeltaVertical; }
    [[nodiscard]] inline int getWheelDeltaHorizontal() const { return this->iWheelDeltaHorizontal; }

   private:
    void setPosXY(double x, double y);  // shit hack

    Vector2d vPos;
    Vector2d vPosWithoutOffset;
    Vector2d vPrevOsMousePos;
    Vector2d vDelta;

    Vector2d vRawDelta;
    Vector2d vRawDeltaActual;
    Vector2d vRawDeltaAbsolute;
    Vector2d vRawDeltaAbsoluteActual;

    bool bMouseLeftDown;
    bool bMouseMiddleDown;
    bool bMouseRightDown;
    bool bMouse4Down;
    bool bMouse5Down;

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

    struct FAKELAG_PACKET {
        float time;
        Vector2 pos;
    };
    std::vector<FAKELAG_PACKET> fakelagBuffer;
    Vector2 vFakeLagPos;
};
