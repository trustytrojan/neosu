#include "Mouse.h"

#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"
#include "ResourceManager.h"

Mouse::Mouse() : InputDevice() {
    this->bMouseLeftDown = false;
    this->bMouseMiddleDown = false;
    this->bMouseRightDown = false;
    this->bMouse4Down = false;
    this->bMouse5Down = false;

    this->iWheelDeltaVertical = 0;
    this->iWheelDeltaHorizontal = 0;
    this->iWheelDeltaVerticalActual = 0;
    this->iWheelDeltaHorizontalActual = 0;

    this->bSetPosWasCalledLastFrame = false;
    this->bAbsolute = false;
    this->bVirtualDesktop = false;
    this->vOffset = Vector2d(0, 0);
    this->vScale = Vector2d(1, 1);
    this->vActualPos = this->vPosWithoutOffset = this->vPos = env->getMousePos();
    this->desktopRect = env->getDesktopRect();
}

void Mouse::draw() {
    if(!cv_debug_mouse.getBool()) return;

    this->drawDebug();

    // green rect = virtual cursor pos
    g->setColor(0xff00ff00);
    float size = 20.0f;
    g->drawRect(this->vActualPos.x - size / 2, this->vActualPos.y - size / 2, size, size);

    // red rect = real cursor pos
    g->setColor(0xffff0000);
    Vector2d envPos = env->getMousePos();
    g->drawRect(envPos.x - size / 2, envPos.y - size / 2, size, size);

    // red = cursor clip
    if(env->isCursorClipped()) {
        McRect cursorClip = env->getCursorClip();
        g->drawRect(cursorClip.getMinX(), cursorClip.getMinY(), cursorClip.getWidth() - 1, cursorClip.getHeight() - 1);
    }

    // green = scaled & offset virtual area
    const Vector2d scaledOffset = this->vOffset;
    const Vector2d scaledEngineScreenSize = engine->getScreenSize() * this->vScale;
    g->setColor(0xff00ff00);
    g->drawRect(-scaledOffset.x, -scaledOffset.y, scaledEngineScreenSize.x, scaledEngineScreenSize.y);
}

void Mouse::drawDebug() {
    Vector2 pos = this->getPos();

    g->setColor(0xff000000);
    g->drawLine(pos.x - 1, pos.y - 1, 0 - 1, pos.y - 1);
    g->drawLine(pos.x - 1, pos.y - 1, engine->getScreenWidth() - 1, pos.y - 1);
    g->drawLine(pos.x - 1, pos.y - 1, pos.x - 1, 0 - 1);
    g->drawLine(pos.x - 1, pos.y - 1, pos.x - 1, engine->getScreenHeight() - 1);

    g->setColor(0xffffffff);
    g->drawLine(pos.x, pos.y, 0, pos.y);
    g->drawLine(pos.x, pos.y, engine->getScreenWidth(), pos.y);
    g->drawLine(pos.x, pos.y, pos.x, 0);
    g->drawLine(pos.x, pos.y, pos.x, engine->getScreenHeight());

    float rectSizePercent = 0.05f;
    float aspectRatio = (float)engine->getScreenWidth() / (float)engine->getScreenHeight();
    Vector2 rectSize = Vector2(engine->getScreenWidth(), engine->getScreenHeight() * aspectRatio) * rectSizePercent;

    g->setColor(0xff000000);
    g->drawRect(pos.x - rectSize.x / 2.0f - 1, pos.y - rectSize.y / 2.0f - 1, rectSize.x, rectSize.y);

    g->setColor(0xffffffff);
    g->drawRect(pos.x - rectSize.x / 2.0f, pos.y - rectSize.y / 2.0f, rectSize.x, rectSize.y);

    McFont *posFont = resourceManager->getFont("FONT_DEFAULT");
    UString posString = UString::format("[%i, %i]", (int)pos.x, (int)pos.y);
    float stringWidth = posFont->getStringWidth(posString);
    float stringHeight = posFont->getHeight();
    Vector2 textOffset = Vector2(
        pos.x + rectSize.x / 2.0f + stringWidth + 5 > engine->getScreenWidth() ? -rectSize.x / 2.0f - stringWidth - 5
                                                                               : rectSize.x / 2.0f + 5,
        (pos.y + rectSize.y / 2.0f + stringHeight > engine->getScreenHeight()) ? -rectSize.y / 2.0f - stringHeight
                                                                               : rectSize.y / 2.0f + stringHeight);

    g->pushTransform();
    g->translate(pos.x + textOffset.x, pos.y + textOffset.y);
    g->drawString(posFont, UString::format("[%i, %i]", (int)pos.x, (int)pos.y));
    g->popTransform();
}

