#include "PauseMenu.h"

#include "AnimationHandler.h"
#include "Bancho.h"
#include "Beatmap.h"
#include "CBaseUIContainer.h"
#include "Chat.h"
#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"
#include "HUD.h"
#include "KeyBindings.h"
#include "Keyboard.h"
#include "ModSelector.h"
#include "Mouse.h"
#include "OptionsMenu.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "RichPresence.h"
#include "Skin.h"
#include "SoundEngine.h"
#include "UIPauseMenuButton.h"

PauseMenu::PauseMenu() : OsuScreen() {
    m_bScheduledVisibility = false;
    m_bScheduledVisibilityChange = false;

    m_selectedButton = NULL;
    m_fWarningArrowsAnimStartTime = 0.0f;
    m_fWarningArrowsAnimAlpha = 0.0f;
    m_fWarningArrowsAnimX = 0.0f;
    m_fWarningArrowsAnimY = 0.0f;
    m_bInitialWarningArrowFlyIn = true;

    m_bContinueEnabled = true;
    m_bClick1Down = false;
    m_bClick2Down = false;

    m_fDimAnim = 0.0f;

    setSize(osu->getScreenWidth(), osu->getScreenHeight());

    UIPauseMenuButton *continueButton =
        addButton([]() -> Image * { return osu->getSkin()->getPauseContinue(); }, "Resume");
    UIPauseMenuButton *retryButton = addButton([]() -> Image * { return osu->getSkin()->getPauseRetry(); }, "Retry");
    UIPauseMenuButton *backButton = addButton([]() -> Image * { return osu->getSkin()->getPauseBack(); }, "Quit");

    continueButton->setClickCallback(fastdelegate::MakeDelegate(this, &PauseMenu::onContinueClicked));
    retryButton->setClickCallback(fastdelegate::MakeDelegate(this, &PauseMenu::onRetryClicked));
    backButton->setClickCallback(fastdelegate::MakeDelegate(this, &PauseMenu::onBackClicked));

    updateLayout();
}

void PauseMenu::draw(Graphics *g) {
    const bool isAnimating = anim->isAnimating(&m_fDimAnim);
    if(!m_bVisible && !isAnimating) return;

    // draw dim
    if(cv_pause_dim_background.getBool()) {
        g->setColor(COLORf(m_fDimAnim * cv_pause_dim_alpha.getFloat(), 0.078f, 0.078f, 0.078f));
        g->fillRect(0, 0, osu->getScreenWidth(), osu->getScreenHeight());
    }

    // draw background image
    if((m_bVisible || isAnimating)) {
        Image *image = NULL;
        if(m_bContinueEnabled)
            image = osu->getSkin()->getPauseOverlay();
        else
            image = osu->getSkin()->getFailBackground();

        if(image != osu->getSkin()->getMissingTexture()) {
            const float scale = Osu::getImageScaleToFillResolution(image, osu->getScreenSize());
            const Vector2 centerTrans = (osu->getScreenSize() / 2);

            g->setColor(COLORf(m_fDimAnim, 1.0f, 1.0f, 1.0f));
            g->pushTransform();
            {
                g->scale(scale, scale);
                g->translate((int)centerTrans.x, (int)centerTrans.y);
                g->drawImage(image);
            }
            g->popTransform();
        }
    }

    // draw buttons
    for(int i = 0; i < m_buttons.size(); i++) {
        m_buttons[i]->setAlpha(1.0f - (1.0f - m_fDimAnim) * (1.0f - m_fDimAnim) * (1.0f - m_fDimAnim));
    }
    OsuScreen::draw(g);

    // draw selection arrows
    if(m_selectedButton != NULL) {
        const Color arrowColor = COLOR(255, 0, 114, 255);
        float animation = fmod((float)(engine->getTime() - m_fWarningArrowsAnimStartTime) * 3.2f, 2.0f);
        if(animation > 1.0f) animation = 2.0f - animation;

        animation = -animation * (animation - 2);  // quad out
        const float offset = osu->getUIScale(20.0f + 45.0f * animation);

        g->setColor(arrowColor);
        g->setAlpha(m_fWarningArrowsAnimAlpha * m_fDimAnim);
        osu->getHUD()->drawWarningArrow(g,
                                        Vector2(m_fWarningArrowsAnimX, m_fWarningArrowsAnimY) +
                                            Vector2(0, m_selectedButton->getSize().y / 2) - Vector2(offset, 0),
                                        false, false);
        osu->getHUD()->drawWarningArrow(g,
                                        Vector2(osu->getScreenWidth() - m_fWarningArrowsAnimX, m_fWarningArrowsAnimY) +
                                            Vector2(0, m_selectedButton->getSize().y / 2) + Vector2(offset, 0),
                                        true, false);
    }
}

