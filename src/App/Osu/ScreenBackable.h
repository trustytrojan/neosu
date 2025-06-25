#pragma once
#include "OsuScreen.h"
#include "UIBackButton.h"

class ScreenBackable : public OsuScreen {
   public:
    ScreenBackable();
    ~ScreenBackable() override;

    void draw(Graphics *g) override;
    void mouse_update(bool *propagate_clicks) override;
    void onKeyDown(KeyboardEvent &e) override;
    void onResolutionChange(Vector2 newResolution) override;
    virtual void onBack() = 0;
    virtual void updateLayout();

    UIBackButton *backButton;
};
