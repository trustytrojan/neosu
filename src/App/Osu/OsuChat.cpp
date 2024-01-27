#include "OsuChat.h"

#include "AnimationHandler.h"
#include "CBaseUIButton.h"
#include "CBaseUIContainer.h"
#include "CBaseUILabel.h"
#include "Engine.h"
#include "Font.h"
#include "Keyboard.h"
#include "RenderTarget.h"
#include "ResourceManager.h"
#include "SoundEngine.h"

#include "Bancho.h"
#include "BanchoNetworking.h"
#include "Osu.h"
#include "OsuBeatmap.h"
#include "OsuLobby.h"
#include "OsuOptionsMenu.h"
#include "OsuPauseMenu.h"
#include "OsuRoom.h"
#include "OsuSkin.h"
#include "OsuSongBrowser2.h"
#include "OsuUIButton.h"


OsuChatChannel::OsuChatChannel(OsuChat* chat, UString name_arg) {
    m_chat = chat;
    name = name_arg;

    ui = new CBaseUIScrollView(0, 0, 0, 0, "");
    ui->setDrawFrame(false);
    ui->setDrawBackground(true);
    ui->setBackgroundColor(0xdd000000);
    ui->setHorizontalScrolling(false);
    ui->setDrawScrollbars(true);

    btn = new OsuUIButton(bancho.osu, 0, 0, 0, 0, "button", name);
    btn->setUseDefaultSkin();
    btn->setClickCallback( fastdelegate::MakeDelegate(this, &OsuChatChannel::onChannelButtonClick) );
    m_chat->m_button_container->addBaseUIElement(btn);
}

OsuChatChannel::~OsuChatChannel() {
    delete ui;
    m_chat->m_button_container->deleteBaseUIElement(btn);
}

void OsuChatChannel::onChannelButtonClick(CBaseUIButton* btn) {
    m_chat->switchToChannel(this);
}

void OsuChatChannel::add_message(ChatMessage msg) {
    const float line_height = 20;

    // TODO @kiwec: this stops following if chat is too fast :/
    bool was_at_bottom = (ui->getSize().y - ui->getScrollPosY()) >= ui->getScrollSize().y;

    bool is_system_message = msg.author_name.length() == 0;

    UString text_str = "";
    float name_width = m_chat->font->getStringWidth(msg.author_name);
    if(!is_system_message) {
        CBaseUILabel *username = new CBaseUILabel(10, y_total, name_width, line_height, "", msg.author_name);
        username->setDrawFrame(false);
        username->setDrawBackground(false);
        username->setTextColor(0xff2596be);
        ui->getContainer()->addBaseUIElement(username);

        // TODO @kiwec: make name clickable, do something when name is clicked

        text_str.append(": ");
    }

    text_str.append(msg.text);
    float text_width = m_chat->font->getStringWidth(text_str);
    float text_height = line_height; // TODO @kiwec: text wrapping

    CBaseUILabel *text = new CBaseUILabel(10 + name_width, y_total, text_width, text_height, "", text_str);
    text->setDrawFrame(false);
    text->setDrawBackground(false);
    if(is_system_message) {
        // TODO @kiwec: use prettier color
        text->setTextColor(0xfff73729);
    }
    ui->getContainer()->addBaseUIElement(text);

    y_total += line_height;
    ui->setScrollSizeToContent();

    if(was_at_bottom) {
        ui->scrollToBottom();
    }
}

void OsuChatChannel::updateLayout(Vector2 pos, Vector2 size) {
    ui->clear();
    ui->setPos(pos);
    ui->setSize(size);
    y_total = 7;

    for(auto msg : messages) {
        add_message(msg);
    }
}

OsuChat::OsuChat(Osu *osu) : OsuScreen(osu) {
    font = engine->getResourceManager()->getFont("FONT_DEFAULT");

    m_container = new CBaseUIContainer(0, 0, 0, 0, "");
    m_button_container = new CBaseUIContainer(0, 0, 0, 0, "");

    m_input_box = new CBaseUITextbox(0, 0, 0, 0, "");
    m_input_box->setDrawFrame(false);
    m_input_box->setDrawBackground(true);
    m_input_box->setBackgroundColor(0xdd000000);
    m_container->addBaseUIElement(m_input_box);

    updateLayout(m_osu->getScreenSize());
}

