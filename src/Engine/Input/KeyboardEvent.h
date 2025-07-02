//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		a wrapper for consumable keyboard events
//
// $NoKeywords: $key
//===============================================================================//

#ifndef KEYBOARDEVENT_H
#define KEYBOARDEVENT_H

#include <cstdint>
using KEYCODE = uint_fast16_t;

class KeyboardEvent {
   public:
    KeyboardEvent(KEYCODE keyCode) : keyCode(keyCode) {}

    constexpr void consume() { this->bConsumed = true; }

    [[nodiscard]] inline bool isConsumed() const { return this->bConsumed; }
    [[nodiscard]] inline KEYCODE getKeyCode() const { return this->keyCode; }
    [[nodiscard]] inline KEYCODE getCharCode() const { return this->keyCode; }

    inline bool operator==(const KEYCODE &rhs) const { return this->keyCode == rhs; }
    inline bool operator!=(const KEYCODE &rhs) const { return this->keyCode != rhs; }

    explicit operator KEYCODE() const { return this->keyCode; }

   private:
    KEYCODE keyCode;
    bool bConsumed{false};
};

#endif
