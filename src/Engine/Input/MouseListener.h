//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		simple listener class for mouse events
//
// $NoKeywords: $mouse
//===============================================================================//

#ifndef MOUSELISTENER_H
#define MOUSELISTENER_H

class MouseListener {
   public:
    virtual ~MouseListener() { ; }

    virtual void onLeftChange(bool down) { (void)down; }
    virtual void onMiddleChange(bool down) { (void)down; }
    virtual void onRightChange(bool down) { (void)down; }
    virtual void onButton4Change(bool down) { (void)down; }
    virtual void onButton5Change(bool down) { (void)down; }

    virtual void onWheelVertical(int delta) { (void)delta; }
    virtual void onWheelHorizontal(int delta) { (void)delta; }
};

#endif
