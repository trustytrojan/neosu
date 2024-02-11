#pragma once
#include "OsuScreen.h"

class CBaseUILabel;
class CBaseUITextbox;
class Graphics;
class Osu;
class OsuUIButton;

class OsuPromptScreen : public OsuScreen {
public:
    OsuPromptScreen(Osu *osu);
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

    CBaseUILabel *m_prompt_label;
    CBaseUITextbox *m_prompt_input;
    OsuUIButton *m_ok_btn;
    OsuUIButton *m_cancel_btn;
    PromptResponseCallback m_callback;
};
