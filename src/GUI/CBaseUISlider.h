#pragma once

// TODO: fix vertical sliders
// TODO: this entire class is a mess

#include "CBaseUIElement.h"

class CBaseUISlider : public CBaseUIElement {
   public:
    CBaseUISlider(float xPos = 0, float yPos = 0, float xSize = 0, float ySize = 0, UString name = "");
    ~CBaseUISlider() override { ; }

    void draw() override;
    void mouse_update(bool *propagate_clicks) override;

    void onKeyDown(KeyboardEvent &e) override;

    void fireChangeCallback();

    void setOrientation(bool horizontal) {
        this->bHorizontal = horizontal;
        this->onResized();
    }

    void setDrawFrame(bool drawFrame) { this->bDrawFrame = drawFrame; }
    void setDrawBackground(bool drawBackground) { this->bDrawBackground = drawBackground; }

    void setFrameColor(Color frameColor) { this->frameColor = frameColor; }
    void setBackgroundColor(Color backgroundColor) { this->backgroundColor = backgroundColor; }

    void setBlockSize(float xSize, float ySize);

    // callbacks, either void or with ourself as the argument
    using SliderChangeCallback = SA::delegate<void(CBaseUISlider *)>;
    CBaseUISlider *setChangeCallback(const SliderChangeCallback& changeCallback) {
        this->sliderChangeCallback = changeCallback;
        return this;
    }

    CBaseUISlider *setAllowMouseWheel(bool allowMouseWheel) {
        this->bAllowMouseWheel = allowMouseWheel;
        return this;
    }
    CBaseUISlider *setAnimated(bool animated) {
        this->bAnimated = animated;
        return this;
    }
    CBaseUISlider *setLiveUpdate(bool liveUpdate) {
        this->bLiveUpdate = liveUpdate;
        return this;
    }
    CBaseUISlider *setBounds(float minValue, float maxValue);
    CBaseUISlider *setKeyDelta(float keyDelta) {
        this->fKeyDelta = keyDelta;
        return this;
    }
    CBaseUISlider *setValue(float value, bool animate = true, bool call_callback = true);
    CBaseUISlider *setInitialValue(float value);

    inline float getFloat() { return this->fCurValue; }
    inline int getInt() { return (int)this->fCurValue; }
    inline bool getBool() { return (bool)this->fCurValue; }
    inline float getMax() { return this->fMaxValue; }
    inline float getMin() { return this->fMinValue; }
    float getPercent();

    // TODO: DEPRECATED, don't use this function anymore, use setChangeCallback() instead
    bool hasChanged();

    void onFocusStolen() override;
    void onMouseUpInside() override;
    void onMouseUpOutside() override;
    void onMouseDownInside() override;
    void onResized() override;

   protected:
    virtual void drawBlock();

    void updateBlockPos();

    bool bDrawFrame, bDrawBackground;
    bool bHorizontal;
    bool bHasChanged;
    bool bAnimated;
    bool bLiveUpdate;
    bool bAllowMouseWheel;
    Color frameColor, backgroundColor;

    float fMinValue, fMaxValue, fCurValue, fCurPercent;
    Vector2 vBlockSize, vBlockPos;

    Vector2 vGrabBackup;
    float fPrevValue;

    float fKeyDelta;
    float fLastSoundPlayTime = 0.f;

    SliderChangeCallback sliderChangeCallback;
};
