#pragma once
#include "OsuScreen.h"

class CBaseUILabel;
class CBaseUITextbox;
class Graphics;
class UIButton;

class PromptScreen : public OsuScreen {
   public:
    PromptScreen();
    virtual void onResolutionChange(Vector2 newResolution);

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);
    virtual void onKeyDown(KeyboardEvent &e);
    virtual void onKeyUp(KeyboardEvent &e);
    virtual void onChar(KeyboardEvent &e);

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
