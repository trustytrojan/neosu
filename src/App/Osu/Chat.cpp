#include "Chat.h"

#include <regex>

#include "AnimationHandler.h"
#include "Bancho.h"
#include "BanchoNetworking.h"
#include "BanchoUsers.h"
#include "Beatmap.h"
#include "CBaseUIButton.h"
#include "CBaseUIContainer.h"
#include "CBaseUILabel.h"
#include "CBaseUITextbox.h"
#include "ChatLink.h"
#include "Engine.h"
#include "Font.h"
#include "Keyboard.h"
#include "Lobby.h"
#include "ModSelector.h"
#include "OptionsMenu.h"
#include "Osu.h"
#include "PauseMenu.h"
#include "PromptScreen.h"
#include "RenderTarget.h"
#include "ResourceManager.h"
#include "RoomScreen.h"
#include "Skin.h"
#include "SongBrowser/ScoreButton.h"
#include "SongBrowser/SongBrowser.h"
#include "SoundEngine.h"
#include "SpectatorScreen.h"
#include "UIButton.h"
#include "UIUserContextMenu.h"

ChatChannel::ChatChannel(Chat *chat, UString name_arg) {
    m_chat = chat;
    name = name_arg;

    ui = new CBaseUIScrollView(0, 0, 0, 0, "");
    ui->setDrawFrame(false);
    ui->setDrawBackground(true);
    ui->setBackgroundColor(0xdd000000);
    ui->setHorizontalScrolling(false);
    ui->setDrawScrollbars(true);
    ui->sticky = true;

    btn = new UIButton(0, 0, 0, 0, "button", name);
    btn->grabs_clicks = true;
    btn->setUseDefaultSkin();
    btn->setClickCallback(fastdelegate::MakeDelegate(this, &ChatChannel::onChannelButtonClick));
    m_chat->m_button_container->addBaseUIElement(btn);
}

ChatChannel::~ChatChannel() {
    delete ui;
    m_chat->m_button_container->deleteBaseUIElement(btn);
}

void ChatChannel::onChannelButtonClick(CBaseUIButton *btn) {
    engine->getSound()->play(osu->getSkin()->m_clickButton);
    m_chat->switchToChannel(this);
}