void PauseMenu::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible) return;

    // update and focus handling
    OsuScreen::mouse_update(propagate_clicks);

    if(m_bScheduledVisibilityChange) {
        m_bScheduledVisibilityChange = false;
        setVisible(m_bScheduledVisibility);
    }

    if(anim->isAnimating(&m_fWarningArrowsAnimX)) m_fWarningArrowsAnimStartTime = engine->getTime();
}

void PauseMenu::onContinueClicked() {
    if(!m_bContinueEnabled) return;
    if(anim->isAnimating(&m_fDimAnim)) return;

    engine->getSound()->play(osu->getSkin()->m_clickPauseContinue);
    osu->getSelectedBeatmap()->pause();

    scheduleVisibilityChange(false);
}

void PauseMenu::onRetryClicked() {
    if(anim->isAnimating(&m_fDimAnim)) return;

    engine->getSound()->play(osu->getSkin()->m_clickPauseRetry);
    osu->getSelectedBeatmap()->restart();

    scheduleVisibilityChange(false);
}

void PauseMenu::onBackClicked() {
    if(anim->isAnimating(&m_fDimAnim)) return;

    engine->getSound()->play(osu->getSkin()->m_clickPauseBack);
    osu->getSelectedBeatmap()->stop();

    scheduleVisibilityChange(false);
}

void PauseMenu::onSelectionChange() {
    if(m_selectedButton != NULL) {
        if(m_bInitialWarningArrowFlyIn) {
            m_bInitialWarningArrowFlyIn = false;

            m_fWarningArrowsAnimY = m_selectedButton->getPos().y;
            m_fWarningArrowsAnimX = m_selectedButton->getPos().x - osu->getUIScale(170.0f);

            anim->moveLinear(&m_fWarningArrowsAnimAlpha, 1.0f, 0.3f);
            anim->moveQuadIn(&m_fWarningArrowsAnimX, m_selectedButton->getPos().x, 0.3f);
        } else
            m_fWarningArrowsAnimX = m_selectedButton->getPos().x;

        anim->moveQuadOut(&m_fWarningArrowsAnimY, m_selectedButton->getPos().y, 0.1f);
    }
}

