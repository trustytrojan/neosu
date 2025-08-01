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

    Chat *chat;
    CBaseUIScrollView *ui;
    UIButton *btn;
    UString name;
    std::vector<ChatMessage> messages;
    float y_total{.0f};
    bool read = true;

    void add_message(ChatMessage msg);
    void updateLayout(Vector2 pos, Vector2 size);
    void onChannelButtonClick(CBaseUIButton *btn);
};

class Chat : public OsuScreen {
   public:
    Chat();
    ~Chat() override;

    void draw() override;
    void drawTicker();
    void mouse_update(bool *propagate_clicks) override;
    void onKeyDown(KeyboardEvent &e) override;
    void onKeyUp(KeyboardEvent &e) override;
    void onChar(KeyboardEvent &e) override;
    void onResolutionChange(Vector2 newResolution) override;

    void mark_as_read(ChatChannel *chan);
    void switchToChannel(ChatChannel *chan);
    void addChannel(const UString &channel_name, bool switch_to = false);
    void addMessage(UString channel_name, const ChatMessage &msg, bool mark_unread = true);
    void addSystemMessage(UString msg);
    void removeChannel(const UString &channel_name);
    void updateLayout(Vector2 newResolution);
    void updateButtonLayout(Vector2 screen);
    void updateTickerLayout(Vector2 screen);
    void updateUserList();

    void join(const UString &channel_name);
    void leave(const UString &channel_name);
    void handle_command(const UString &msg);
    void send_message(const UString &msg);
    void onDisconnect();

    CBaseUIContainer *setVisible(bool visible) override;
    bool isSmallChat();
    bool isVisibilityForced();
    void updateVisibility();
    bool isMouseInChat();

    void askWhatChannelToJoin(CBaseUIButton *btn);
    UIButton *join_channel_btn;

    ChatChannel *selected_channel = NULL;
    std::vector<ChatChannel *> channels;
    CBaseUIContainer *button_container;
    CBaseUITextbox *input_box;
    CBaseUIScrollView *user_list;

    float fAnimation = 0.f;
    bool user_wants_chat = false;
    bool visibility_was_forced = false;
    bool layout_update_scheduled = false;

    const float input_box_height = 30.f;
    const float button_height = 26.f;

    UString away_msg;
    UString tab_completion_prefix;
    UString tab_completion_match;

    ChatChannel *ticker = NULL;
    f64 ticker_tms = 0.0;
};
