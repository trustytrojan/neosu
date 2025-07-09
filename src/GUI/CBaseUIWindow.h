#pragma once
#include "CBaseUIElement.h"

class CBaseUIButton;
class CBaseUIContainer;
class CBaseUIBoxShadow;

class RenderTarget;

class CBaseUIWindow : public CBaseUIElement {
   public:
    CBaseUIWindow(float xPos = 0, float yPos = 0, float xSize = 0, float ySize = 0, const UString& name = "");
    ~CBaseUIWindow() override;

    void draw() override;
    virtual void drawCustomContent() { ; }
    void mouse_update(bool *propagate_clicks) override;

    void onKeyDown(KeyboardEvent &e) override;
    void onKeyUp(KeyboardEvent &e) override;
    void onChar(KeyboardEvent &e) override;

    // actions
    void close();
    void open();

    void minimize();

    // BETA: mimic native window
    CBaseUIWindow *enableCoherenceMode();

    // set
    CBaseUIWindow *setSizeToContent(int horizontalBorderSize = 1, int verticalBorderSize = 1);
    CBaseUIWindow *setTitleBarHeight(int height) {
        this->iTitleBarHeight = height;
        this->updateTitleBarMetrics();
        return this;
    }
    CBaseUIWindow *setTitle(UString text);
    CBaseUIWindow *setTitleFont(McFont *titleFont) {
        this->titleFont = titleFont;
        this->updateTitleBarMetrics();
        return this;
    }
    CBaseUIWindow *setResizeLimit(int maxWidth, int maxHeight) {
        this->vResizeLimit = Vector2(maxWidth, maxHeight);
        return this;
    }
    CBaseUIWindow *setResizeable(bool resizeable) {
        this->bResizeable = resizeable;
        return this;
    }
    CBaseUIWindow *setDrawTitleBarLine(bool drawTitleBarLine) {
        this->bDrawTitleBarLine = drawTitleBarLine;
        return this;
    }
    CBaseUIWindow *setDrawFrame(bool drawFrame) {
        this->bDrawFrame = drawFrame;
        return this;
    }
    CBaseUIWindow *setDrawBackground(bool drawBackground) {
        this->bDrawBackground = drawBackground;
        return this;
    }
    CBaseUIWindow *setRoundedRectangle(bool roundedRectangle) {
        this->bRoundedRectangle = roundedRectangle;
        return this;
    }

    CBaseUIWindow *setBackgroundColor(Color backgroundColor) {
        this->backgroundColor = backgroundColor;
        return this;
    }
    CBaseUIWindow *setFrameColor(Color frameColor) {
        this->frameColor = frameColor;
        return this;
    }
    CBaseUIWindow *setFrameBrightColor(Color frameBrightColor) {
        this->frameBrightColor = frameBrightColor;
        return this;
    }
    CBaseUIWindow *setFrameDarkColor(Color frameDarkColor) {
        this->frameDarkColor = frameDarkColor;
        return this;
    }
    CBaseUIWindow *setTitleColor(Color titleColor) {
        this->titleColor = titleColor;
        return this;
    }

    // get
    bool isBusy() override;
    bool isActive() override;
    [[nodiscard]] inline bool isMoving() const { return this->bMoving; }
    [[nodiscard]] inline bool isResizing() const { return this->bResizing; }
    [[nodiscard]] inline CBaseUIContainer *getContainer() const { return this->container; }
    [[nodiscard]] inline CBaseUIContainer *getTitleBarContainer() const { return this->titleBarContainer; }
    inline int getTitleBarHeight() { return this->iTitleBarHeight; }

    // events
    void onMouseDownInside() override;
    void onMouseUpInside() override;
    void onMouseUpOutside() override;

    void onMoved() override;
    void onResized() override;

    virtual void onResolutionChange(Vector2 newResolution);

    void onEnabled() override;
    void onDisabled() override;

   protected:
    void updateTitleBarMetrics();
    void udpateResizeAndMoveLogic(bool captureMouse);
    void updateWindowLogic();

    virtual void onClosed();

    inline CBaseUIButton *getCloseButton() { return this->closeButton; }
    inline CBaseUIButton *getMinimizeButton() { return this->minimizeButton; }

   private:
    // colors
    Color frameColor;
    Color frameBrightColor;
    Color frameDarkColor;
    Color backgroundColor;
    Color titleColor;

    // window properties
    bool bIsOpen;
    bool bAnimIn;
    bool bResizeable;
    bool bCoherenceMode;
    float fAnimation;

    bool bDrawFrame;
    bool bDrawBackground;
    bool bRoundedRectangle;

    // title bar
    bool bDrawTitleBarLine;
    CBaseUIContainer *titleBarContainer;
    McFont *titleFont;
    float fTitleFontWidth;
    float fTitleFontHeight;
    int iTitleBarHeight;
    UString sTitle;

    CBaseUIButton *closeButton;
    CBaseUIButton *minimizeButton;

    // main container
    CBaseUIContainer *container;

    // moving
    bool bMoving;
    Vector2 vMousePosBackup;
    Vector2 vLastPos;

    // resizing
    Vector2 vResizeLimit;
    bool bResizing;
    int iResizeType;
    Vector2 vLastSize;

    // test features
    RenderTarget *rt;
    CBaseUIBoxShadow *shadow;
};
