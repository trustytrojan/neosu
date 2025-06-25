#pragma once
#include "OsuScreen.h"

class SongBrowser;
class CBaseUIContainer;
class UIPauseMenuButton;

class PauseMenu : public OsuScreen {
   public:
    PauseMenu();

    void draw(Graphics *g) override;
    void mouse_update(bool *propagate_clicks) override;

    void onKeyDown(KeyboardEvent &e) override;
    void onKeyUp(KeyboardEvent &e) override;
    void onChar(KeyboardEvent &e) override;

    void onResolutionChange(Vector2 newResolution) override;

    CBaseUIContainer *setVisible(bool visible) override;

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
