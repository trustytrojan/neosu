#include "NotificationOverlay.h"

#include "AnimationHandler.h"
#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Osu.h"
#include "ResourceManager.h"

NotificationOverlay::NotificationOverlay() : OsuScreen() {
    m_bWaitForKey = false;
    m_bWaitForKeyDisallowsLeftClick = false;
    m_bConsumeNextChar = false;
    m_keyListener = NULL;
}

static const f64 TOAST_WIDTH = 350.0;
static const f64 TOAST_INNER_X_MARGIN = 5.0;
static const f64 TOAST_INNER_Y_MARGIN = 5.0;
static const f64 TOAST_OUTER_Y_MARGIN = 10.0;
static const f64 TOAST_SCREEN_BOTTOM_MARGIN = 20.0;
static const f64 TOAST_SCREEN_RIGHT_MARGIN = 10.0;

ToastElement::ToastElement(UString text, Color borderColor_arg) : CBaseUIButton(0, 0, 0, 0, "", "") {
    grabs_clicks = true;

    // TODO: animations
    // TODO: ui scaling

    alpha = 0.9;  // TODO: fade in/out
    borderColor = borderColor_arg;
    creationTime = engine->getTime();

    const auto font = engine->getResourceManager()->getFont("FONT_DEFAULT");
    lines = text.wrap(font, TOAST_WIDTH - TOAST_INNER_X_MARGIN * 2.0);
    setSize(TOAST_WIDTH, (font->getHeight() * 1.5 * lines.size()) + (TOAST_INNER_Y_MARGIN * 2.0));
}

void ToastElement::onClicked() {
    // Set creationTime to -10 so toast is deleted in NotificationOverlay::mouse_update
    creationTime = -10.0;

    CBaseUIButton::onClicked();
}

void ToastElement::draw(Graphics *g) {
    const auto font = engine->getResourceManager()->getFont("FONT_DEFAULT");

    // background
    g->setColor(isMouseInside() ? 0xff222222 : 0xff111111);
    g->setAlpha(alpha);
    g->fillRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);

    // border
    g->setColor(isMouseInside() ? 0xffffffff : borderColor);
    g->setAlpha(alpha);
    g->drawRect(m_vPos.x, m_vPos.y, m_vSize.x, m_vSize.y);

    // text
    f64 y = m_vPos.y;
    for(const auto &line : lines) {
        y += (font->getHeight() * 1.5);
        g->setColor(0xffffffff);
        g->setAlpha(alpha);
        g->pushTransform();
        g->translate(m_vPos.x + TOAST_INNER_X_MARGIN, y);
        g->drawString(font, line);
        g->popTransform();
    }
}

void NotificationOverlay::mouse_update(bool *propagate_clicks) {
    // HACKHACK: should put all toasts in a container instead
    bool a_toast_is_hovered = false;
    Vector2 screen = engine->getScreenSize();
    f64 bottom_y = screen.y - TOAST_SCREEN_BOTTOM_MARGIN;
    for(auto t : toasts) {
        bottom_y -= TOAST_OUTER_Y_MARGIN + t->getSize().y;
        t->setPos(screen.x - (TOAST_SCREEN_RIGHT_MARGIN + TOAST_WIDTH), bottom_y);
        t->mouse_update(propagate_clicks);
        a_toast_is_hovered |= t->isMouseInside();
    }

    if(a_toast_is_hovered) {
        for(auto t : toasts) {
            // Delay toast disappearance
            t->creationTime += engine->getFrameTime();
        }
    }

    f64 current_time = engine->getTime();
    for(auto it = toasts.begin(); it != toasts.end(); it++) {
        auto toast = *it;
        if(toast->creationTime + 10.0 < current_time) {
            toasts.erase(it);
            delete toast;
            return;
        }
    }
}

void NotificationOverlay::draw(Graphics *g) {
    for(auto t : toasts) {
        t->draw(g);
    }

    if(!isVisible()) return;

    if(m_bWaitForKey) {
        g->setColor(0x22ffffff);
        g->setAlpha((m_notification1.backgroundAnim / 0.5f) * 0.13f);
        g->fillRect(0, 0, osu->getScreenWidth(), osu->getScreenHeight());
    }

    drawNotificationBackground(g, m_notification2);
    drawNotificationBackground(g, m_notification1);
    drawNotificationText(g, m_notification2);
    drawNotificationText(g, m_notification1);
}

