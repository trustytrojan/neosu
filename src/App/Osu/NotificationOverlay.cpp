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
    this->bWaitForKey = false;
    this->bWaitForKeyDisallowsLeftClick = false;
    this->bConsumeNextChar = false;
    this->keyListener = NULL;
}

static const f64 TOAST_WIDTH = 350.0;
static const f64 TOAST_INNER_X_MARGIN = 5.0;
static const f64 TOAST_INNER_Y_MARGIN = 5.0;
static const f64 TOAST_OUTER_Y_MARGIN = 10.0;
static const f64 TOAST_SCREEN_BOTTOM_MARGIN = 20.0;
static const f64 TOAST_SCREEN_RIGHT_MARGIN = 10.0;

ToastElement::ToastElement(UString text, Color borderColor_arg) : CBaseUIButton(0, 0, 0, 0, "", "") {
    this->grabs_clicks = true;

    // TODO: animations
    // TODO: ui scaling

    this->alpha = 0.9;  // TODO: fade in/out
    this->borderColor = borderColor_arg;
    this->creationTime = engine->getTime();

    const auto font = engine->getResourceManager()->getFont("FONT_DEFAULT");
    this->lines = text.wrap(font, TOAST_WIDTH - TOAST_INNER_X_MARGIN * 2.0);
    this->setSize(TOAST_WIDTH, (font->getHeight() * 1.5 * this->lines.size()) + (TOAST_INNER_Y_MARGIN * 2.0));
}

void ToastElement::onClicked() {
    // Set creationTime to -10 so toast is deleted in NotificationOverlay::mouse_update
    this->creationTime = -10.0;

    CBaseUIButton::onClicked();
}

void ToastElement::draw(Graphics *g) {
    const auto font = engine->getResourceManager()->getFont("FONT_DEFAULT");

    // background
    g->setColor(this->isMouseInside() ? 0xff222222 : 0xff111111);
    g->setAlpha(this->alpha);
    g->fillRect(this->vPos.x, this->vPos.y, this->vSize.x, this->vSize.y);

    // border
    g->setColor(this->isMouseInside() ? 0xffffffff : this->borderColor);
    g->setAlpha(this->alpha);
    g->drawRect(this->vPos.x, this->vPos.y, this->vSize.x, this->vSize.y);

    // text
    f64 y = this->vPos.y;
    for(const auto &line : this->lines) {
        y += (font->getHeight() * 1.5);
        g->setColor(0xffffffff);
        g->setAlpha(this->alpha);
        g->pushTransform();
        g->translate(this->vPos.x + TOAST_INNER_X_MARGIN, y);
        g->drawString(font, line);
        g->popTransform();
    }
}

void NotificationOverlay::mouse_update(bool *propagate_clicks) {
    // HACKHACK: should put all toasts in a container instead
    bool a_toast_is_hovered = false;
    Vector2 screen = engine->getScreenSize();
    f64 bottom_y = screen.y - TOAST_SCREEN_BOTTOM_MARGIN;
    for(auto t : this->toasts) {
        bottom_y -= TOAST_OUTER_Y_MARGIN + t->getSize().y;
        t->setPos(screen.x - (TOAST_SCREEN_RIGHT_MARGIN + TOAST_WIDTH), bottom_y);
        t->mouse_update(propagate_clicks);
        a_toast_is_hovered |= t->isMouseInside();
    }

    if(a_toast_is_hovered) {
        for(auto t : this->toasts) {
            // Delay toast disappearance
            t->creationTime += engine->getFrameTime();
        }
    }

    f64 current_time = engine->getTime();
    for(auto it = this->toasts.begin(); it != this->toasts.end(); it++) {
        auto toast = *it;
        if(toast->creationTime + 10.0 < current_time) {
            this->toasts.erase(it);
            delete toast;
            return;
        }
    }
}