void OsuChat::draw(Graphics *g) {
    const bool isAnimating = anim->isAnimating(&m_fAnimation);
    if (!m_bVisible && !isAnimating) return;

    if (isAnimating) {
        m_osu->getSliderFrameBuffer()->enable();
        g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_PREMUL_ALPHA);
    }

    if(m_selected_channel == nullptr) {
        const float chat_y = m_container->getSize().y * 0.66f;
        const float chat_height = m_container->getSize().y - chat_y;

        g->setColor(COLOR(isAnimating ? 150 : 100, 0, 10, 50)); // dirty hack, idk how opengl works
        g->fillRect(0, chat_y - 20, m_container->getSize().x, 20);
        g->setColor(COLOR(150, 0, 0, 0));
        g->fillRect(0, chat_y, m_container->getSize().x, chat_height);
    } else {
        g->setColor(COLOR(isAnimating ? 150 : 100, 0, 10, 50)); // dirty hack, idk how opengl works
        g->fillRect(
            m_button_container->getPos().x, m_button_container->getPos().y,
            m_button_container->getSize().x, m_button_container->getSize().y
        );
        m_button_container->draw(g);

        m_container->draw(g);
        if(m_selected_channel != nullptr) {
            m_selected_channel->ui->draw(g);
        }
    }

    if (isAnimating) {
        g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);
        m_osu->getSliderFrameBuffer()->disable();

        g->push3DScene(McRect(0, 0, m_container->getSize().x, m_container->getSize().y));
        {
            g->rotate3DScene(-(1.0f - m_fAnimation)*90, 0, 0);
            g->translate3DScene(0, -(1.0f - m_fAnimation)*m_container->getSize().y*1.25f, -(1.0f - m_fAnimation)*700);

            m_osu->getSliderFrameBuffer()->setColor(COLORf(m_fAnimation, 1.0f, 1.0f, 1.0f));
            m_osu->getSliderFrameBuffer()->draw(g, 0, 0);
        }
        g->pop3DScene();
    }
}

void OsuChat::mouse_update(bool *propagate_clicks) {
    if (!m_bVisible) return;
    m_container->mouse_update(propagate_clicks);
    m_button_container->mouse_update(propagate_clicks);
    if(m_selected_channel) {
        m_selected_channel->ui->mouse_update(propagate_clicks);
    }
    m_input_box->focus();
}

void OsuChat::onKeyDown(KeyboardEvent &key) {
    if(!m_bVisible) return;

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
        if(m_selected_channel != nullptr && m_input_box->getText().length() > 0) {
            Packet packet = {0};
            packet.id = m_selected_channel->name[0] == '#' ? SEND_PUBLIC_MESSAGE : SEND_PRIVATE_MESSAGE;
            write_string(&packet, (char*)bancho.username.toUtf8());
            write_string(&packet, (char*)m_input_box->getText().toUtf8());
            write_string(&packet, (char*)m_selected_channel->name.toUtf8());
            write_int(&packet, bancho.user_id);
            send_packet(packet);

            // Server doesn't echo the message back
            addMessage(m_selected_channel->name, ChatMessage{
                .author_id = bancho.user_id,
                .author_name = bancho.username,
                .text = m_input_box->getText(),
            });

            // TODO @kiwec: handle cases where the message is rejected

            m_input_box->clear();
        }
        return;
    }

    // Ctrl+W: Close current channel
    if(engine->getKeyboard()->isControlDown() && key.getKeyCode() == KEY_W) {
        key.consume();
        if(m_selected_channel != nullptr && m_selected_channel->name != UString("#lobby")) {
            if(m_selected_channel->name[0] == '#') {
                Packet packet = {0};
                packet.id = CHANNEL_PART;
                write_string(&packet, m_selected_channel->name.toUtf8());
                send_packet(packet);
            }

            removeChannel(m_selected_channel->name);
        }
        return;
    }

    // Ctrl+Tab: Switch channels
    // KEY_TAB doesn't work... idk why
    if(engine->getKeyboard()->isControlDown() && key.getKeyCode() == 65056) {
        key.consume();
        if(m_selected_channel == nullptr) return;
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
        return;
    }

    // Typing in chat: capture keypresses
    if(!engine->getKeyboard()->isControlDown() && !engine->getKeyboard()->isAltDown()) {
        m_input_box->onKeyDown(key);
        key.consume();
        return;
    }
}

void OsuChat::onKeyUp(KeyboardEvent &key) {
    if(!m_bVisible || key.isConsumed()) return;

    m_input_box->onKeyUp(key);
    key.consume();
}

void OsuChat::onChar(KeyboardEvent &key) {
    if(!m_bVisible || key.isConsumed()) return;

    m_input_box->onChar(key);
    key.consume();
}

void OsuChat::switchToChannel(OsuChatChannel* chan) {
    m_selected_channel = chan;
    m_selected_channel->read = true;

    // Update button colors
    updateButtonLayout(m_container->getSize());
}

void OsuChat::addChannel(UString channel_name) {
    for(auto chan : m_channels) {
        if(chan->name == channel_name) return;
    }

    OsuChatChannel* chan = new OsuChatChannel(this, channel_name);
    m_channels.push_back(chan);

    if(m_selected_channel == nullptr && m_channels.size() == 1) {
        switchToChannel(chan);
    } else if(channel_name == UString("#multiplayer") || channel_name == UString("#lobby")) {
        switchToChannel(chan);
    }

    updateLayout(engine->getScreenSize());
}

void OsuChat::addMessage(UString channel_name, ChatMessage msg) {
    if(msg.author_id > 0 && channel_name[0] != '#' && msg.author_name != bancho.username) {
        // If it's a PM, the channel title should be the one who sent the message
        channel_name = msg.author_name;
    }

    addChannel(channel_name);
    for(auto chan : m_channels) {
        if(chan->name != channel_name) continue;
        chan->messages.push_back(msg);
        chan->add_message(msg);
        if(chan != m_selected_channel && chan->read) {
            chan->read = false;
            updateButtonLayout(m_container->getSize());
        }
        return;
    }
}

