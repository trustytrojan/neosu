#include "PromptScreen.h"

#include "CBaseUILabel.h"
#include "CBaseUITextbox.h"
#include "Keyboard.h"
#include "Osu.h"
#include "UIButton.h"

PromptScreen::PromptScreen() : OsuScreen() {
    m_prompt_label = new CBaseUILabel(0, 0, 0, 0, "", "");
    m_prompt_label->setDrawFrame(false);
    m_prompt_label->setDrawBackground(false);
    addBaseUIElement(m_prompt_label);

    m_prompt_input = new CBaseUITextbox(0, 0, 400, 40, "");
    addBaseUIElement(m_prompt_input);

    m_ok_btn = new UIButton(0, 0, 110, 35, "ok_btn", "OK");
    m_ok_btn->setColor(0xff00ff00);
    m_ok_btn->setUseDefaultSkin();
    m_ok_btn->setClickCallback(fastdelegate::MakeDelegate(this, &PromptScreen::on_ok));
    addBaseUIElement(m_ok_btn);

    m_cancel_btn = new UIButton(0, 0, 110, 35, "cancel_btn", "Cancel");
    m_cancel_btn->setColor(0xff0e94b5);
    m_cancel_btn->setUseDefaultSkin();
    m_cancel_btn->setClickCallback(fastdelegate::MakeDelegate(this, &PromptScreen::on_cancel));
    addBaseUIElement(m_cancel_btn);
}

void PromptScreen::onResolutionChange(Vector2 newResolution) {
    const float xmiddle = newResolution.x / 2;
    const float ymiddle = newResolution.y / 2;

    setSize(newResolution);

    m_prompt_label->setSizeToContent();
    m_prompt_label->setPos(xmiddle - 200, ymiddle - 30);

    m_prompt_input->setPos(xmiddle - 200, ymiddle);

    m_ok_btn->setPos(xmiddle - 120, ymiddle + 50);
    m_cancel_btn->setPos(xmiddle + 10, ymiddle + 50);
}

void PromptScreen::draw(Graphics *g) {
    if(!m_bVisible) return;

    g->setColor(COLOR(200, 0, 0, 0));
    g->fillRect(0, 0, getSize().x, getSize().y);

    OsuScreen::draw(g);
}

void PromptScreen::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible) return;

    OsuScreen::mouse_update(propagate_clicks);
    *propagate_clicks = false;
}

void PromptScreen::onKeyDown(KeyboardEvent &e) {
    if(!m_bVisible) return;

    if(e == KEY_ENTER) {
        on_ok();
        e.consume();
        return;
    }

    if(e == KEY_ESCAPE) {
        on_cancel();
        e.consume();
        return;
    }

    m_prompt_input->onKeyDown(e);
    e.consume();
}

void PromptScreen::onKeyUp(KeyboardEvent &e) {
    if(!m_bVisible) return;
    m_prompt_input->onKeyUp(e);
    e.consume();
}

void PromptScreen::onChar(KeyboardEvent &e) {
    if(!m_bVisible) return;
    m_prompt_input->onChar(e);
    e.consume();
}

void PromptScreen::prompt(UString msg, PromptResponseCallback callback) {
    m_prompt_label->setText(msg);
    m_prompt_input->setText("");
    m_prompt_input->focus();
    m_callback = callback;
    m_bVisible = true;

    onResolutionChange(osu->getScreenSize());
}

void PromptScreen::on_ok() {
    m_bVisible = false;
    m_callback(m_prompt_input->getText());
}

void PromptScreen::on_cancel() { m_bVisible = false; }
