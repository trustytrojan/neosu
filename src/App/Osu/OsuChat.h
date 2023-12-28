#pragma once

#include "CBaseUIScrollView.h"
#include "CBaseUITextbox.h"
#include "OsuScreen.h"


// TODO @kiwec: grab mouse clicks (need to add conditions everywhere in the code)
// TODO @kiwec: handle having no channels open (display something in the background)
// TODO @kiwec: draw background behind buttons
// TODO @kiwec: do something when username is clicked
// TODO @kiwec: limit to 100 messages / channel
// TODO @kiwec: auto-scrolling + always open at bottom (latest message)
// TODO @kiwec: optimize

class CBaseUIButton;
class McFont;
class OsuUIButton;


struct ChatMessage {
    uint32_t author_id;
    UString author_name;
    UString text;
};

struct OsuChatChannel {
    OsuChatChannel(Osu* osu, CBaseUIContainer* container, UString name_arg);
    ~OsuChatChannel();

    Osu* m_osu;
    CBaseUIContainer* m_container;
    McFont* font;
    CBaseUIScrollView* ui;
    OsuUIButton *btn;
    UString name;
    std::vector<ChatMessage> messages;
    float y_total;
    bool read = true;

    void add_message(ChatMessage msg);
    void updateLayout(Vector2 pos, Vector2 size);
    void onChannelButtonClick(CBaseUIButton* btn);
};

struct OsuChat : public OsuScreen
{
    OsuChat(Osu *osu);

    virtual void draw(Graphics *g);
    virtual void update();
    virtual void onKeyDown(KeyboardEvent &e);
    virtual void onKeyUp(KeyboardEvent &e);
    virtual void onChar(KeyboardEvent &e);
    virtual void onResolutionChange(Vector2 newResolution);
    virtual void setVisible(bool visible);

    void switchToChannel(OsuChatChannel* chan);
    void addChannel(UString channel_name);
    void addMessage(UString channel_name, ChatMessage msg);
    void removeChannel(UString channel_name);
    void updateLayout(Vector2 newResolution);
    void updateButtonLayout(Vector2 screen);

    OsuChatChannel* m_selected_channel = nullptr;
    std::vector<OsuChatChannel*> m_channels;
    CBaseUIContainer *m_container;
    CBaseUITextbox *m_input_box;

    McFont* font;
    float m_fAnimation = 0.f;
};
