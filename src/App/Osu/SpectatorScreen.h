#pragma once
#include "OsuScreen.h"

class McFont;
class MainMenuPauseButton;
class CBaseUILabel;
class UserCard;

class SpectatorScreen : public OsuScreen {
   public:
    SpectatorScreen();

    virtual void draw(Graphics* g) override;
    virtual void mouse_update(bool* propagate_clicks) override;

    i32 current_map_id = 0;

   private:
    McFont* font = nullptr;
    McFont* lfont = nullptr;
    MainMenuPauseButton* m_pauseButton = nullptr;
    UserCard* m_userCard = nullptr;
    CBaseUILabel* m_spectating = nullptr;
    CBaseUILabel* m_status = nullptr;
};

void start_spectating(i32 user_id);
void stop_spectating();
