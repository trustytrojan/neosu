#pragma once
#include "CBaseUIElement.h"

class CBaseUIContainer;

class CBaseUIScrollView : public CBaseUIElement {
   public:
    CBaseUIScrollView(float xPos = 0, float yPos = 0, float xSize = 0, float ySize = 0, const UString& name = "");
    ~CBaseUIScrollView() override;

    void freeElements();

    void draw() override;
    void mouse_update(bool *propagate_clicks) override;

    void onKeyUp(KeyboardEvent &e) override;
    void onKeyDown(KeyboardEvent &e) override;
    void onChar(KeyboardEvent &e) override;

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
    [[nodiscard]] inline CBaseUIContainer *getContainer() const { return this->container; }
    [[nodiscard]] inline float getRelPosY() const { return this->vScrollPos.y; }
    [[nodiscard]] inline float getRelPosX() const { return this->vScrollPos.x; }
    [[nodiscard]] inline Vector2 getScrollSize() const { return this->vScrollSize; }
    [[nodiscard]] inline Vector2 getVelocity() const { return (this->vScrollPos - this->vVelocity); }

    [[nodiscard]] inline bool isScrolling() const { return this->bScrolling; }
    bool isBusy() override;

    // events
    void onResized() override;
    void onMouseDownOutside(bool left = true, bool right = false) override;
    void onMouseDownInside(bool left = true, bool right = false) override;
    void onMouseUpInside(bool left = true, bool right = false) override;
    void onMouseUpOutside(bool left = true, bool right = false) override;

    void onFocusStolen() override;
    void onEnabled() override;
    void onDisabled() override;
   protected:
    void onMoved() override;

   private:
    void updateClipping();
    void updateScrollbars();

    void scrollToYInt(int scrollPosY, bool animated = true, bool slow = true);
    void scrollToXInt(int scrollPosX, bool animated = true, bool slow = true);

    // main container
    CBaseUIContainer *container;

    // vars
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
    Vector2 vScrollSize;
    Vector2 vMouseBackup2;
    Vector2 vMouseBackup3;
    Vector2 vVelocity;
    Vector2 vKineticAverage;

    int iPrevScrollDeltaX;
    int iScrollResistance;

    unsigned bAutoScrollingX : 1;
    unsigned bAutoScrollingY : 1;

    unsigned bScrollResistanceCheck : 1;
    unsigned bScrolling : 1;
    unsigned bScrollbarScrolling : 1;
    unsigned bScrollbarIsVerticalScrolling : 1;
    unsigned bBlockScrolling : 1;
    unsigned bHorizontalScrolling : 1;
    unsigned bVerticalScrolling : 1;
    unsigned bFirstScrollSizeToContent : 1 = true;

    // vars
    unsigned bDrawFrame : 1;
    unsigned bDrawBackground : 1;
    unsigned bDrawScrollbars : 1;

public:
    // When you scrolled to the bottom, and new content is added, setting this
    // to true makes it so you'll stay at the bottom.
    // Useful in places where you're waiting on new content, like chat logs.
    unsigned sticky : 1 = false;

    unsigned bHorizontalClipping : 1 = true;
    unsigned bVerticalClipping : 1 = true;
    unsigned bScrollbarOnLeft : 1 = false;
};