void NotificationOverlay::drawNotificationText(Graphics *g, NotificationOverlay::NOTIFICATION &n) {
    McFont *font = osu->getSubTitleFont();
    int height = font->getHeight() * 2;
    int stringWidth = font->getStringWidth(n.text);

    g->pushTransform();
    {
        g->setColor(0xff000000);
        g->setAlpha(n.alpha);
        g->translate((int)(osu->getScreenWidth() / 2 - stringWidth / 2 + 1),
                     (int)(osu->getScreenHeight() / 2 + font->getHeight() / 2 + n.fallAnim * height * 0.15f + 1));
        g->drawString(font, n.text);

        g->setColor(n.textColor);
        g->setAlpha(n.alpha);
        g->translate(-1, -1);
        g->drawString(font, n.text);
    }
    g->popTransform();
}

void NotificationOverlay::drawNotificationBackground(Graphics *g, NotificationOverlay::NOTIFICATION &n) {
    McFont *font = osu->getSubTitleFont();
    int height = font->getHeight() * 2 * n.backgroundAnim;

    g->setColor(0xff000000);
    g->setAlpha(n.alpha * 0.75f);
    g->fillRect(0, osu->getScreenHeight() / 2 - height / 2, osu->getScreenWidth(), height);
}

void NotificationOverlay::onKeyDown(KeyboardEvent &e) {
    if(!isVisible()) return;

    // escape always stops waiting for a key
    if(e.getKeyCode() == KEY_ESCAPE) {
        if(m_bWaitForKey) e.consume();

        stopWaitingForKey();
    }

    // key binding logic
    if(m_bWaitForKey) {
        // HACKHACK: prevent left mouse click bindings if relevant
        if(env->getOS() == Environment::OS::WINDOWS && m_bWaitForKeyDisallowsLeftClick &&
           e.getKeyCode() == 0x01)  // 0x01 == VK_LBUTTON
            stopWaitingForKey();
        else {
            stopWaitingForKey(true);

            debugLog("keyCode = %lu\n", e.getKeyCode());

            if(m_keyListener != NULL) m_keyListener->onKey(e);
        }

        e.consume();
    }

    if(m_bWaitForKey) e.consume();
}

void NotificationOverlay::onKeyUp(KeyboardEvent &e) {
    if(!isVisible()) return;

    if(m_bWaitForKey) e.consume();
}

void NotificationOverlay::onChar(KeyboardEvent &e) {
    if(m_bWaitForKey || m_bConsumeNextChar) e.consume();

    m_bConsumeNextChar = false;
}

void NotificationOverlay::addNotification(UString text, Color textColor, bool waitForKey, float duration) {
    const float notificationDuration = (duration < 0.0f ? cv_notification_duration.getFloat() : duration);

    // swap effect
    if(isVisible()) {
        m_notification2.text = m_notification1.text;
        m_notification2.textColor = 0xffffffff;

        m_notification2.time = 0.0f;
        m_notification2.alpha = 0.5f;
        m_notification2.backgroundAnim = 1.0f;
        m_notification2.fallAnim = 0.0f;

        anim->deleteExistingAnimation(&m_notification1.alpha);

        anim->moveQuadIn(&m_notification2.fallAnim, 1.0f, 0.2f, 0.0f, true);
        anim->moveQuadIn(&m_notification2.alpha, 0.0f, 0.2f, 0.0f, true);
    }

    // build new notification
    m_bWaitForKey = waitForKey;
    m_bConsumeNextChar = m_bWaitForKey;

    float fadeOutTime = 0.4f;

    m_notification1.text = text;
    m_notification1.textColor = textColor;

    if(!waitForKey)
        m_notification1.time = engine->getTime() + notificationDuration + fadeOutTime;
    else
        m_notification1.time = 0.0f;

    m_notification1.alpha = 0.0f;
    m_notification1.backgroundAnim = 0.5f;
    m_notification1.fallAnim = 0.0f;

    // animations
    if(isVisible())
        m_notification1.alpha = 1.0f;
    else
        anim->moveLinear(&m_notification1.alpha, 1.0f, 0.075f, true);

    if(!waitForKey) anim->moveQuadOut(&m_notification1.alpha, 0.0f, fadeOutTime, notificationDuration, false);

    anim->moveQuadOut(&m_notification1.backgroundAnim, 1.0f, 0.15f, 0.0f, true);
}

void NotificationOverlay::addToast(UString text, Color borderColor, ToastClickCallback callback) {
    auto toast = new ToastElement(text, borderColor);
    toast->setClickCallback(callback);
    toasts.push_back(toast);
}

void NotificationOverlay::stopWaitingForKey(bool stillConsumeNextChar) {
    m_bWaitForKey = false;
    m_bWaitForKeyDisallowsLeftClick = false;
    m_bConsumeNextChar = stillConsumeNextChar;
}

bool NotificationOverlay::isVisible() {
    return engine->getTime() < m_notification1.time || engine->getTime() < m_notification2.time || m_bWaitForKey;
}
