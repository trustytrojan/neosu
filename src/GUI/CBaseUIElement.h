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
        this->vPos.x = xPos;
        this->vPos.y = yPos;
        this->vmPos.x = this->vPos.x;
        this->vmPos.y = this->vPos.y;
        this->vSize.x = xSize;
        this->vSize.y = ySize;
        this->vmSize.x = this->vSize.x;
        this->vmSize.y = this->vSize.y;
        this->sName = name;
    }
    ~CBaseUIElement() override { ; }

    // main
    virtual void draw(Graphics *g) = 0;
    virtual void mouse_update(bool *propagate_clicks);
    bool grabs_clicks = false;

    // keyboard input
    void onKeyUp(KeyboardEvent &e) override { (void)e; }
    void onKeyDown(KeyboardEvent &e) override { (void)e; }
    void onChar(KeyboardEvent &e) override { (void)e; }

    // getters
    [[nodiscard]] inline const Vector2 &getPos() const { return this->vPos; }
    [[nodiscard]] inline const Vector2 &getSize() const { return this->vSize; }
    [[nodiscard]] inline UString getName() const { return this->sName; }
    [[nodiscard]] inline const Vector2 &getRelPos() const { return this->vmPos; }
    [[nodiscard]] inline const Vector2 &getRelSize() const { return this->vmSize; }

    virtual bool isActive() { return this->bActive || this->isBusy(); }
    virtual bool isVisible() { return this->bVisible; }
    virtual bool isEnabled() { return this->bEnabled; }
    virtual bool isBusy() { return this->bBusy && this->isVisible(); }
    virtual bool isMouseInside() { return this->bMouseInside && this->isVisible(); }

    virtual CBaseUIElement *setPos(float xPos, float yPos) {
        if(this->vPos.x != xPos || this->vPos.y != yPos) {
            this->vPos.x = xPos;
            this->vPos.y = yPos;
            this->onMoved();
        }
        return this;
    }
    virtual CBaseUIElement *setPosX(float xPos) {
        if(this->vPos.x != xPos) {
            this->vPos.x = xPos;
            this->onMoved();
        }
        return this;
    }
    virtual CBaseUIElement *setPosY(float yPos) {
        if(this->vPos.y != yPos) {
            this->vPos.y = yPos;
            this->onMoved();
        }
        return this;
    }
    virtual CBaseUIElement *setPos(Vector2 position) { return this->setPos(position.x, position.y); }

    virtual CBaseUIElement *setRelPos(float xPos, float yPos) {
        this->vmPos.x = xPos;
        this->vmPos.y = yPos;
        return this;
    }
    virtual CBaseUIElement *setRelPosX(float xPos) {
        this->vmPos.x = xPos;
        return this;
    }
    virtual CBaseUIElement *setRelPosY(float yPos) {
        this->vmPos.y = yPos;
        return this;
    }
    virtual CBaseUIElement *setRelPos(Vector2 position) { return this->setRelPos(position.x, position.y); }

    virtual CBaseUIElement *setSize(float xSize, float ySize) {
        if(this->vSize.x != xSize || this->vSize.y != ySize) {
            this->vSize.x = xSize;
            this->vSize.y = ySize;
            this->onResized();
            this->onMoved();
        }
        return this;
    }
    virtual CBaseUIElement *setSizeX(float xSize) {
        if(this->vSize.x != xSize) {
            this->vSize.x = xSize;
            this->onResized();
            this->onMoved();
        }
        return this;
    }
    virtual CBaseUIElement *setSizeY(float ySize) {
        if(this->vSize.y != ySize) {
            this->vSize.y = ySize;
            this->onResized();
            this->onMoved();
        }
        return this;
    }
    virtual CBaseUIElement *setSize(Vector2 size) { return this->setSize(size.x, size.y); }

    virtual CBaseUIElement *setVisible(bool visible) {
        this->bVisible = visible;
        return this;
    }
    virtual CBaseUIElement *setActive(bool active) {
        this->bActive = active;
        return this;
    }
    virtual CBaseUIElement *setKeepActive(bool keepActive) {
        this->bKeepActive = keepActive;
        return this;
    }
    virtual CBaseUIElement *setEnabled(bool enabled, const char *reason = NULL) {
        if(enabled != this->bEnabled) {
            this->bEnabled = enabled;
            if(this->bEnabled) {
                this->onEnabled();
            } else {
                this->disabled_reason = reason;
                this->onDisabled();
            }
        }
        return this;
    }
    virtual CBaseUIElement *setBusy(bool busy) {
        this->bBusy = busy;
        return this;
    }
    virtual CBaseUIElement *setName(UString name) {
        this->sName = name;
        return this;
    }

    // actions
    void stealFocus() {
        this->bMouseInsideCheck = true;
        this->bActive = false;
        this->onFocusStolen();
    }

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
    UString sName;

    // attributes
    bool bVisible = true;
    bool bActive = false;  // we are doing something, e.g. textbox is blinking and ready to receive input
    bool bBusy = false;    // we demand the focus to be kept on us, e.g. click-drag scrolling in a scrollview
    bool bEnabled = true;

    bool bKeepActive = false;  // once clicked, don't lose m_bActive, we have to manually release it (e.g. textbox)
    bool bMouseInside = false;

    // position and size
    Vector2 vPos;
    Vector2 vmPos;
    Vector2 vSize;
    Vector2 vmSize;

    const char *disabled_reason = NULL;

   private:
    bool bMouseInsideCheck = false;
    bool bMouseUpCheck = false;
};
