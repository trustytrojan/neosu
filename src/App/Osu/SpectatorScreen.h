#pragma once
// Copyright (c) 2024, kiwec, All rights reserved.

#include "OsuScreen.h"

class McFont;
class PauseButton;
class CBaseUILabel;
class UserCard;
class CBaseUIScrollView;
class UIButton;

class SpectatorScreen : public OsuScreen {
   public:
    SpectatorScreen();

    void mouse_update(bool* propagate_clicks) override;
    void draw() override;
    bool isVisible() override;
    CBaseUIElement* setVisible(bool visible) override;
    void onKeyDown(KeyboardEvent& e) override;
    void onStopSpectatingClicked();

    UserCard* userCard = NULL;

   private:
    McFont* font = NULL;
    McFont* lfont = NULL;
    PauseButton* pauseButton = NULL;
    CBaseUIScrollView* background = NULL;
    UIButton* stop_btn = NULL;
    CBaseUILabel* spectating = NULL;
    CBaseUILabel* status = NULL;
};

void start_spectating(i32 user_id);
void stop_spectating();
