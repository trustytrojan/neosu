//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		app base class (v3)
//
// $NoKeywords: $appb
//===============================================================================//

#ifndef APP_H
#define APP_H

#include "KeyboardListener.h"
#include "cbase.h"

class Engine;

class App : public KeyboardListener {
   public:
    App() { ; }
    ~App() override { ; }

    virtual void draw(Graphics *g) { (void)g; }
    virtual void update() { ; }

    void onKeyDown(KeyboardEvent &e) override { (void)e; }
    void onKeyUp(KeyboardEvent &e) override { (void)e; }
    void onChar(KeyboardEvent &e) override { (void)e; }
    virtual void stealFocus() { ; }

    virtual void onResolutionChanged(Vector2 newResolution) { (void)newResolution; }
    virtual void onDPIChanged() { ; }

    virtual void onFocusGained() { ; }
    virtual void onFocusLost() { ; }

    virtual void onMinimized() { ; }
    virtual void onRestored() { ; }

    virtual bool onShutdown() { return true; }
};

#endif
