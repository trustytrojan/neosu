#pragma once

#include "CBaseUIScrollView.h"
#include "CBaseUITextbox.h"
#include "OsuScreen.h"

class CBaseUIButton;
class McFont;
class Chat;
class UIButton;

struct ChatMessage {
    time_t tms;
    i32 author_id;
    UString author_name;
    UString text;
};

struct ChatChannel {
    ChatChannel(Chat *chat, UString name_arg);
    ~ChatChannel();

    Chat *m_chat;
    CBaseUIScrollView *ui;
    UIButton *btn;
    UString name;
    std::vector<ChatMessage> messages;
    float y_total;
    bool read = true;

    void add_message(ChatMessage msg);
    void updateLayout(Vector2 pos, Vector2 size);
    void onChannelButtonClick(CBaseUIButton *btn);
};

class Chat : public OsuScreen {
   public:
    Chat();
    ~Chat();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);
    virtual void onKeyDown(KeyboardEvent &e);
    virtual void onKeyUp(KeyboardEvent &e);
    virtual void onChar(KeyboardEvent &e);
    virtual void onResolutionChange(Vector2 newResolution);

    void mark_as_read(ChatChannel *chan);
    void switchToChannel(ChatChannel *chan);
    void addChannel(UString channel_name, bool switch_to = false);
    void addMessage(UString channel_name, ChatMessage msg, bool mark_unread = true);
    void addSystemMessage(UString msg);
    void removeChannel(UString channel_name);
    void updateLayout(Vector2 newResolution);
    void updateButtonLayout(Vector2 screen);

    void join(UString channel_name);
    void leave(UString channel_name);
    void handle_command(UString msg);
    void send_message(UString msg);
    void onDisconnect();

    virtual CBaseUIContainer *setVisible(bool visible);
    bool isSmallChat();
    bool isVisibilityForced();
    void updateVisibility();
    bool isMouseInChat();

    void askWhatChannelToJoin(CBaseUIButton *btn);
    UIButton *join_channel_btn;

    ChatChannel *m_selected_channel = NULL;
    std::vector<ChatChannel *> m_channels;
    CBaseUIContainer *m_button_container;
    CBaseUITextbox *m_input_box;

    McFont *font;
    float m_fAnimation = 0.f;
    bool user_wants_chat = false;
    bool visibility_was_forced = false;
    bool layout_update_scheduled = false;

    const float input_box_height = 30.f;
    const float button_height = 26.f;

    UString away_msg;
    UString tab_completion_prefix;
    UString tab_completion_match;
};