void Mouse::update() {
    this->vDelta.zero();

    this->vRawDelta = this->vRawDeltaActual;
    this->vRawDeltaActual.zero();

    this->vRawDeltaAbsolute = this->vRawDeltaAbsoluteActual;  // don't zero an absolute!

    this->resetWheelDelta();

    // TODO: clean up OS specific handling, specifically all the linux blocks

    // if the operating system cursor is potentially being used or visible in any way, do not interfere with it!
    // (sensitivity, setCursorPos(), etc.) same goes for a sensitivity of 1 without raw input, it is not necessary to
    // call env->setPos() in that case

    const float sens = cv_mouse_sensitivity.getFloat();
    const bool isRaw = cv_mouse_raw_input.getBool();

    const McRect windowRect{0, 0, static_cast<float>(engine->getScreenWidth()),
                            static_cast<float>(engine->getScreenHeight())};
    const bool osCursorVisible = env->isCursorVisible() || !env->isCursorInWindow() || !engine->hasFocus();
    const bool sensitivityAdjustmentNeeded = sens < 0.999f || sens > 1.001f;

    const Vector2d osMousePos = env->getMousePos();

    Vector2d nextPos = osMousePos;

    if(osCursorVisible || (!sensitivityAdjustmentNeeded && !isRaw) || this->bAbsolute) {
        // this block handles visible/active OS cursor movement without sensitivity adjustments, and absolute input
        // device movement
        if(!Env::cfg(OS::LINUX) && this->bAbsolute && !cv_tablet_sensitivity_ignore.getBool()) {
            // absolute input (with sensitivity)
            // NOTE: these range values work on windows only!
            // TODO: standardize the input values before they even reach the engine, this should not be in here
            double rawRangeX = 65536;  // absolute coord range, but what if I want to have a tablet that's more
                                      // accurate than 1/65536-th? >:(
            double rawRangeY = 65536;

            // if enabled, uses the screen resolution as the coord range, instead of 65536
            if(cv_win_ink_workaround.getBool()) {
                rawRangeX = env->getNativeScreenSize().x;
                rawRangeY = env->getNativeScreenSize().y;
            }

            // NOTE: mouse_raw_input_absolute_to_window only applies if raw input is enabled in general
            if(isRaw && cv_mouse_raw_input_absolute_to_window.getBool()) {
                const Vector2d scaledOffset = this->vOffset;
                const Vector2d scaledEngineScreenSize = engine->getScreenSize() + 2 * scaledOffset;

                nextPos.x =
                    (((double)((this->vRawDeltaAbsolute.x - rawRangeX / 2) * sens) + rawRangeX / 2) / rawRangeX) *
                        scaledEngineScreenSize.x -
                    scaledOffset.x;
                nextPos.y =
                    (((double)((this->vRawDeltaAbsolute.y - rawRangeY / 2) * sens) + rawRangeY / 2) / rawRangeY) *
                        scaledEngineScreenSize.y -
                    scaledOffset.y;
            } else {
                // shift and scale to desktop
                const McRect screen{this->bVirtualDesktop ? env->getVirtualScreenRect() : this->desktopRect};
                const Vector2d posInScreenCoords{
                    (this->vRawDeltaAbsolute.x / rawRangeX) * screen.getWidth() + screen.getX(),
                    (this->vRawDeltaAbsolute.y / rawRangeY) * screen.getHeight() + screen.getY()};

                // offset to window
                nextPos = posInScreenCoords - env->getWindowPos();

                // apply sensitivity, scale and offset to engine
                nextPos.x = ((nextPos.x - engine->getScreenSize().x / 2) * sens + engine->getScreenSize().x / 2);
                nextPos.y = ((nextPos.y - engine->getScreenSize().y / 2) * sens + engine->getScreenSize().y / 2);
            }
        } else if(!this->bAbsolute) {
            // relative input (without sensitivity)
            // (nothing to do here except updating the delta, since nextPos is already set to env->getMousePos() by
            // default)
            this->vDelta = osMousePos - this->vPrevOsMousePos;
        }
    } else {
        // this block handles relative input with sensitivity adjustments, either raw or non-raw

        // calculate delta (either raw, or non-raw)
        if(isRaw)
            this->vDelta = this->vRawDelta;  // this is already scaled to the sensitivity
        else {
            // non-raw input is always in pixels, sub-pixel movement is handled/buffered by the operating system
            // without this check some people would get mouse drift
            if((int)std::round(osMousePos.x) != (int)std::round(this->vPrevOsMousePos.x) ||
               (int)std::round(osMousePos.y) != (int)std::round(this->vPrevOsMousePos.y))
                this->vDelta = (osMousePos - this->vPrevOsMousePos) * sens;
        }

        nextPos = this->vPosWithoutOffset + this->vDelta;

        // special case: relative input is ALWAYS clipped/confined to the window
        nextPos.x = std::clamp<double>(nextPos.x, windowRect.getMinX(), windowRect.getMaxX());
        nextPos.y = std::clamp<double>(nextPos.y, windowRect.getMinY(), windowRect.getMaxY());
    }

    // clip/confine cursor
    if(env->isCursorClipped()) {
        const McRect cliprect = env->getCursorClip();
        nextPos.x = std::clamp<double>(nextPos.x, cliprect.getMinX() + 1, cliprect.getMaxX() - 1);
        nextPos.y = std::clamp<double>(nextPos.y, cliprect.getMinY() + 1, cliprect.getMaxY() - 1);
    }

    // set new virtual cursor position (this applies the offset as well)
    this->onPosChange(nextPos);

    // set new os cursor position, but only if the osMousePos is still within the window and the sensitivity is not 1;
    // raw input ALWAYS needs env->setPos()
    // first person games which call mouse->setPos() every frame to manually re-center the cursor NEVER
    // need env->setPos() absolute input NEVER needs env->setPos() also update prevOsMousePos
    if(windowRect.contains(osMousePos) && !osCursorVisible && (sensitivityAdjustmentNeeded || isRaw) &&
       !this->bSetPosWasCalledLastFrame && !this->bAbsolute) {
        const Vector2d newOsMousePos{this->vPosWithoutOffset};

        env->setMousePos(newOsMousePos.x, newOsMousePos.y);

        // assume that the operating system has set the cursor to nextPos quickly enough for the next frame
        if constexpr(!Env::cfg(OS::LINUX)) {
            this->vPrevOsMousePos.x = std::round(newOsMousePos.x);
            this->vPrevOsMousePos.y = std::round(newOsMousePos.y);
        } else {
            this->vPrevOsMousePos = newOsMousePos;
        }

        // 3 cases can happen in the next frame:
        // 1) the operating system did not update the cursor quickly enough. osMousePos != nextPos
        // 2) the operating system did update the cursor quickly enough, but the user moved the mouse in the meantime.
        // osMousePos != nextPos 3) the operating system did update the cursor quickly enough, and the user did not move
        // the mouse in the meantime. osMousePos == nextPos

        // it's impossible to determine if osMousePos != nextPos was caused by the user moving the mouse, or by the
        // operating system not updating the cursor position quickly enough

        // 2 cases can happen after trusting the operating system to apply nextPos correctly:
        // 1) delta is applied twice, due to the operating system not updating quickly enough, cursor jumps too far
        // 2) delta is applied once, cursor works as expected

        // all of this shit can be avoided by just enabling mouse_raw_input
    } else
        this->vPrevOsMousePos = osMousePos;

    this->bSetPosWasCalledLastFrame = false;
}

