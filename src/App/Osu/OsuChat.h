#pragma once

#include "CBaseUIScrollView.h"
#include "CBaseUITextbox.h"
#include "OsuScreen.h"


// TODO @kiwec: grab mouse clicks (need to add conditions everywhere in the code)
// TODO @kiwec: fix scrolling
// TODO @kiwec: Add timestamps
// TODO @kiwec: do something when username is clicked
// TODO @kiwec: limit to 100 messages / channel
// TODO @kiwec: optimize

// TODO @kiwec: Add channel connection info like osu!stable does
// Eg. use #osu as main (closeable) channel, always add it when logging it

class CBaseUIButton;
class McFont;
class OsuChat;
class OsuUIButton;


struct ChatMessage {
    int32_t author_id;
    UString author_name;
    UString text;
};

struct OsuChatChannel {
    OsuChatChannel(OsuChat* chat, UString name_arg);
    ~OsuChatChannel();

    OsuChat* m_chat;
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

    void switchToChannel(OsuChatChannel* chan);
    void addChannel(UString channel_name);
    void addMessage(UString channel_name, ChatMessage msg);
    void removeChannel(UString channel_name);
    void updateLayout(Vector2 newResolution);
    void updateButtonLayout(Vector2 screen);

    virtual void setVisible(bool visible);
    bool isVisibilityForced();
    void updateVisibility();

    OsuChatChannel* m_selected_channel = nullptr;
    std::vector<OsuChatChannel*> m_channels;
    CBaseUIContainer *m_container;
    CBaseUIContainer *m_button_container;
    CBaseUITextbox *m_input_box;

    McFont* font;
    float m_fAnimation = 0.f;
    bool user_wants_chat = false;
};
