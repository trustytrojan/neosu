#pragma once
#include "Cursors.h"
#include "InputDevice.h"
#include "MouseListener.h"

class Mouse : public InputDevice {
   public:
    Mouse();
    virtual ~Mouse() { ; }

    virtual void draw(Graphics *g);
    void drawDebug(Graphics *g);
    virtual void update();

    void addListener(MouseListener *mouseListener, bool insertOnTop = false);
    void removeListener(MouseListener *mouseListener);

    void resetWheelDelta();

    void onPosChange(Vector2 pos);
    void onRawMove(int xDelta, int yDelta, bool absolute = false, bool virtualDesktop = false);

    void onWheelVertical(int delta);
    void onWheelHorizontal(int delta);

    void onLeftChange(bool leftDown);
    void onMiddleChange(bool middleDown);
    void onRightChange(bool rightDown);
    void onButton4Change(bool button4down);
    void onButton5Change(bool button5down);

    void setPos(Vector2 pos);
    void setCursorType(CURSORTYPE cursorType);
    void setCursorVisible(bool cursorVisible);

    void setOffset(Vector2 offset) { this->vOffset = offset; }
    void setScale(Vector2 scale) { this->vScale = scale; }

    inline Vector2 getPos() const { return this->vPos; }
    inline Vector2 getDelta() const { return this->vDelta; }
    inline Vector2 getRawDelta() const { return this->vRawDelta; }
    inline Vector2 getOffset() const { return this->vOffset; }

    inline bool isLeftDown() const { return this->bMouseLeftDown; }
    inline bool isMiddleDown() const { return this->bMouseMiddleDown; }
    inline bool isRightDown() const { return this->bMouseRightDown; }
    inline bool isButton4Down() const { return this->bMouse4Down; }
    inline bool isButton5Down() const { return this->bMouse5Down; }

    bool isCursorVisible();

    inline int getWheelDeltaVertical() const { return this->iWheelDeltaVertical; }
    inline int getWheelDeltaHorizontal() const { return this->iWheelDeltaHorizontal; }

   private:
    void setPosXY(float x, float y);  // shit hack

    Vector2 vPos;
    Vector2 vPosWithoutOffset;
    Vector2 vPrevOsMousePos;
    Vector2 vDelta;

    Vector2 vRawDelta;
    Vector2 vRawDeltaActual;
    Vector2 vRawDeltaAbsolute;
    Vector2 vRawDeltaAbsoluteActual;

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
    Vector2 vActualPos;
    Vector2 vOffset;
    Vector2 vScale;
    McRect desktopRect;

    struct FAKELAG_PACKET {
        float time;
        Vector2 pos;
    };
    std::vector<FAKELAG_PACKET> fakelagBuffer;
    Vector2 vFakeLagPos;
};