void ChatChannel::add_message(ChatMessage msg) {
    const float line_height = 20;
    const Color system_color = 0xffffff00;

    float x = 10;

    bool is_action = msg.text.startsWith("\001ACTION");
    if(is_action) {
        msg.text.erase(0, 7);
        if(msg.text.endsWith("\001")) {
            msg.text.erase(msg.text.length() - 1, 1);
        }
    }

    struct tm *tm = localtime(&msg.tms);
    auto timestamp_str = UString::format("%02d:%02d ", tm->tm_hour, tm->tm_min);
    if(is_action) timestamp_str.append("*");
    float time_width = m_chat->font->getStringWidth(timestamp_str);
    CBaseUILabel *timestamp = new CBaseUILabel(x, y_total, time_width, line_height, "", timestamp_str);
    timestamp->setDrawFrame(false);
    timestamp->setDrawBackground(false);
    ui->getContainer()->addBaseUIElement(timestamp);
    x += time_width;

    bool is_system_message = msg.author_name.length() == 0;
    if(!is_system_message) {
        float name_width = m_chat->font->getStringWidth(msg.author_name);
        auto user_box = new UIUserLabel(msg.author_id, msg.author_name);
        user_box->setTextColor(0xff2596be);
        user_box->setPos(x, y_total);
        user_box->setSize(name_width, line_height);
        ui->getContainer()->addBaseUIElement(user_box);
        x += name_width;

        if(!is_action) {
            msg.text.insert(0, ": ");
        }
    }

    // regex101 format: (\[\[(.+?)\]\])|(\[((\S+):\/\/\S+) (.+?)\])|(https?:\/\/\S+)
    // This matches:
    // - Raw URLs      https://example.com
    // - Labeled URLs  [https://regex101.com useful website]
    // - Lobby invites [osump://0/ join my lobby plz]
    // - Wiki links    [[Chat Console]]
    //
    // Group 1) [[FAQ]]
    // Group 2) FAQ
    // Group 3) [https://regex101.com label]
    // Group 4) https://regex101.com
    // Group 5) https
    // Group 6) label
    // Group 7) https://example.com
    //
    // Groups 1, 2 only exist for wiki links
    // Groups 3, 4, 5, 6 only exist for labeled links
    // Group 7 only exists for raw links
    std::wregex url_regex(L"(\\[\\[(.+?)\\]\\])|(\\[((\\S+)://\\S+) (.+?)\\])|(https?://\\S+)");

    std::wstring msg_text = msg.text.wc_str();
    std::wsmatch match;
    std::vector<CBaseUILabel *> text_fragments;
    std::wstring::const_iterator search_start = msg_text.cbegin();
    int text_idx = 0;
    while(std::regex_search(search_start, msg_text.cend(), match, url_regex)) {
        int match_pos;
        int match_len;
        UString link_url;
        UString link_label;
        if(match[7].matched) {
            // Raw link
            match_pos = match.position(7);
            match_len = match.length(7);
            link_url = match.str(7).c_str();
            link_label = match.str(7).c_str();
        } else if(match[3].matched) {
            // Labeled link
            match_pos = match.position(3);
            match_len = match.length(3);
            link_url = match.str(4).c_str();
            link_label = match.str(6).c_str();

            // Normalize invite links to osump://
            UString link_protocol = match.str(5).c_str();
            if(link_protocol == UString("osu")) {
                // osu:// -> osump://
                link_url.insert(2, "mp");
            } else if(link_protocol == UString("http://osump")) {
                // http://osump:// -> osump://
                link_url.erase(0, 7);
            }
        } else {
            // Wiki link
            match_pos = match.position(1);
            match_len = match.length(1);
            link_url = "https://osu.ppy.sh/wiki/";
            link_url.append(match.str(2).c_str());
            link_label = "wiki:";
            link_label.append(match.str(2).c_str());
        }

        int search_idx = search_start - msg_text.cbegin();
        auto url_start = search_idx + match_pos;
        auto preceding_text = msg.text.substr(text_idx, url_start - text_idx);
        if(preceding_text.length() > 0) {
            text_fragments.push_back(new CBaseUILabel(0, 0, 0, 0, "", preceding_text));
        }

        auto link = new ChatLink(0, 0, 0, 0, link_url, link_label);
        text_fragments.push_back(link);

        text_idx = url_start + match_len;
        search_start = msg_text.cbegin() + text_idx;
    }
    if(is_action) {
        // Only appending now to prevent this character from being included in a link
        msg.text.append(L"*");
        msg_text.append(L"*");
    }
    if(search_start != msg_text.cend()) {
        auto text = msg.text.substr(text_idx);
        text_fragments.push_back(new CBaseUILabel(0, 0, 0, 0, "", text));
    }

    // We're offsetting the first fragment to account for the username + timestamp
    // Since first fragment will always be text, we don't care about the size being wrong
    float line_width = x;

    // We got a bunch of text fragments, now position them, and if we start a new line,
    // possibly divide them into more text fragments.
    for(auto fragment : text_fragments) {
        UString text_str("");
        auto fragment_text = fragment->getText();

        for(int i = 0; i < fragment_text.length(); i++) {
            float char_width = m_chat->font->getGlyphMetrics(fragment_text[i]).advance_x;
            if(line_width + char_width + 20 >= m_chat->getSize().x) {
                ChatLink *link_fragment = dynamic_cast<ChatLink *>(fragment);
                if(link_fragment == NULL) {
                    CBaseUILabel *text = new CBaseUILabel(x, y_total, line_width - x, line_height, "", text_str);
                    text->setDrawFrame(false);
                    text->setDrawBackground(false);
                    if(is_system_message) {
                        text->setTextColor(system_color);
                    }
                    ui->getContainer()->addBaseUIElement(text);
                } else {
                    ChatLink *link =
                        new ChatLink(x, y_total, line_width - x, line_height, fragment->getName(), text_str);
                    ui->getContainer()->addBaseUIElement(link);
                }

                x = 10;
                y_total += line_height;
                line_width = x;
                text_str = "";
            }

            text_str.append(fragment_text[i]);
            line_width += char_width;
        }

        ChatLink *link_fragment = dynamic_cast<ChatLink *>(fragment);
        if(link_fragment == NULL) {
            CBaseUILabel *text = new CBaseUILabel(x, y_total, line_width - x, line_height, "", text_str);
            text->setDrawFrame(false);
            text->setDrawBackground(false);
            if(is_system_message) {
                text->setTextColor(system_color);
            }
            ui->getContainer()->addBaseUIElement(text);
        } else {
            ChatLink *link = new ChatLink(x, y_total, line_width - x, line_height, fragment->getName(), text_str);
            ui->getContainer()->addBaseUIElement(link);
        }

        x = line_width;
    }

    y_total += line_height;
    ui->setScrollSizeToContent();
}

void ChatChannel::updateLayout(Vector2 pos, Vector2 size) {
    ui->clear();
    ui->setPos(pos);
    ui->setSize(size);
    y_total = 7;

    for(auto msg : messages) {
        add_message(msg);
    }

    ui->setScrollSizeToContent();
}

