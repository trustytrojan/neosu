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
    McFont* font = nullptr;
    McFont* lfont = nullptr;
    MainMenuPauseButton* m_pauseButton = nullptr;
    CBaseUIScrollView* m_background = nullptr;
    UserCard* m_userCard = nullptr;
    UIButton* m_stop_btn = nullptr;
    CBaseUILabel* m_spectating = nullptr;
    CBaseUILabel* m_status = nullptr;
};

void start_spectating(i32 user_id);
void stop_spectating();
