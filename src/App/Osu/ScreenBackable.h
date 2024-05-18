#pragma once
#include "OsuScreen.h"
#include "UIBackButton.h"

class ScreenBackable : public OsuScreen {
   public:
    ScreenBackable();
    virtual ~ScreenBackable();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);
    virtual void onKeyDown(KeyboardEvent &e);
    virtual void onResolutionChange(Vector2 newResolution);

   protected:
    virtual void onBack() = 0;

    virtual void updateLayout();

    UIBackButton *m_backButton;
};
