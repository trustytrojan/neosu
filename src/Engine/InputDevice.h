// Copyright (c) 2015, PG, All rights reserved.
#ifndef INPUTDEVICE_H
#define INPUTDEVICE_H

#include "cbase.h"

class InputDevice {
    NOCOPY_NOMOVE(InputDevice)

   public:
    InputDevice() = default;
    virtual ~InputDevice() = default;

    virtual void update() = 0;
    virtual void draw() = 0;
};

#endif