void Mouse::addListener(MouseListener *mouseListener, bool insertOnTop) {
    if(mouseListener == NULL) {
        engine->showMessageError("Mouse Error", "addListener(NULL)!");
        return;
    }

    if(insertOnTop)
        this->listeners.insert(this->listeners.begin(), mouseListener);
    else
        this->listeners.push_back(mouseListener);
}

void Mouse::removeListener(MouseListener *mouseListener) {
    for(size_t i = 0; i < this->listeners.size(); i++) {
        if(this->listeners[i] == mouseListener) {
            this->listeners.erase(this->listeners.begin() + i);
            i--;
        }
    }
}

void Mouse::resetWheelDelta() {
    this->iWheelDeltaVertical = this->iWheelDeltaVerticalActual;
    this->iWheelDeltaVerticalActual = 0;

    this->iWheelDeltaHorizontal = this->iWheelDeltaHorizontalActual;
    this->iWheelDeltaHorizontalActual = 0;
}

void Mouse::onPosChange(Vector2d pos) {
    this->vPos = (this->vOffset + pos);
    this->vPosWithoutOffset = pos;

    this->vActualPos = this->vPos;

    this->setPosXY(this->vPos.x, this->vPos.y);
}

void Mouse::setPosXY(double x, double y) {
    if(cv_mouse_fakelag.getFloat() > 0.0f) {
        FAKELAG_PACKET p;
        p.time = engine->getTime() + cv_mouse_fakelag.getFloat();
        p.pos = Vector2d(x, y);
        this->fakelagBuffer.push_back(p);

        float engineTime = engine->getTime();
        for(size_t i = 0; i < this->fakelagBuffer.size(); i++) {
            if(engineTime >= this->fakelagBuffer[i].time) {
                this->vFakeLagPos = this->fakelagBuffer[i].pos;

                this->fakelagBuffer.erase(this->fakelagBuffer.begin() + i);
                i--;
            }
        }
        this->vPos = this->vFakeLagPos;
    } else {
        this->vPos.x = x;
        this->vPos.y = y;
    }
}