Chat::Chat() : OsuScreen() {
    font = engine->getResourceManager()->getFont("FONT_DEFAULT");

    m_button_container = new CBaseUIContainer(0, 0, 0, 0, "");

    join_channel_btn = new UIButton(0, 0, 0, 0, "button", "+");
    join_channel_btn->setUseDefaultSkin();
    join_channel_btn->setColor(0xffffff55);
    join_channel_btn->setSize(button_height + 2, button_height + 2);
    join_channel_btn->setClickCallback(fastdelegate::MakeDelegate(this, &Chat::askWhatChannelToJoin));
    m_button_container->addBaseUIElement(join_channel_btn);

    m_input_box = new CBaseUITextbox(0, 0, 0, 0, "");
    m_input_box->setDrawFrame(false);
    m_input_box->setDrawBackground(true);
    m_input_box->setBackgroundColor(0xdd000000);
    addBaseUIElement(m_input_box);

    updateLayout(osu->getScreenSize());
}

Chat::~Chat() { delete m_button_container; }

void Chat::draw(Graphics *g) {
    const bool isAnimating = anim->isAnimating(&m_fAnimation);
    if(!m_bVisible && !isAnimating) return;

    if(isAnimating) {
        // XXX: Setting BLEND_MODE_PREMUL_ALPHA is not enough, transparency is still incorrect
        osu->getSliderFrameBuffer()->enable();
        g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_PREMUL_ALPHA);
    }

    if(m_selected_channel == NULL) {
        const float chat_h = round(getSize().y * 0.3f);
        const float chat_y = getSize().y - chat_h;
        float chat_w = getSize().x;
        if(isSmallChat()) {
            // In the lobby and in multi rooms, don't take the full horizontal width to allow for cleaner UI designs.
            chat_w = round(chat_w * 0.6);
        }

        g->setColor(COLOR(100, 0, 10, 50));
        g->fillRect(0, chat_y - (button_height + 2.f), chat_w, (button_height + 2.f));
        g->setColor(COLOR(150, 0, 0, 0));
        g->fillRect(0, chat_y, chat_w, chat_h);
    } else {
        g->setColor(COLOR(100, 0, 10, 50));
        g->fillRect(m_button_container->getPos().x, m_button_container->getPos().y, m_button_container->getSize().x,
                    m_button_container->getSize().y);
        m_button_container->draw(g);

        OsuScreen::draw(g);
        if(m_selected_channel != NULL) {
            m_selected_channel->ui->draw(g);
        }
    }

    if(isAnimating) {
        osu->getSliderFrameBuffer()->disable();

        g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);
        g->push3DScene(McRect(0, 0, getSize().x, getSize().y));
        {
            g->rotate3DScene(-(1.0f - m_fAnimation) * 90, 0, 0);
            g->translate3DScene(0, -(1.0f - m_fAnimation) * getSize().y * 1.25f, -(1.0f - m_fAnimation) * 700);

            osu->getSliderFrameBuffer()->setColor(COLORf(m_fAnimation, 1.0f, 1.0f, 1.0f));
            osu->getSliderFrameBuffer()->draw(g, 0, 0);
        }
        g->pop3DScene();
    }
}

void Chat::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible) return;
    OsuScreen::mouse_update(propagate_clicks);
    m_button_container->mouse_update(propagate_clicks);
    if(m_selected_channel) {
        m_selected_channel->ui->mouse_update(propagate_clicks);
    }

    // Focus without placing the cursor at the end of the field
    m_input_box->focus(false);
}

