#pragma once
#include "KeyboardEvent.h"
#include "OsuScreen.h"

class NotificationOverlayKeyListener {
   public:
    virtual ~NotificationOverlayKeyListener() { ; }
    virtual void onKey(KeyboardEvent &e) = 0;
};

class NotificationOverlay : public OsuScreen {
   public:
    NotificationOverlay();
    virtual ~NotificationOverlay() { ; }

    virtual void draw(Graphics *g);

    virtual void onKeyDown(KeyboardEvent &e);
    virtual void onKeyUp(KeyboardEvent &e);
    virtual void onChar(KeyboardEvent &e);

    void addNotification(UString text, Color textColor = 0xffffffff, bool waitForKey = false, float duration = -1.0f);
    void setDisallowWaitForKeyLeftClick(bool disallowWaitForKeyLeftClick) {
        m_bWaitForKeyDisallowsLeftClick = disallowWaitForKeyLeftClick;
    }

    void stopWaitingForKey(bool stillConsumeNextChar = false);

    void addKeyListener(NotificationOverlayKeyListener *keyListener) { m_keyListener = keyListener; }

    virtual bool isVisible();

    inline bool isWaitingForKey() { return m_bWaitForKey || m_bConsumeNextChar; }

   private:
    struct NOTIFICATION {
        UString text = "";
        Color textColor = COLOR(255, 255, 255, 255);

        float time = 0.f;
        float alpha = 0.f;
        float backgroundAnim = 0.f;
        float fallAnim = 0.f;
    };

    void drawNotificationText(Graphics *g, NOTIFICATION &n);
    void drawNotificationBackground(Graphics *g, NOTIFICATION &n);

    NOTIFICATION m_notification1;
    NOTIFICATION m_notification2;

    bool m_bWaitForKey;
    bool m_bWaitForKeyDisallowsLeftClick;
    bool m_bConsumeNextChar;
    NotificationOverlayKeyListener *m_keyListener;
};
