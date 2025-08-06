//========== Copyright (c) 2015, PG & 2025, WH, All rights reserved. ============//
//
// Purpose:		mouse wrapper
//
// $NoKeywords: $mouse
//===============================================================================//

#include "Mouse.h"

#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"
#include "ResourceManager.h"

Mouse::Mouse() : InputDevice(), vPos(env->getMousePos()), vPosWithoutOffsets(this->vPos), vActualPos(this->vPos) {
    this->fSensitivity = cv::mouse_sensitivity.getFloat();
    this->bIsRawInput = cv::mouse_raw_input.getBool();
    cv::mouse_raw_input.setCallback(SA::MakeDelegate<&Mouse::onRawInputChanged>(this));
    cv::mouse_sensitivity.setCallback(SA::MakeDelegate<&Mouse::onSensitivityChanged>(this));
}

void Mouse::draw() {
    if(!cv::debug_mouse.getBool()) return;

    this->drawDebug();

    // green rect = virtual cursor pos
    g->setColor(0xff00ff00);
    float size = 20.0f;
    g->drawRect(this->vActualPos.x - size / 2, this->vActualPos.y - size / 2, size, size);

    // red rect = real cursor pos
    g->setColor(0xffff0000);
    Vector2 envPos = env->getMousePos();
    g->drawRect(envPos.x - size / 2, envPos.y - size / 2, size, size);

    // red = cursor clip
    if(env->isCursorClipped()) {
        McRect cursorClip = env->getCursorClip();
        g->drawRect(cursorClip.getMinX(), cursorClip.getMinY(), cursorClip.getWidth() - 1, cursorClip.getHeight() - 1);
    }

    // green = scaled & offset virtual area
    const Vector2 scaledOffset = this->vOffset;
    const Vector2 scaledEngineScreenSize = engine->getScreenSize() * this->vScale;
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
    this->resetWheelDelta();

    // if onMotion wasn't called last frame, there is no motion delta
    if(!this->bLastFrameHadMotion) {
        this->vDelta.zero();
        this->vRawDelta.zero();
    } else {
        // center the OS cursor if it's close to the screen edges, for non-raw input with sensitivity <1.0
        // the reason for trying hard to avoid env->setMousePos is because setting the OS cursor position can take a
        // long time so we try to use the virtual cursor position as much as possible and only update the OS cursor when
        // it's going to go somewhere we don't want
        if(this->bNeedsLock) {
            const bool clipped = env->isCursorClipped();
            const McRect clipRect{clipped ? env->getCursorClip() : engine->getScreenRect()};
            const Vector2 center{clipRect.getCenter()};
            const Vector2 realPosNudgedOut{Vector2{env->getMousePos()}.nudge(center, 10.0f)};
            if(!clipRect.contains(realPosNudgedOut)) {
                if(clipped)
                    env->setMousePos(center);
                else if(!env->isCursorVisible())  // FIXME: this is crazy. for windowed mode, need to "pop out" the OS
                                                  // cursor
                    env->setMousePos(Vector2{this->vPosWithoutOffsets}.nudge(center, 0.1f));
            }
        }

        this->onPosChange(this->vPosWithoutOffsets);
    }

    this->bLastFrameHadMotion = false;
}

