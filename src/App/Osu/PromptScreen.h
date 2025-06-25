#pragma once
#include "OsuScreen.h"

class CBaseUILabel;
class CBaseUITextbox;
class Graphics;
class UIButton;

class PromptScreen : public OsuScreen {
   public:
    PromptScreen();
    void onResolutionChange(Vector2 newResolution) override;

    void draw(Graphics *g) override;
    void mouse_update(bool *propagate_clicks) override;
    void onKeyDown(KeyboardEvent &e) override;
    void onKeyUp(KeyboardEvent &e) override;
    void onChar(KeyboardEvent &e) override;

    typedef fastdelegate::FastDelegate1<UString> PromptResponseCallback;
    void prompt(UString msg, PromptResponseCallback callback);

   private:
    void on_ok();
    void on_cancel();

    CBaseUILabel *prompt_label;
    CBaseUITextbox *prompt_input;
    UIButton *ok_btn;
    UIButton *cancel_btn;
    PromptResponseCallback callback;
};