void NotificationOverlay::draw(Graphics *g) {
    for(auto t : this->toasts) {
        t->draw(g);
    }

    if(!this->isVisible()) return;

    if(this->bWaitForKey) {
        g->setColor(0x22ffffff);
        g->setAlpha((this->notification1.backgroundAnim / 0.5f) * 0.13f);
        g->fillRect(0, 0, osu->getScreenWidth(), osu->getScreenHeight());
    }

    this->drawNotificationBackground(g, this->notification2);
    this->drawNotificationBackground(g, this->notification1);
    this->drawNotificationText(g, this->notification2);
    this->drawNotificationText(g, this->notification1);
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
    if(!this->isVisible()) return;

    // escape always stops waiting for a key
    if(e.getKeyCode() == KEY_ESCAPE) {
        if(this->bWaitForKey) e.consume();

        this->stopWaitingForKey();
    }

    // key binding logic
    if(this->bWaitForKey) {
        // HACKHACK: prevent left mouse click bindings if relevant
        if(env->getOS() == Environment::OS::WINDOWS && this->bWaitForKeyDisallowsLeftClick &&
           e.getKeyCode() == 0x01)  // 0x01 == VK_LBUTTON
            this->stopWaitingForKey();
        else {
            this->stopWaitingForKey(true);

            debugLog("keyCode = %lu\n", e.getKeyCode());

            if(this->keyListener != NULL) this->keyListener->onKey(e);
        }

        e.consume();
    }

    if(this->bWaitForKey) e.consume();
}

void NotificationOverlay::onKeyUp(KeyboardEvent &e) {
    if(!this->isVisible()) return;

    if(this->bWaitForKey) e.consume();
}

void NotificationOverlay::onChar(KeyboardEvent &e) {
    if(this->bWaitForKey || this->bConsumeNextChar) e.consume();

    this->bConsumeNextChar = false;
}

void NotificationOverlay::addNotification(UString text, Color textColor, bool waitForKey, float duration) {
    const float notificationDuration = (duration < 0.0f ? cv_notification_duration.getFloat() : duration);

    // swap effect
    if(this->isVisible()) {
        this->notification2.text = this->notification1.text;
        this->notification2.textColor = 0xffffffff;

        this->notification2.time = 0.0f;
        this->notification2.alpha = 0.5f;
        this->notification2.backgroundAnim = 1.0f;
        this->notification2.fallAnim = 0.0f;

        anim->deleteExistingAnimation(&this->notification1.alpha);

        anim->moveQuadIn(&this->notification2.fallAnim, 1.0f, 0.2f, 0.0f, true);
        anim->moveQuadIn(&this->notification2.alpha, 0.0f, 0.2f, 0.0f, true);
    }

    // build new notification
    this->bWaitForKey = waitForKey;
    this->bConsumeNextChar = this->bWaitForKey;

    float fadeOutTime = 0.4f;

    this->notification1.text = text;
    this->notification1.textColor = textColor;

    if(!waitForKey)
        this->notification1.time = engine->getTime() + notificationDuration + fadeOutTime;
    else
        this->notification1.time = 0.0f;

    this->notification1.alpha = 0.0f;
    this->notification1.backgroundAnim = 0.5f;
    this->notification1.fallAnim = 0.0f;

    // animations
    if(this->isVisible())
        this->notification1.alpha = 1.0f;
    else
        anim->moveLinear(&this->notification1.alpha, 1.0f, 0.075f, true);

    if(!waitForKey) anim->moveQuadOut(&this->notification1.alpha, 0.0f, fadeOutTime, notificationDuration, false);

    anim->moveQuadOut(&this->notification1.backgroundAnim, 1.0f, 0.15f, 0.0f, true);
}

void NotificationOverlay::addToast(UString text, Color borderColor, ToastClickCallback callback) {
    auto toast = new ToastElement(text, borderColor);
    toast->setClickCallback(callback);
    this->toasts.push_back(toast);
}

void NotificationOverlay::stopWaitingForKey(bool stillConsumeNextChar) {
    this->bWaitForKey = false;
    this->bWaitForKeyDisallowsLeftClick = false;
    this->bConsumeNextChar = stillConsumeNextChar;
}

bool NotificationOverlay::isVisible() {
    return engine->getTime() < this->notification1.time || engine->getTime() < this->notification2.time ||
           this->bWaitForKey;
}