void Chat::handle_command(UString msg) {
    if(msg == UString("/clear")) {
        m_selected_channel->messages.clear();
        updateLayout(osu->getScreenSize());
        return;
    }

    if(msg == UString("/close") || msg == UString("/p") || msg == UString("/part")) {
        leave(m_selected_channel->name);
        return;
    }

    if(msg == UString("/help") || msg == UString("/keys")) {
        env->openURLInDefaultBrowser("https://osu.ppy.sh/wiki/en/Client/Interface/Chat_console");
        return;
    }

    if(msg == UString("/np")) {
        auto diff = osu->getSelectedBeatmap()->getSelectedDifficulty2();
        if(diff == NULL) {
            addSystemMessage("You are not listening to anything.");
            return;
        }

        UString song_name = UString::format("%s - %s [%s]", diff->getArtist().c_str(), diff->getTitle().c_str(),
                                            diff->getDifficultyName().c_str());
        UString song_link = UString::format("[https://osu.%s/beatmaps/%d %s]", bancho.endpoint.toUtf8(), diff->getID(),
                                            song_name.toUtf8());

        UString np_msg;
        if(osu->isInPlayMode()) {
            np_msg = UString::format("\001ACTION is playing %s", song_link.toUtf8());

            auto mods = osu->getScore()->getMods();
            if(mods.speed != 1.f) {
                auto speed_modifier = UString::format(" x%.1f", mods.speed);
                np_msg.append(speed_modifier);
            }
            auto mod_string = ScoreButton::getModsStringForDisplay(mods);
            if(mod_string.length() > 0) {
                np_msg.append(" (+");
                np_msg.append(mod_string);
                np_msg.append(")");
            }

            np_msg.append("\001");
        } else {
            np_msg = UString::format("\001ACTION is listening to %s\001", song_link.toUtf8());
        }

        send_message(np_msg);
        return;
    }

    if(msg.startsWith("/addfriend ")) {
        auto friend_name = msg.substr(11);
        auto user = find_user(friend_name);
        if(!user) {
            addSystemMessage(UString::format("User '%s' not found. Are they online?", friend_name.toUtf8()));
            return;
        }

        if(user->is_friend()) {
            addSystemMessage(UString::format("You are already friends with %s!", friend_name.toUtf8()));
        } else {
            Packet packet;
            packet.id = FRIEND_ADD;
            write<u32>(&packet, user->user_id);
            send_packet(packet);

            friends.push_back(user->user_id);

            addSystemMessage(UString::format("You are now friends with %s.", friend_name.toUtf8()));
        }

        return;
    }

    if(msg.startsWith("/bb ")) {
        addChannel("BanchoBot", true);
        send_message(msg.substr(4));
        return;
    }

    if(msg == UString("/away")) {
        away_msg = "";
        addSystemMessage("Away message removed.");
        return;
    }
    if(msg.startsWith("/away ")) {
        away_msg = msg.substr(6);
        addSystemMessage(UString::format("Away message set to '%s'.", away_msg.toUtf8()));
        return;
    }

    if(msg.startsWith("/delfriend ")) {
        auto friend_name = msg.substr(11);
        auto user = find_user(friend_name);
        if(!user) {
            addSystemMessage(UString::format("User '%s' not found. Are they online?", friend_name.toUtf8()));
            return;
        }

        if(user->is_friend()) {
            Packet packet;
            packet.id = FRIEND_REMOVE;
            write<u32>(&packet, user->user_id);
            send_packet(packet);

            auto it = std::find(friends.begin(), friends.end(), user->user_id);
            if(it != friends.end()) {
                friends.erase(it);
            }

            addSystemMessage(UString::format("You are no longer friends with %s.", friend_name.toUtf8()));
        } else {
            addSystemMessage(UString::format("You aren't friends with %s!", friend_name.toUtf8()));
        }

        return;
    }

    if(msg.startsWith("/me ")) {
        auto new_text = msg.substr(3);
        new_text.insert(0, "\001ACTION");
        new_text.append("\001");
        send_message(new_text);
        return;
    }

    if(msg.startsWith("/chat ") || msg.startsWith("/msg ") || msg.startsWith("/query ")) {
        auto username = msg.substr(msg.find(L" "));
        addChannel(username, true);
        return;
    }

    if(msg.startsWith("/invite ")) {
        if(!bancho.is_in_a_multi_room()) {
            addSystemMessage("You are not in a multiplayer room!");
            return;
        }

        auto username = msg.substr(8);
        auto invite_msg = UString::format("\001ACTION has invited you to join [osump://%d/%s %s]\001", bancho.room.id,
                                          bancho.room.password.toUtf8(), bancho.room.name.toUtf8());

        Packet packet;
        packet.id = SEND_PRIVATE_MESSAGE;
        write_string(&packet, (char *)bancho.username.toUtf8());
        write_string(&packet, (char *)invite_msg.toUtf8());
        write_string(&packet, (char *)username.toUtf8());
        write<u32>(&packet, bancho.user_id);
        send_packet(packet);

        addSystemMessage(UString::format("%s has been invited to the game.", username.toUtf8()));
        return;
    }

    if(msg.startsWith("/j ") || msg.startsWith("/join ")) {
        auto channel = msg.substr(msg.find(L" "));
        join(channel);
        return;
    }

    if(msg.startsWith("/p ") || msg.startsWith("/part ")) {
        auto channel = msg.substr(msg.find(L" "));
        leave(channel);
        return;
    }

    addSystemMessage("This command is not supported.");
}