void Mouse::onMotion(Vector2 rel, Vector2 abs, bool preTransformed) {
    Vector2 newRel{rel}, newAbs{abs};

    this->bNeedsLock = false;  // assume we don't have to lock the cursor

    const bool osCursorVisible = (env->isCursorVisible() || !env->isCursorInWindow() || !engine->hasFocus());

    // rawinput has sensitivity pre-applied
    // this entire block may be skipped if: (preTransformed || (sens == 1 && !clipped))
    if(!preTransformed && !osCursorVisible) {
        // need to apply sensitivity
        if(this->fSensitivity < 0.999f || this->fSensitivity > 1.001f) {
            // need to lock the OS cursor to the center of the screen if rawinput is disabled, otherwise it can exit the
            // screen rect before the virtual cursor does don't do it here because we don't want the event loop to make
            // more external calls than necessary, just set a flag to do it on the engine update loop
            if(this->fSensitivity < 0.995f) {
                this->bNeedsLock = true;
            }
            newRel *= this->fSensitivity;
            if(newRel.length() > 50.0f)  // don't allow obviously bogus values
                newRel.zero();
            newAbs = this->vPosWithoutOffsets + newRel;
        }
        if(env->isCursorClipped()) {
            const McRect clipRect = env->getCursorClip();

            // clamp the final position to the clip rect
            newAbs.x = std::clamp<float>(newAbs.x, clipRect.getMinX(), clipRect.getMaxX());
            newAbs.y = std::clamp<float>(newAbs.y, clipRect.getMinY(), clipRect.getMaxY());
        }
    }

    // we have to accumulate all motion collected in this frame, then reset it at the start of the next frame
    // use Mouse::update always setting m_bLastFrameHadMotion to false as a signal that the deltas need to be reset now
    // this is because onMotion can be called multiple times in a frame, depending on how many mouse motion events were
    // collected but Mouse::update only happens once per frame
    if(!this->bLastFrameHadMotion) {
        this->vDelta.zero();
        this->vRawDelta.zero();
    }

    // rawdelta doesn't include sensitivity or clipping
    this->vRawDelta += (newRel / this->fSensitivity);
    this->vDelta += newRel;
    // for the absolute position, we can just update it directly
    this->vPosWithoutOffsets = newAbs;

    this->bLastFrameHadMotion = true;

    if(unlikely(cv::debug_mouse.getBool()))
        debugLog(
            "frame: {} rawInput: {} m_vRawDelta: {:.2f},{:.2f} m_vDelta: {:.2f},{:.2f} m_vPosWithoutOffset: "
            "{:.2f},{:.2f}\n",
            engine->getFrameCount() + 1, preTransformed, this->vRawDelta.x, this->vRawDelta.y, this->vDelta.x,
            this->vDelta.y, this->vPosWithoutOffsets.x, this->vPosWithoutOffsets.y);
}

void Mouse::resetWheelDelta() {
    this->iWheelDeltaVertical = this->iWheelDeltaVerticalActual;
    this->iWheelDeltaVerticalActual = 0;

    this->iWheelDeltaHorizontal = this->iWheelDeltaHorizontalActual;
    this->iWheelDeltaHorizontalActual = 0;
}

void Mouse::onPosChange(Vector2 pos) {
    this->vPosWithoutOffsets = pos;
    this->vPos = (this->vOffset + pos);
    this->vActualPos = this->vPos;
}

void Mouse::onWheelVertical(int delta) {
    this->iWheelDeltaVerticalActual += delta;

    for(auto &listener : this->listeners) {
        listener->onWheelVertical(delta);
    }
}

void Mouse::onWheelHorizontal(int delta) {
    this->iWheelDeltaHorizontalActual += delta;

    for(auto &listener : this->listeners) {
        listener->onWheelHorizontal(delta);
    }
}

void Mouse::onButtonChange(ButtonIndex button, bool down) {
    using enum ButtonIndex;
    if(button == BUTTON_NONE || button >= BUTTON_COUNT) return;

    this->bMouseButtonDownArray[static_cast<size_t>(button)] = down;

    // notify listeners
    for(auto &listener : this->listeners) {
        listener->onButtonChange(button, down);
    }
}

void Mouse::setPos(Vector2 newPos) {
    this->bLastFrameHadMotion = true;

    this->vPos = newPos;
    env->setMousePos(newPos.x, newPos.y);
}

void Mouse::setOffset(Vector2 offset) {
    Vector2 oldOffset = this->vOffset;
    this->vOffset = offset;

    // update position to maintain visual position after offset change
    Vector2 posAdjustment = this->vOffset - oldOffset;
    this->vPos += posAdjustment;
    this->vActualPos += posAdjustment;
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

void Mouse::onRawInputChanged(float newval) {
    this->bIsRawInput = !!static_cast<int>(newval);
    env->notifyWantRawInput(this->bIsRawInput);  // request environment to change the real OS cursor state (may or may
                                                 // not take effect immediately)
}

void Mouse::onSensitivityChanged(float newSens) { this->fSensitivity = newSens; }
