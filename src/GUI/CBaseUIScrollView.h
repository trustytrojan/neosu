#pragma once
#include "CBaseUIElement.h"

class CBaseUIContainer;

class CBaseUIScrollView : public CBaseUIElement {
   public:
    CBaseUIScrollView(float xPos = 0, float yPos = 0, float xSize = 0, float ySize = 0, UString name = "");
    virtual ~CBaseUIScrollView();

    void clear();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    virtual void onKeyUp(KeyboardEvent &e);
    virtual void onKeyDown(KeyboardEvent &e);
    virtual void onChar(KeyboardEvent &e);

    // scrolling
    void scrollY(int delta, bool animated = true);
    void scrollX(int delta, bool animated = true);
    void scrollToY(int scrollPosY, bool animated = true);
    void scrollToX(int scrollPosX, bool animated = true);
    void scrollToElement(CBaseUIElement *element, int xOffset = 0, int yOffset = 0, bool animated = true);

    void scrollToLeft();
    void scrollToRight();
    void scrollToBottom();
    void scrollToTop();

    // set
    CBaseUIScrollView *setDrawBackground(bool drawBackground) {
        this->bDrawBackground = drawBackground;
        return this;
    }
    CBaseUIScrollView *setDrawFrame(bool drawFrame) {
        this->bDrawFrame = drawFrame;
        return this;
    }
    CBaseUIScrollView *setDrawScrollbars(bool drawScrollbars) {
        this->bDrawScrollbars = drawScrollbars;
        return this;
    }

    CBaseUIScrollView *setBackgroundColor(Color backgroundColor) {
        this->backgroundColor = backgroundColor;
        return this;
    }
    CBaseUIScrollView *setFrameColor(Color frameColor) {
        this->frameColor = frameColor;
        return this;
    }
    CBaseUIScrollView *setFrameBrightColor(Color frameBrightColor) {
        this->frameBrightColor = frameBrightColor;
        return this;
    }
    CBaseUIScrollView *setFrameDarkColor(Color frameDarkColor) {
        this->frameDarkColor = frameDarkColor;
        return this;
    }
    CBaseUIScrollView *setScrollbarColor(Color scrollbarColor) {
        this->scrollbarColor = scrollbarColor;
        return this;
    }

    CBaseUIScrollView *setHorizontalScrolling(bool horizontalScrolling) {
        this->bHorizontalScrolling = horizontalScrolling;
        return this;
    }
    CBaseUIScrollView *setVerticalScrolling(bool verticalScrolling) {
        this->bVerticalScrolling = verticalScrolling;
        return this;
    }
    CBaseUIScrollView *setScrollSizeToContent(int border = 5);
    CBaseUIScrollView *setScrollResistance(int scrollResistanceInPixels) {
        this->iScrollResistance = scrollResistanceInPixels;
        return this;
    }

    CBaseUIScrollView *setBlockScrolling(bool block) {
        this->bBlockScrolling = block;
        return this;
    }  // means: disable scrolling, not scrolling in 'blocks'

    void setScrollMouseWheelMultiplier(float scrollMouseWheelMultiplier) {
        this->fScrollMouseWheelMultiplier = scrollMouseWheelMultiplier;
    }
    void setScrollbarSizeMultiplier(float scrollbarSizeMultiplier) {
        this->fScrollbarSizeMultiplier = scrollbarSizeMultiplier;
    }

    // get
    inline CBaseUIContainer *getContainer() const { return this->container; }
    inline float getRelPosY() const { return this->vScrollPos.y; }
    inline float getRelPosX() const { return this->vScrollPos.x; }
    inline Vector2 getScrollSize() const { return this->vScrollSize; }
    inline Vector2 getVelocity() const { return (this->vScrollPos - this->vVelocity); }

    inline bool isScrolling() const { return this->bScrolling; }
    bool isBusy();

    // events
    void onResized();
    void onMouseDownOutside();
    void onMouseDownInside();
    void onMouseUpInside();
    void onMouseUpOutside();

    void onFocusStolen();
    void onEnabled();
    void onDisabled();

    // When you scrolled to the bottom, and new content is added, setting this
    // to true makes it so you'll stay at the bottom.
    // Useful in places where you're waiting on new content, like chat logs.
    bool sticky = false;

    bool bHorizontalClipping = true;
    bool bVerticalClipping = true;
    bool bScrollbarOnLeft = false;

   protected:
    virtual void onMoved();

   private:
    void updateClipping();
    void updateScrollbars();

    void scrollToYInt(int scrollPosY, bool animated = true, bool slow = true);
    void scrollToXInt(int scrollPosX, bool animated = true, bool slow = true);

    // main container
    CBaseUIContainer *container;

    // vars
    bool bDrawFrame;
    bool bDrawBackground;
    bool bDrawScrollbars;

    Color backgroundColor;
    Color frameColor;
    Color frameBrightColor;
    Color frameDarkColor;
    Color scrollbarColor;

    Vector2 vScrollPos;
    Vector2 vScrollPosBackup;
    Vector2 vMouseBackup;

    float fScrollMouseWheelMultiplier;
    float fScrollbarSizeMultiplier;
    McRect verticalScrollbar;
    McRect horizontalScrollbar;

    // scroll logic
    bool bScrolling;
    bool bScrollbarScrolling;
    bool bScrollbarIsVerticalScrolling;
    bool bBlockScrolling;
    bool bHorizontalScrolling;
    bool bVerticalScrolling;
    bool bFirstScrollSizeToContent = true;
    Vector2 vScrollSize;
    Vector2 vMouseBackup2;
    Vector2 vMouseBackup3;
    Vector2 vVelocity;
    Vector2 vKineticAverage;

    bool bAutoScrollingX;
    bool bAutoScrollingY;
    int iPrevScrollDeltaX;

    bool bScrollResistanceCheck;
    int iScrollResistance;
};