void Chat::onKeyDown(KeyboardEvent &key) {
    if(!m_bVisible) return;

    if(engine->getKeyboard()->isAltDown()) {
        i32 tab_select = -1;
        if(key.getKeyCode() == KEY_1) tab_select = 0;
        if(key.getKeyCode() == KEY_2) tab_select = 1;
        if(key.getKeyCode() == KEY_3) tab_select = 2;
        if(key.getKeyCode() == KEY_4) tab_select = 3;
        if(key.getKeyCode() == KEY_5) tab_select = 4;
        if(key.getKeyCode() == KEY_6) tab_select = 5;
        if(key.getKeyCode() == KEY_7) tab_select = 6;
        if(key.getKeyCode() == KEY_8) tab_select = 7;
        if(key.getKeyCode() == KEY_9) tab_select = 8;
        if(key.getKeyCode() == KEY_0) tab_select = 9;

        if(tab_select != -1) {
            if(tab_select >= m_channels.size()) {
                key.consume();
                return;
            }

            key.consume();
            switchToChannel(m_channels[tab_select]);
            return;
        }
    }

    if(key.getKeyCode() == KEY_PAGEUP) {
        if(m_selected_channel != NULL) {
            key.consume();
            m_selected_channel->ui->scrollY(getSize().y - input_box_height);
            return;
        }
    }

    if(key.getKeyCode() == KEY_PAGEDOWN) {
        if(m_selected_channel != NULL) {
            key.consume();
            m_selected_channel->ui->scrollY(-(getSize().y - input_box_height));
            return;
        }
    }

    // Escape: close chat
    if(key.getKeyCode() == KEY_ESCAPE) {
        if(isVisibilityForced()) return;

        key.consume();
        user_wants_chat = false;
        updateVisibility();
        return;
    }

    // Return: send message
    if(key.getKeyCode() == KEY_ENTER) {
        key.consume();
        if(m_selected_channel != NULL && m_input_box->getText().length() > 0) {
            if(m_input_box->getText()[0] == L'/') {
                handle_command(m_input_box->getText());
            } else {
                send_message(m_input_box->getText());
            }

            engine->getSound()->play(osu->getSkin()->m_messageSent);
            m_input_box->clear();
        }
        tab_completion_prefix = "";
        tab_completion_match = "";
        return;
    }

    // Ctrl+W: Close current channel
    if(engine->getKeyboard()->isControlDown() && key.getKeyCode() == KEY_W) {
        key.consume();
        if(m_selected_channel != NULL) {
            leave(m_selected_channel->name);
        }
        return;
    }

    // Ctrl+Tab: Switch channels
    // KEY_TAB doesn't work on Linux
    if(engine->getKeyboard()->isControlDown() && (key.getKeyCode() == 65056 || key.getKeyCode() == KEY_TAB)) {
        key.consume();
        if(m_selected_channel == NULL) return;
        int chan_index = m_channels.size();
        for(auto chan : m_channels) {
            if(chan == m_selected_channel) {
                break;
            }
            chan_index++;
        }

        if(engine->getKeyboard()->isShiftDown()) {
            // Ctrl+Shift+Tab: Go to previous channel
            auto new_chan = m_channels[(chan_index - 1) % m_channels.size()];
            switchToChannel(new_chan);
        } else {
            // Ctrl+Tab: Go to next channel
            auto new_chan = m_channels[(chan_index + 1) % m_channels.size()];
            switchToChannel(new_chan);
        }

        engine->getSound()->play(osu->getSkin()->m_clickButton);

        return;
    }

    // TAB: Complete nickname
    // KEY_TAB doesn't work on Linux
    if(key.getKeyCode() == 65056 || key.getKeyCode() == KEY_TAB) {
        key.consume();

        auto text = m_input_box->getText();
        i32 username_start_idx = text.findLast(" ", 0, m_input_box->m_iCaretPosition) + 1;
        i32 username_end_idx = m_input_box->m_iCaretPosition;
        i32 username_len = username_end_idx - username_start_idx;

        if(tab_completion_prefix.length() == 0) {
            tab_completion_prefix = text.substr(username_start_idx, username_len);
        } else {
            username_start_idx = m_input_box->m_iCaretPosition - tab_completion_match.length();
            username_len = username_end_idx - username_start_idx;
        }

        auto user = find_user_starting_with(tab_completion_prefix, tab_completion_match);
        if(user) {
            tab_completion_match = user->name;

            // Remove current username, add new username
            m_input_box->m_sText.erase(m_input_box->m_iCaretPosition - username_len, username_len);
            m_input_box->m_iCaretPosition -= username_len;
            m_input_box->m_sText.insert(m_input_box->m_iCaretPosition, tab_completion_match);
            m_input_box->m_iCaretPosition += tab_completion_match.length();
            m_input_box->setText(m_input_box->m_sText);
            m_input_box->updateTextPos();
            m_input_box->tickCaret();

            Sound *sounds[] = {osu->getSkin()->m_typing1, osu->getSkin()->m_typing2, osu->getSkin()->m_typing3,
                               osu->getSkin()->m_typing4};
            engine->getSound()->play(sounds[rand() % 4]);
        }

        return;
    }

    // Typing in chat: capture keypresses
    if(!engine->getKeyboard()->isAltDown()) {
        tab_completion_prefix = "";
        tab_completion_match = "";
        m_input_box->onKeyDown(key);
        key.consume();
        return;
    }
}