void PauseMenu::onKeyDown(KeyboardEvent &e) {
    OsuScreen::onKeyDown(e);  // only used for options menu
    if(!m_bVisible || e.isConsumed()) return;

    if(e == (KEYCODE)cv_LEFT_CLICK.getInt() || e == (KEYCODE)cv_RIGHT_CLICK.getInt() ||
       e == (KEYCODE)cv_LEFT_CLICK_2.getInt() || e == (KEYCODE)cv_RIGHT_CLICK_2.getInt()) {
        bool fireButtonClick = false;
        if((e == (KEYCODE)cv_LEFT_CLICK.getInt() || e == (KEYCODE)cv_LEFT_CLICK_2.getInt()) && !m_bClick1Down) {
            m_bClick1Down = true;
            fireButtonClick = true;
        }
        if((e == (KEYCODE)cv_RIGHT_CLICK.getInt() || e == (KEYCODE)cv_RIGHT_CLICK_2.getInt()) && !m_bClick2Down) {
            m_bClick2Down = true;
            fireButtonClick = true;
        }
        if(fireButtonClick) {
            for(int i = 0; i < m_buttons.size(); i++) {
                if(m_buttons[i]->isMouseInside()) {
                    m_buttons[i]->click();
                    break;
                }
            }
        }
    }

    // handle arrow keys selection
    if(m_buttons.size() > 0) {
        if(!engine->getKeyboard()->isAltDown() && e == KEY_DOWN) {
            UIPauseMenuButton *nextSelectedButton = m_buttons[0];

            // get first visible button
            for(int i = 0; i < m_buttons.size(); i++) {
                if(!m_buttons[i]->isVisible()) continue;

                nextSelectedButton = m_buttons[i];
                break;
            }

            // next selection logic
            bool next = false;
            for(int i = 0; i < m_buttons.size(); i++) {
                if(!m_buttons[i]->isVisible()) continue;

                if(next) {
                    nextSelectedButton = m_buttons[i];
                    break;
                }
                if(m_selectedButton == m_buttons[i]) next = true;
            }
            m_selectedButton = nextSelectedButton;
            onSelectionChange();
        }

        if(!engine->getKeyboard()->isAltDown() && e == KEY_UP) {
            UIPauseMenuButton *nextSelectedButton = m_buttons[m_buttons.size() - 1];

            // get first visible button
            for(int i = m_buttons.size() - 1; i >= 0; i--) {
                if(!m_buttons[i]->isVisible()) continue;

                nextSelectedButton = m_buttons[i];
                break;
            }

            // next selection logic
            bool next = false;
            for(int i = m_buttons.size() - 1; i >= 0; i--) {
                if(!m_buttons[i]->isVisible()) continue;

                if(next) {
                    nextSelectedButton = m_buttons[i];
                    break;
                }
                if(m_selectedButton == m_buttons[i]) next = true;
            }
            m_selectedButton = nextSelectedButton;
            onSelectionChange();
        }

        if(m_selectedButton != NULL && e == KEY_ENTER) m_selectedButton->click();
    }

    // consume ALL events, except for a few special binds which are allowed through (e.g. for unpause or changing the
    // local offset in Osu.cpp)
    if(e != KEY_ESCAPE && e != (KEYCODE)cv_GAME_PAUSE.getInt() && e != (KEYCODE)cv_INCREASE_LOCAL_OFFSET.getInt() &&
       e != (KEYCODE)cv_DECREASE_LOCAL_OFFSET.getInt())
        e.consume();
}

void PauseMenu::onKeyUp(KeyboardEvent &e) {
    if(e == (KEYCODE)cv_LEFT_CLICK.getInt() || e == (KEYCODE)cv_LEFT_CLICK_2.getInt()) m_bClick1Down = false;

    if(e == (KEYCODE)cv_RIGHT_CLICK.getInt() || e == (KEYCODE)cv_RIGHT_CLICK_2.getInt()) m_bClick2Down = false;
}

void PauseMenu::onChar(KeyboardEvent &e) {
    if(!m_bVisible) return;

    e.consume();
}

void PauseMenu::scheduleVisibilityChange(bool visible) {
    if(visible != m_bVisible) {
        m_bScheduledVisibilityChange = true;
        m_bScheduledVisibility = visible;
    }

    // HACKHACK:
    if(!visible) setContinueEnabled(true);
}

