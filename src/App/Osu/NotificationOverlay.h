#pragma once
#include "CBaseUIButton.h"
#include "KeyboardEvent.h"
#include "OsuScreen.h"

class ToastElement : public CBaseUIButton {
   public:
    ToastElement(UString text, Color borderColor_arg);
    virtual ~ToastElement() { ; }

    virtual void draw(Graphics *g);
    virtual void onClicked() override;

    std::vector<UString> lines;
    Color borderColor;
    f32 alpha = 0.f;
    f32 height = 0.f;
    f64 creationTime;
};

class NotificationOverlayKeyListener {
   public:
    virtual ~NotificationOverlayKeyListener() { ; }
    virtual void onKey(KeyboardEvent &e) = 0;
};

class NotificationOverlay : public OsuScreen {
   public:
    NotificationOverlay();
    virtual ~NotificationOverlay() { ; }

    virtual void mouse_update(bool *propagate_clicks);
    virtual void draw(Graphics *g);

    virtual void onKeyDown(KeyboardEvent &e);
    virtual void onKeyUp(KeyboardEvent &e);
    virtual void onChar(KeyboardEvent &e);

    typedef fastdelegate::FastDelegate0<> ToastClickCallback;
    void addToast(UString text, Color borderColor = 0xffdd0000, ToastClickCallback callback = NULL);

    void addNotification(UString text, Color textColor = 0xffffffff, bool waitForKey = false, float duration = -1.0f);
    void setDisallowWaitForKeyLeftClick(bool disallowWaitForKeyLeftClick) {
        this->bWaitForKeyDisallowsLeftClick = disallowWaitForKeyLeftClick;
    }

    void stopWaitingForKey(bool stillConsumeNextChar = false);

    void addKeyListener(NotificationOverlayKeyListener *keyListener) { this->keyListener = keyListener; }

    virtual bool isVisible();

    inline bool isWaitingForKey() { return this->bWaitForKey || this->bConsumeNextChar; }

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

    std::vector<ToastElement *> toasts;

    NOTIFICATION notification1;
    NOTIFICATION notification2;

    bool bWaitForKey;
    bool bWaitForKeyDisallowsLeftClick;
    bool bConsumeNextChar;
    NotificationOverlayKeyListener *keyListener;
};
