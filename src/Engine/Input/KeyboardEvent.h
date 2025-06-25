//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		a wrapper for consumable keyboard events
//
// $NoKeywords: $key
//===============================================================================//

#ifndef KEYBOARDEVENT_H
#define KEYBOARDEVENT_H

typedef unsigned long KEYCODE;

class KeyboardEvent {
   public:
    KeyboardEvent(KEYCODE keyCode);

    void consume();

    [[nodiscard]] inline bool isConsumed() const { return this->bConsumed; }
    [[nodiscard]] inline KEYCODE getKeyCode() const { return this->keyCode; }
    [[nodiscard]] inline KEYCODE getCharCode() const { return this->keyCode; }

    bool operator==(const KEYCODE &rhs) const;
    bool operator!=(const KEYCODE &rhs) const;

   private:
    KEYCODE keyCode;
    bool bConsumed;
};

inline bool KeyboardEvent::operator==(const KEYCODE &rhs) const { return this->keyCode == rhs; }

inline bool KeyboardEvent::operator!=(const KEYCODE &rhs) const { return this->keyCode != rhs; }

#endif