void Chat::onKeyUp(KeyboardEvent &key) {
    if(!m_bVisible || key.isConsumed()) return;

    m_input_box->onKeyUp(key);
    key.consume();
}

void Chat::onChar(KeyboardEvent &key) {
    if(!m_bVisible || key.isConsumed()) return;

    m_input_box->onChar(key);
    key.consume();
}

void Chat::mark_as_read(ChatChannel *chan) {
    if(!m_bVisible) return;

    // XXX: Only mark as read after 500ms
    chan->read = true;

    CURL *curl = curl_easy_init();
    if(!curl) {
        debugLog("Failed to initialize cURL in Chat::mark_as_read()!\n");
        return;
    }
    char *channel_urlencoded = curl_easy_escape(curl, chan->name.toUtf8(), 0);
    if(!channel_urlencoded) {
        debugLog("Failed to encode channel name!\n");
        curl_easy_cleanup(curl);
        return;
    }

    APIRequest request;
    request.type = MARK_AS_READ;
    request.path = UString::format("/web/osu-markasread.php?u=%s&h=%s&channel=%s", bancho.username.toUtf8(),
                                   bancho.pw_md5.toUtf8(), channel_urlencoded);
    request.mime = NULL;

    send_api_request(request);

    curl_free(channel_urlencoded);
    curl_easy_cleanup(curl);
}

void Chat::switchToChannel(ChatChannel *chan) {
    m_selected_channel = chan;
    if(!chan->read) {
        mark_as_read(m_selected_channel);
    }

    // Update button colors
    updateButtonLayout(getSize());
}

void Chat::addChannel(UString channel_name, bool switch_to) {
    for(auto chan : m_channels) {
        if(chan->name == channel_name) {
            if(switch_to) {
                switchToChannel(chan);
            }
            return;
        }
    }

    ChatChannel *chan = new ChatChannel(this, channel_name);
    m_channels.push_back(chan);

    if(m_selected_channel == NULL && m_channels.size() == 1) {
        switchToChannel(chan);
    } else if(channel_name == UString("#multiplayer") || channel_name == UString("#lobby")) {
        switchToChannel(chan);
    } else if(switch_to) {
        switchToChannel(chan);
    }

    updateLayout(osu->getScreenSize());

    if(isVisible()) {
        engine->getSound()->play(osu->getSkin()->m_expand);
    }
}

void Chat::addMessage(UString channel_name, ChatMessage msg, bool mark_unread) {
    bool is_pm = msg.author_id > 0 && channel_name[0] != '#' && msg.author_name != bancho.username;
    if(is_pm) {
        // If it's a PM, the channel title should be the one who sent the message
        channel_name = msg.author_name;
    }

    addChannel(channel_name);
    for(auto chan : m_channels) {
        if(chan->name != channel_name) continue;
        chan->messages.push_back(msg);
        chan->add_message(msg);

        if(mark_unread) {
            chan->read = false;
            if(chan == m_selected_channel) {
                mark_as_read(chan);
            } else {
                updateButtonLayout(getSize());
            }
        }

        if(chan->messages.size() > 100) {
            chan->messages.erase(chan->messages.begin());
        }

        break;
    }

    if(is_pm && away_msg.length() > 0) {
        Packet packet;
        packet.id = SEND_PRIVATE_MESSAGE;
        write_string(&packet, (char *)bancho.username.toUtf8());
        write_string(&packet, (char *)away_msg.toUtf8());
        write_string(&packet, (char *)msg.author_name.toUtf8());
        write<u32>(&packet, bancho.user_id);
        send_packet(packet);

        // Server doesn't echo the message back
        addMessage(channel_name, ChatMessage{
                                     .tms = time(NULL),
                                     .author_id = bancho.user_id,
                                     .author_name = bancho.username,
                                     .text = away_msg,
                                 });
    }
}

