#pragma once
#include "OsuScreen.h"

class McFont;
class MainMenuPauseButton;
class CBaseUILabel;
class UserCard;
class CBaseUIScrollView;
class UIButton;

class SpectatorScreen : public OsuScreen {
   public:
    SpectatorScreen();

    virtual void draw(Graphics* g) override;
    virtual bool isVisible() override;
    virtual void onKeyDown(KeyboardEvent& e) override;
    void onStopSpectatingClicked();

    i32 current_map_id = 0;

   private:
    McFont* font = NULL;
    McFont* lfont = NULL;
    MainMenuPauseButton* m_pauseButton = NULL;
    CBaseUIScrollView* m_background = NULL;
    UserCard* m_userCard = NULL;
    UIButton* m_stop_btn = NULL;
    CBaseUILabel* m_spectating = NULL;
    CBaseUILabel* m_status = NULL;
};

void start_spectating(i32 user_id);
void stop_spectating();
