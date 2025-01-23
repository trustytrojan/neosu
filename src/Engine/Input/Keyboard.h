//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		state class and listeners (FIFO)
//
// $NoKeywords: $key
//===============================================================================//

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "InputDevice.h"
#include "KeyboardEvent.h"
#include "KeyboardKeys.h"
#include "KeyboardListener.h"

class Keyboard : public InputDevice {
   public:
    Keyboard();
    virtual ~Keyboard() { ; }

    void addListener(KeyboardListener *keyboardListener, bool insertOnTop = false);
    void removeListener(KeyboardListener *keyboardListener);
    void reset();

    virtual void onKeyDown(KEYCODE keyCode);
    virtual void onKeyUp(KEYCODE keyCode);
    virtual void onChar(KEYCODE charCode);

    inline bool isControlDown() const { return this->bControlDown; }
    inline bool isAltDown() const { return this->bAltDown; }
    inline bool isShiftDown() const { return this->bShiftDown; }
    inline bool isSuperDown() const { return this->bSuperDown; }

   private:
    bool bControlDown;
    bool bAltDown;
    bool bShiftDown;
    bool bSuperDown;

    std::vector<KeyboardListener *> listeners;
};

#endif