void Mouse::onRawMove(double xDelta, double yDelta, bool absolute, bool virtualDesktop) {
    this->bAbsolute = absolute;
    this->bVirtualDesktop = virtualDesktop;

    if(xDelta != 0 || yDelta != 0)  // sanity check, else some people get mouse drift like above, I don't even
    {
        if(!this->bAbsolute)  // mouse
            this->vRawDeltaActual += Vector2d(xDelta, yDelta) * cv_mouse_sensitivity.getFloat();
        else  // tablet
            this->vRawDeltaAbsoluteActual = Vector2d(xDelta, yDelta);
    }
}

void Mouse::onWheelVertical(int delta) {
    this->iWheelDeltaVerticalActual += delta;

    for(size_t i = 0; i < this->listeners.size(); i++) {
        this->listeners[i]->onWheelVertical(delta);
    }
}

void Mouse::onWheelHorizontal(int delta) {
    this->iWheelDeltaHorizontalActual += delta;

    for(size_t i = 0; i < this->listeners.size(); i++) {
        this->listeners[i]->onWheelHorizontal(delta);
    }
}

void Mouse::onLeftChange(bool leftDown) {
    this->bMouseLeftDown = leftDown;

    for(size_t i = 0; i < this->listeners.size(); i++) {
        this->listeners[i]->onLeftChange(this->bMouseLeftDown);
    }
}

void Mouse::onMiddleChange(bool middleDown) {
    this->bMouseMiddleDown = middleDown;

    for(size_t i = 0; i < this->listeners.size(); i++) {
        this->listeners[i]->onMiddleChange(this->bMouseMiddleDown);
    }
}

void Mouse::onRightChange(bool rightDown) {
    this->bMouseRightDown = rightDown;

    for(size_t i = 0; i < this->listeners.size(); i++) {
        this->listeners[i]->onRightChange(this->bMouseRightDown);
    }
}

void Mouse::onButton4Change(bool button4down) {
    this->bMouse4Down = button4down;

    for(size_t i = 0; i < this->listeners.size(); i++) {
        this->listeners[i]->onButton4Change(this->bMouse4Down);
    }
}

void Mouse::onButton5Change(bool button5down) {
    this->bMouse5Down = button5down;

    for(size_t i = 0; i < this->listeners.size(); i++) {
        this->listeners[i]->onButton5Change(this->bMouse5Down);
    }
}

void Mouse::setPos(Vector2d newPos) {
    this->bSetPosWasCalledLastFrame = true;

    this->setPosXY(newPos.x, newPos.y);
    env->setMousePos(this->vPos.x, this->vPos.y);

    this->vPrevOsMousePos = this->vPos;
    this->vPrevOsMousePos.x = this->vPrevOsMousePos.x;
    this->vPrevOsMousePos.y = this->vPrevOsMousePos.y;
}

void Mouse::setCursorType(CURSORTYPE cursorType) { env->setCursor(cursorType); }

void Mouse::setCursorVisible(bool cursorVisible) { env->setCursorVisible(cursorVisible); }

bool Mouse::isCursorVisible() { return env->isCursorVisible(); }