void Chat::addSystemMessage(UString msg) {
    addMessage(m_selected_channel->name, ChatMessage{
                                             .tms = time(NULL),
                                             .author_id = 0,
                                             .author_name = "",
                                             .text = msg,
                                         });
}

void Chat::removeChannel(UString channel_name) {
    ChatChannel *chan = NULL;
    for(auto c : m_channels) {
        if(c->name == channel_name) {
            chan = c;
            break;
        }
    }

    if(chan == NULL) return;

    auto it = std::find(m_channels.begin(), m_channels.end(), chan);
    m_channels.erase(it);
    if(m_selected_channel == chan) {
        m_selected_channel = NULL;
        if(!m_channels.empty()) {
            switchToChannel(m_channels[0]);
        }
    }

    delete chan;
    updateButtonLayout(getSize());
}

void Chat::updateLayout(Vector2 newResolution) {
    // We don't want to update while the chat is hidden, to avoid lagspikes during gameplay
    if(!m_bVisible) {
        layout_update_scheduled = true;
        return;
    }

    // In the lobby and in multi rooms don't take the full horizontal width to allow for cleaner UI designs.
    if(isSmallChat()) {
        newResolution.x = round(newResolution.x * 0.6);
    }

    setSize(newResolution);

    const float chat_w = newResolution.x;
    const float chat_h = round(newResolution.y * 0.3f) - input_box_height;
    const float chat_y = newResolution.y - (chat_h + input_box_height);
    for(auto chan : m_channels) {
        chan->updateLayout(Vector2{0.f, chat_y}, Vector2{chat_w, chat_h});
    }

    m_input_box->setPos(Vector2{0.f, chat_y + chat_h});
    m_input_box->setSize(Vector2{chat_w, input_box_height});

    if(m_selected_channel == NULL && !m_channels.empty()) {
        m_selected_channel = m_channels[0];
        m_selected_channel->read = true;
    }

    updateButtonLayout(newResolution);
    updateButtonLayout(newResolution);  // idk
    layout_update_scheduled = false;
}

void Chat::updateButtonLayout(Vector2 screen) {
    const float initial_x = 2;
    float total_x = initial_x;

    std::sort(m_channels.begin(), m_channels.end(), [](ChatChannel *a, ChatChannel *b) { return a->name < b->name; });

    // Look, I really tried. But for some reason setPos() doesn't work until we change
    // the screen resolution once. So I'll just compute absolute position manually.
    float button_container_height = button_height + 2;
    for(auto chan : m_channels) {
        UIButton *btn = chan->btn;
        float button_width = font->getStringWidth(btn->getText()) + 20;

        // Wrap channel buttons
        if(total_x + button_width > screen.x - 20) {
            total_x = initial_x;
            button_container_height += button_height;
        }

        total_x += button_width;
    }

    const float chat_y = round(screen.y * 0.7f);
    float total_y = 0.f;
    total_x = initial_x;
    for(auto chan : m_channels) {
        UIButton *btn = chan->btn;
        float button_width = font->getStringWidth(btn->getText()) + 20;

        // Wrap channel buttons
        if(total_x + button_width > screen.x - 20) {
            total_x = initial_x;
            total_y += button_height;
        }

        btn->setPos(total_x, chat_y - button_container_height + total_y);
        // Buttons are drawn a bit smaller than they should, so we up the size here
        btn->setSize(button_width + 2, button_height + 2);

        if(m_selected_channel->name == btn->getText()) {
            btn->setColor(0xfffefffd);
        } else {
            if(chan->read) {
                btn->setColor(0xff38439f);
            } else {
                btn->setColor(0xff88a0f7);
            }
        }

        total_x += button_width;
    }

    join_channel_btn->setPos(total_x, chat_y - button_container_height + total_y);
    m_button_container->setPos(0, chat_y - button_container_height);
    m_button_container->setSize(screen.x, button_container_height);
}

void Chat::join(UString channel_name) {
    // XXX: Open the channel immediately, without letting the user send messages in it.
    //      Would require a way to signal if a channel is joined or not.
    //      Would allow to keep open the tabs of the channels we got kicked out of.
    Packet packet;
    packet.id = CHANNEL_JOIN;
    write_string(&packet, channel_name.toUtf8());
    send_packet(packet);
}

void Chat::leave(UString channel_name) {
    bool send_leave_packet = channel_name[0] == '#';
    if(channel_name == UString("#lobby")) send_leave_packet = false;
    if(channel_name == UString("#multiplayer")) send_leave_packet = false;

    if(send_leave_packet) {
        Packet packet;
        packet.id = CHANNEL_PART;
        write_string(&packet, channel_name.toUtf8());
        send_packet(packet);
    }

    removeChannel(channel_name);

    engine->getSound()->play(osu->getSkin()->m_closeChatTab);
}