void PauseMenu::updateLayout() {
    const float height = (osu->getScreenHeight() / (float)m_buttons.size());
    const float half = (m_buttons.size() - 1) / 2.0f;

    float maxWidth = 0.0f;
    float maxHeight = 0.0f;
    for(int i = 0; i < m_buttons.size(); i++) {
        Image *img = m_buttons[i]->getImage();
        if(img == NULL) img = osu->getSkin()->getMissingTexture();

        const float scale = osu->getUIScale(256) / (411.0f * (osu->getSkin()->isPauseContinue2x() ? 2.0f : 1.0f));

        m_buttons[i]->setBaseScale(scale, scale);
        m_buttons[i]->setSize(img->getWidth() * scale, img->getHeight() * scale);

        if(m_buttons[i]->getSize().x > maxWidth) maxWidth = m_buttons[i]->getSize().x;
        if(m_buttons[i]->getSize().y > maxHeight) maxHeight = m_buttons[i]->getSize().y;
    }

    for(int i = 0; i < m_buttons.size(); i++) {
        Vector2 newPos =
            Vector2(osu->getScreenWidth() / 2.0f - maxWidth / 2, (i + 1) * height - height / 2.0f - maxHeight / 2.0f);

        const float pinch = max(0.0f, (height / 2.0f - maxHeight / 2.0f));
        if((float)i < half)
            newPos.y += pinch;
        else if((float)i > half)
            newPos.y -= pinch;

        m_buttons[i]->setPos(newPos);
        m_buttons[i]->setSize(maxWidth, maxHeight);
    }

    onSelectionChange();
}

void PauseMenu::onResolutionChange(Vector2 newResolution) {
    setSize(newResolution);
    updateLayout();
}

CBaseUIContainer *PauseMenu::setVisible(bool visible) {
    m_bVisible = visible;

    if(osu->isInPlayMode()) {
        setContinueEnabled(!osu->getSelectedBeatmap()->hasFailed());
    } else {
        setContinueEnabled(true);
    }

    if(visible) {
        engine->getSound()->play(osu->getSkin()->m_pauseLoop);

        if(m_bContinueEnabled) {
            RichPresence::setStatus("Paused");
            RichPresence::setBanchoStatus("Taking a break", PAUSED);

            if(!bancho.spectators.empty()) {
                Packet packet;
                packet.id = OUT_SPECTATE_FRAMES;
                write<i32>(&packet, 0);
                write<u16>(&packet, 0);
                write<u8>(&packet, LiveReplayBundle::Action::PAUSE);
                write<ScoreFrame>(&packet, ScoreFrame::get());
                write<u16>(&packet, osu->getSelectedBeatmap()->spectator_sequence++);
                send_packet(packet);
            }
        } else {
            RichPresence::setBanchoStatus("Failed", SUBMITTING);
        }
    } else {
        engine->getSound()->stop(osu->getSkin()->m_pauseLoop);

        RichPresence::onPlayStart();

        if(!bancho.spectators.empty()) {
            Packet packet;
            packet.id = OUT_SPECTATE_FRAMES;
            write<i32>(&packet, 0);
            write<u16>(&packet, 0);
            write<u8>(&packet, LiveReplayBundle::Action::UNPAUSE);
            write<ScoreFrame>(&packet, ScoreFrame::get());
            write<u16>(&packet, osu->getSelectedBeatmap()->spectator_sequence++);
            send_packet(packet);
        }
    }

    // HACKHACK: force disable mod selection screen in case it was open and the beatmap ended/failed
    osu->getModSelector()->setVisible(false);

    // reset
    m_selectedButton = NULL;
    m_bInitialWarningArrowFlyIn = true;
    m_fWarningArrowsAnimAlpha = 0.0f;
    m_bScheduledVisibility = visible;
    m_bScheduledVisibilityChange = false;

    if(m_bVisible) updateLayout();

    osu->updateConfineCursor();
    osu->updateWindowsKeyDisable();

    anim->moveQuadOut(&m_fDimAnim, (m_bVisible ? 1.0f : 0.0f),
                      cv_pause_anim_duration.getFloat() * (m_bVisible ? 1.0f - m_fDimAnim : m_fDimAnim), true);
    osu->m_chat->updateVisibility();
    return this;
}

void PauseMenu::setContinueEnabled(bool continueEnabled) {
    m_bContinueEnabled = continueEnabled;
    if(m_buttons.size() > 0) m_buttons[0]->setVisible(m_bContinueEnabled);
}

UIPauseMenuButton *PauseMenu::addButton(std::function<Image *()> getImageFunc, UString btn_name) {
    UIPauseMenuButton *button = new UIPauseMenuButton(getImageFunc, 0, 0, 0, 0, btn_name);
    addBaseUIElement(button);
    m_buttons.push_back(button);
    return button;
}
