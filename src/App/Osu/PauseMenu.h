//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		pause menu (while playing)
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUPAUSEMENU_H
#define OSUPAUSEMENU_H

#include "OsuScreen.h"

class Osu;
class SongBrowser;
class CBaseUIContainer;
class UIPauseMenuButton;

class PauseMenu : public OsuScreen {
   public:
    PauseMenu(Osu *osu);

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

    UIPauseMenuButton *addButton(std::function<Image *()> getImageFunc);

    bool m_bScheduledVisibilityChange;
    bool m_bScheduledVisibility;

    std::vector<UIPauseMenuButton *> m_buttons;
    UIPauseMenuButton *m_selectedButton;
    float m_fWarningArrowsAnimStartTime;
    float m_fWarningArrowsAnimAlpha;
    float m_fWarningArrowsAnimX;
    float m_fWarningArrowsAnimY;
    bool m_bInitialWarningArrowFlyIn;

    bool m_bContinueEnabled;
    bool m_bClick1Down;
    bool m_bClick2Down;

    float m_fDimAnim;
};

#endif
