#pragma once
#include "KeyboardListener.h"
#include "cbase.h"

// Guidelines for avoiding hair pulling:
// - Don't use m_vmSize
// - When an element is standalone, use getPos/setPos
// - In a container or in a scrollview, use getRelPos/setRelPos and call update_pos() on the container

class CBaseUIElement : public KeyboardListener {
   public:
    CBaseUIElement(float xPos = 0, float yPos = 0, float xSize = 0, float ySize = 0, UString name = "") {
        m_vPos.x = xPos;
        m_vPos.y = yPos;
        m_vmPos.x = m_vPos.x;
        m_vmPos.y = m_vPos.y;
        m_vSize.x = xSize;
        m_vSize.y = ySize;
        m_vmSize.x = m_vSize.x;
        m_vmSize.y = m_vSize.y;
        m_sName = name;
    }
    virtual ~CBaseUIElement() { ; }

    // main
    virtual void draw(Graphics *g) = 0;
    virtual void mouse_update(bool *propagate_clicks);
    bool grabs_clicks = false;

    // keyboard input
    virtual void onKeyUp(KeyboardEvent &e) { (void)e; }
    virtual void onKeyDown(KeyboardEvent &e) { (void)e; }
    virtual void onChar(KeyboardEvent &e) { (void)e; }

    // getters
    inline const Vector2 &getPos() const { return m_vPos; }
    inline const Vector2 &getSize() const { return m_vSize; }
    inline UString getName() const { return m_sName; }
    inline const Vector2 &getRelPos() const { return m_vmPos; }
    inline const Vector2 &getRelSize() const { return m_vmSize; }

    virtual bool isActive() { return m_bActive || isBusy(); }
    virtual bool isVisible() { return m_bVisible; }
    virtual bool isEnabled() { return m_bEnabled; }
    virtual bool isBusy() { return m_bBusy && isVisible(); }
    virtual bool isMouseInside() { return m_bMouseInside && isVisible(); }

    virtual CBaseUIElement *setPos(float xPos, float yPos) {
        if(m_vPos.x != xPos || m_vPos.y != yPos) {
            m_vPos.x = xPos;
            m_vPos.y = yPos;
            onMoved();
        }
        return this;
    }
    virtual CBaseUIElement *setPosX(float xPos) {
        if(m_vPos.x != xPos) {
            m_vPos.x = xPos;
            onMoved();
        }
        return this;
    }
    virtual CBaseUIElement *setPosY(float yPos) {
        if(m_vPos.y != yPos) {
            m_vPos.y = yPos;
            onMoved();
        }
        return this;
    }
    virtual CBaseUIElement *setPos(Vector2 position) { return setPos(position.x, position.y); }

    virtual CBaseUIElement *setRelPos(float xPos, float yPos) {
        m_vmPos.x = xPos;
        m_vmPos.y = yPos;
        return this;
    }
    virtual CBaseUIElement *setRelPosX(float xPos) {
        m_vmPos.x = xPos;
        return this;
    }
    virtual CBaseUIElement *setRelPosY(float yPos) {
        m_vmPos.y = yPos;
        return this;
    }
    virtual CBaseUIElement *setRelPos(Vector2 position) { return setRelPos(position.x, position.y); }

    virtual CBaseUIElement *setSize(float xSize, float ySize) {
        if(m_vSize.x != xSize || m_vSize.y != ySize) {
            m_vSize.x = xSize;
            m_vSize.y = ySize;
            onResized();
            onMoved();
        }
        return this;
    }
    virtual CBaseUIElement *setSizeX(float xSize) {
        if(m_vSize.x != xSize) {
            m_vSize.x = xSize;
            onResized();
            onMoved();
        }
        return this;
    }
    virtual CBaseUIElement *setSizeY(float ySize) {
        if(m_vSize.y != ySize) {
            m_vSize.y = ySize;
            onResized();
            onMoved();
        }
        return this;
    }
    virtual CBaseUIElement *setSize(Vector2 size) { return setSize(size.x, size.y); }

    virtual CBaseUIElement *setVisible(bool visible) {
        m_bVisible = visible;
        return this;
    }
    virtual CBaseUIElement *setActive(bool active) {
        m_bActive = active;
        return this;
    }
    virtual CBaseUIElement *setKeepActive(bool keepActive) {
        m_bKeepActive = keepActive;
        return this;
    }
    virtual CBaseUIElement *setEnabled(bool enabled, const char *reason = NULL) {
        if(enabled != m_bEnabled) {
            m_bEnabled = enabled;
            if(m_bEnabled) {
                onEnabled();
            } else {
                disabled_reason = reason;
                onDisabled();
            }
        }
        return this;
    }
    virtual CBaseUIElement *setBusy(bool busy) {
        m_bBusy = busy;
        return this;
    }
    virtual CBaseUIElement *setName(UString name) {
        m_sName = name;
        return this;
    }

    // actions
    void stealFocus() {
        m_bMouseInsideCheck = true;
        m_bActive = false;
        onFocusStolen();
    }

   protected:
    // events
    virtual void onResized() { ; }
    virtual void onMoved() { ; }

    virtual void onFocusStolen() { ; }
    virtual void onEnabled() { ; }
    virtual void onDisabled() { ; }

    virtual void onMouseInside() { ; }
    virtual void onMouseOutside() { ; }
    virtual void onMouseDownInside() { ; }
    virtual void onMouseDownOutside() { ; }
    virtual void onMouseUpInside() { ; }
    virtual void onMouseUpOutside() { ; }

    // vars
    UString m_sName;

    // attributes
    bool m_bVisible = true;
    bool m_bActive = false;  // we are doing something, e.g. textbox is blinking and ready to receive input
    bool m_bBusy = false;    // we demand the focus to be kept on us, e.g. click-drag scrolling in a scrollview
    bool m_bEnabled = true;

    bool m_bKeepActive = false;  // once clicked, don't lose m_bActive, we have to manually release it (e.g. textbox)
    bool m_bMouseInside = false;

    // position and size
    Vector2 m_vPos;
    Vector2 m_vmPos;
    Vector2 m_vSize;
    Vector2 m_vmSize;

    const char *disabled_reason = NULL;

   private:
    bool m_bMouseInsideCheck = false;
    bool m_bMouseUpCheck = false;
};
