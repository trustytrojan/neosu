//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		windows software rasterizer graphics interface
//
// $NoKeywords: $winswi
//===============================================================================//

#ifdef _WIN32

#ifndef WINSWGRAPHICSINTERFACE_H
#define WINSWGRAPHICSINTERFACE_H

#include "cbase.h"
#include <Windows.h>

#include "SWGraphicsInterface.h"

class WinSWGraphicsInterface : public SWGraphicsInterface {
   public:
    WinSWGraphicsInterface(HWND hwnd);
    virtual ~WinSWGraphicsInterface();

    // scene
    void endScene();

    // device settings
    void setVSync(bool vsync);

   private:
    // device context
    HWND m_hwnd;
    HDC m_hdc;
};

#endif

#endif
