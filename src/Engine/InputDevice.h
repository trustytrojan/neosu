// Copyright (c) 2015, PG, All rights reserved.
#ifndef INPUTDEVICE_H
#define INPUTDEVICE_H

#include "cbase.h"

class InputDevice {
   public:
    InputDevice() { ; }
    virtual ~InputDevice() { ; }

    virtual void update() { ; }
    virtual void draw() { ; }
};

#endif