void Chat::send_message(UString msg) {
    Packet packet;
    packet.id = m_selected_channel->name[0] == '#' ? SEND_PUBLIC_MESSAGE : SEND_PRIVATE_MESSAGE;
    write_string(&packet, (char *)bancho.username.toUtf8());
    write_string(&packet, (char *)msg.toUtf8());
    write_string(&packet, (char *)m_selected_channel->name.toUtf8());
    write<u32>(&packet, bancho.user_id);
    send_packet(packet);

    // Server doesn't echo the message back
    addMessage(m_selected_channel->name, ChatMessage{
                                             .tms = time(NULL),
                                             .author_id = bancho.user_id,
                                             .author_name = bancho.username,
                                             .text = msg,
                                         });
}

void Chat::onDisconnect() {
    for(auto chan : m_channels) {
        delete chan;
    }
    m_channels.clear();

    for(auto chan : chat_channels) {
        delete chan.second;
    }
    chat_channels.clear();

    m_selected_channel = NULL;
    updateLayout(osu->getScreenSize());

    updateVisibility();
}

void Chat::onResolutionChange(Vector2 newResolution) { updateLayout(newResolution); }

bool Chat::isSmallChat() {
    if(osu->m_room == NULL || osu->m_lobby == NULL || osu->m_songBrowser2 == NULL) return false;
    bool sitting_in_room =
        osu->m_room->isVisible() && !osu->m_songBrowser2->isVisible() && !bancho.is_playing_a_multi_map();
    bool sitting_in_lobby = osu->m_lobby->isVisible();
    return sitting_in_room || sitting_in_lobby;
}

bool Chat::isVisibilityForced() {
    bool is_forced = (isSmallChat() || osu->m_spectatorScreen->isVisible());
    if(is_forced != visibility_was_forced) {
        // Chat width changed: update the layout now
        visibility_was_forced = is_forced;
        updateLayout(osu->getScreenSize());
    }
    return is_forced;
}

void Chat::updateVisibility() {
    auto selected_beatmap = osu->getSelectedBeatmap();
    bool can_skip = (selected_beatmap != NULL) && (selected_beatmap->isInSkippableSection());
    bool is_spectating = osu->m_bModAuto || (osu->m_bModAutopilot && osu->m_bModRelax) ||
                         (selected_beatmap != NULL && selected_beatmap->is_watching) || bancho.spectated_player_id != 0;
    bool is_clicking_circles = osu->isInPlayMode() && !can_skip && !is_spectating && !osu->m_pauseMenu->isVisible();
    if(bancho.is_playing_a_multi_map() && !bancho.room.all_players_loaded) {
        is_clicking_circles = false;
    }
    bool force_hide = osu->m_optionsMenu->isVisible() || osu->m_modSelector->isVisible() || is_clicking_circles;
    if(!bancho.is_online()) force_hide = true;

    if(force_hide) {
        setVisible(false);
    } else if(isVisibilityForced()) {
        setVisible(true);
    } else {
        setVisible(user_wants_chat);
    }
}

CBaseUIContainer *Chat::setVisible(bool visible) {
    if(visible == m_bVisible) return this;

    engine->getSound()->play(osu->getSkin()->m_clickButton);

    if(visible && bancho.user_id <= 0) {
        osu->m_optionsMenu->askForLoginDetails();
        return this;
    }

    m_bVisible = visible;
    if(visible) {
        osu->m_optionsMenu->setVisible(false);
        anim->moveQuartOut(&m_fAnimation, 1.0f, 0.25f * (1.0f - m_fAnimation), true);

        if(m_selected_channel != NULL && !m_selected_channel->read) {
            mark_as_read(m_selected_channel);
        }

        if(layout_update_scheduled) {
            updateLayout(osu->getScreenSize());
        }
    } else {
        anim->moveQuadOut(&m_fAnimation, 0.0f, 0.25f * m_fAnimation, true);
    }

    return this;
}

bool Chat::isMouseInChat() {
    if(!isVisible()) return false;
    if(m_selected_channel == NULL) return false;
    return m_input_box->isMouseInside() || m_selected_channel->ui->isMouseInside();
}

void Chat::askWhatChannelToJoin(CBaseUIButton *btn) {
    // XXX: Could display nicer UI with full channel list (chat_channels in Bancho.cpp)
    osu->m_prompt->prompt("Type in the channel you want to join (e.g. '#osu'):",
                          fastdelegate::MakeDelegate(this, &Chat::join));
}
