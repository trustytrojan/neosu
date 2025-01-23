#pragma once
#include "OsuScreen.h"

class SongBrowser;
class CBaseUIContainer;
class UIPauseMenuButton;

class PauseMenu : public OsuScreen {
   public:
    PauseMenu();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    virtual void onKeyDown(KeyboardEvent &e);
    virtual void onKeyUp(KeyboardEvent &e);
    virtual void onChar(KeyboardEvent &e);

    virtual void onResolutionChange(Vector2 newResolution);

    virtual CBaseUIContainer *setVisible(bool visible);

    void setContinueEnabled(bool continueEnabled);

   private:
    void updateLayout();

    void onContinueClicked();
    void onRetryClicked();
    void onBackClicked();

    void onSelectionChange();

    void scheduleVisibilityChange(bool visible);

    UIPauseMenuButton *addButton(std::function<Image *()> getImageFunc, UString name);

    bool bScheduledVisibilityChange;
    bool bScheduledVisibility;

    std::vector<UIPauseMenuButton *> buttons;
    UIPauseMenuButton *selectedButton;
    float fWarningArrowsAnimStartTime;
    float fWarningArrowsAnimAlpha;
    float fWarningArrowsAnimX;
    float fWarningArrowsAnimY;
    bool bInitialWarningArrowFlyIn;

    bool bContinueEnabled;
    bool bClick1Down;
    bool bClick2Down;

    float fDimAnim;
};