void OsuChat::removeChannel(UString channel_name) {
    OsuChatChannel* chan = nullptr;
    for(auto c : m_channels) {
        if(c->name == channel_name) {
            chan = c;
            break;
        }
    }

    if(chan == nullptr) return;

    auto it = std::find(m_channels.begin(), m_channels.end(), chan);
    m_channels.erase(it);
    if(m_selected_channel == chan) {
        m_selected_channel = nullptr;
        if(!m_channels.empty()) {
            switchToChannel(m_channels[0]);
        }
    }

    delete chan;
    updateButtonLayout(m_container->getSize());
}

void OsuChat::updateLayout(Vector2 newResolution) {
    m_container->setSize(newResolution);

    const float chat_height = newResolution.y * 0.66f;
    const float input_box_height = 30.f;

    for(auto chan : m_channels) {
        chan->updateLayout(
            Vector2{0.f, chat_height},
            Vector2{newResolution.x, (newResolution.y - chat_height) - input_box_height}
        );
    }

    m_input_box->setPos(Vector2{0.f, newResolution.y - (input_box_height + 2.f)});
    m_input_box->setSize(Vector2{newResolution.x, input_box_height});

    if(m_selected_channel == nullptr && !m_channels.empty()) {
        m_selected_channel = m_channels[0];
        m_selected_channel->read = true;
    }

    updateButtonLayout(newResolution);
    updateButtonLayout(newResolution); // idk
}

void OsuChat::updateButtonLayout(Vector2 screen) {
    const float initial_x = 2;
    const float button_height = 26;
    float total_x = initial_x;

    std::sort(m_channels.begin(), m_channels.end(), [](OsuChatChannel* a, OsuChatChannel* b) {
        return a->name < b->name;
    });

    // Look, I really tried. But for some reason setRelPos() doesn't work until we change
    // the screen resolution once. So I'll just compute absolute position manually.
    float button_container_height = button_height + 2;
    for(auto chan : m_channels) {
        OsuUIButton* btn = chan->btn;
        float button_width = font->getStringWidth(btn->getText()) + 20;

        // Wrap channel buttons
        if(total_x + button_width > screen.x) {
            total_x = initial_x;
            button_container_height += button_height;
        }

        total_x += button_width;
    }

    const float chat_height = screen.y * 0.66f;
    float total_y = 0.f;
    total_x = initial_x;
    for(auto chan : m_channels) {
        OsuUIButton* btn = chan->btn;
        float button_width = font->getStringWidth(btn->getText()) + 20;

        // Wrap channel buttons
        if(total_x + button_width > screen.x) {
            total_x = initial_x;
            total_y += button_height;
        }

        btn->setPos(total_x, chat_height - button_container_height + total_y);
        // Buttons are drawn a bit smaller than they should, so we up the size here
        btn->setSize(button_width + 2, button_height + 2);

        if(m_selected_channel->name == btn->getText()) {
            btn->setColor(0xfffefffd);
        } else {
            if(chan->read) {
                btn->setColor(0xff38439f);
            } else {
                btn->setColor(0xff5c7ffe);
            }
        }

        total_x += button_width;
    }

    m_button_container->setPos(0, chat_height - button_container_height);
    m_button_container->setSize(screen.x, button_container_height);
}

void OsuChat::onResolutionChange(Vector2 newResolution) {
    updateLayout(newResolution);
}

bool OsuChat::isVisibilityForced() {
    bool sitting_in_room = m_osu->m_room->isVisible() && !m_osu->m_songBrowser2->isVisible() && !bancho.is_playing_a_multi_map();
    return m_osu->m_lobby->isVisible() || sitting_in_room;
}

void OsuChat::updateVisibility() {
    if(isVisibilityForced()) {
        setVisible(true);
        return;
    }

    auto selected_beatmap = m_osu->getSelectedBeatmap();
    bool can_skip = (selected_beatmap != nullptr) && (selected_beatmap->isInSkippableSection());
    bool is_clicking_circles = m_osu->isInPlayMode() && !can_skip && !m_osu->m_bModAuto && !m_osu->m_pauseMenu->isVisible();
    bool force_hide = m_osu->m_optionsMenu->isVisible() || is_clicking_circles;
    if(force_hide) {
        setVisible(false);
        return;
    }

    setVisible(user_wants_chat);
}

void OsuChat::setVisible(bool visible) {
    if(visible == m_bVisible) return;

    if(visible && bancho.user_id <= 0) {
        m_osu->m_optionsMenu->askForLoginDetails();
        return;
    }

    m_bVisible = visible;
    if(visible) {
        m_osu->m_optionsMenu->setVisible(false);
        anim->moveQuartOut(&m_fAnimation, 1.0f, 0.25f*(1.0f - m_fAnimation), true);
    } else {
        anim->moveQuadOut(&m_fAnimation, 0.0f, 0.25f*m_fAnimation, true);
    }
}
