#pragma once
#include "HitObject.h"

class SliderCurve;

class VertexArrayObject;

class Slider final : public HitObject {
   public:
    struct SLIDERCLICK {
        long time;
        int type;
        int tickIndex;
        bool finished;
        bool successful;
        bool sliderend;
    };

   public:
    Slider(char stype, int repeat, float pixelLength, std::vector<Vector2> points, std::vector<int> hitSounds,
           std::vector<float> ticks, float sliderTime, float sliderTimeWithoutRepeats, long time, int sampleType,
           int comboNumber, bool isEndOfCombo, int colorCounter, int colorOffset, BeatmapInterface *beatmap);
    ~Slider() override;

    void draw() override;
    void draw2() override;
    void draw2(bool drawApproachCircle, bool drawOnlyApproachCircle);
    void update(long curPos, f64 frame_time) override;

    void updateStackPosition(float stackOffset) override;
    void miss(long curPos) override;
    int getCombo() override { return 2 + std::max((this->iRepeat - 1), 0) + (std::max((this->iRepeat - 1), 0) + 1) * this->ticks.size(); }

    [[nodiscard]] Vector2 getRawPosAt(long pos) const override;
    [[nodiscard]] Vector2 getOriginalRawPosAt(long pos) const override;
    [[nodiscard]] inline Vector2 getAutoCursorPos(long  /*curPos*/) const override { return this->vCurPoint; }

    void onClickEvent(std::vector<Click> &clicks) override;
    void onReset(long curPos) override;

    void rebuildVertexBuffer(bool useRawCoords = false);

    [[nodiscard]] inline bool isStartCircleFinished() const { return this->bStartFinished; }
    [[nodiscard]] inline int getRepeat() const { return this->iRepeat; }
    [[nodiscard]] inline std::vector<Vector2> getRawPoints() const { return this->points; }
    [[nodiscard]] inline float getPixelLength() const { return this->fPixelLength; }
    [[nodiscard]] inline const std::vector<SLIDERCLICK> &getClicks() const { return this->clicks; }

    // ILLEGAL:
    [[nodiscard]] inline VertexArrayObject *getVAO() const { return this->vao; }
    [[nodiscard]] inline SliderCurve *getCurve() const { return this->curve; }

   private:
    void drawStartCircle(float alpha);
    void drawEndCircle(float alpha, float sliderSnake);
    void drawBody(float alpha, float from, float to);

    void updateAnimations(long curPos);

    void onHit(LiveScore::HIT result, long delta, bool startOrEnd, float targetDelta = 0.0f, float targetAngle = 0.0f,
               bool isEndResultFromStrictTrackingMod = false);
    void onRepeatHit(bool successful, bool sliderend);
    void onTickHit(bool successful, int tickIndex);
    void onSliderBreak();

    [[nodiscard]] float getT(long pos, bool raw) const;

    bool isClickHeldSlider();  // special logic to disallow hold tapping

    struct SLIDERTICK {
        float percent;
        bool finished;
    };

    std::vector<Vector2> points;
    std::vector<int> hitSounds;

    std::vector<SLIDERTICK> ticks;  // ticks (drawing)

    // TEMP: auto cursordance
    std::vector<SLIDERCLICK> clicks;  // repeats (type 0) + ticks (type 1)

    SliderCurve *curve;
    VertexArrayObject *vao;

    Vector2 vCurPoint;
    Vector2 vCurPointRaw;

    long iStrictTrackingModLastClickHeldTime;

    float fPixelLength;

    float fSliderTime;
    float fSliderTimeWithoutRepeats;

    float fSlidePercent;        // 0.0f - 1.0f - 0.0f - 1.0f - etc.
    float fActualSlidePercent;  // 0.0f - 1.0f
    float fSliderSnakePercent;
    float fReverseArrowAlpha;
    float fBodyAlpha;

    float fStartHitAnimation;
    float fEndHitAnimation;
    float fEndSliderBodyFadeAnimation;

    float fFollowCircleTickAnimationScale;
    float fFollowCircleAnimationScale;
    float fFollowCircleAnimationAlpha;

    int iRepeat;
    int iFatFingerKey;
    int iPrevSliderSlideSoundSampleSet;
    int iReverseArrowPos;
    int iCurRepeat;
    int iCurRepeatCounterForHitSounds;

    char cType;

    LiveScore::HIT startResult : 4;
    LiveScore::HIT endResult : 4;

    unsigned bStartFinished : 1;
    unsigned bEndFinished : 1;
    unsigned bCursorLeft : 1;
    unsigned bCursorInside : 1;
    unsigned bHeldTillEnd : 1;
    unsigned bHeldTillEndForLenienceHack : 1;
    unsigned bHeldTillEndForLenienceHackCheck : 1;
    unsigned bInReverse : 1;
    unsigned bHideNumberAfterFirstRepeatHit : 1;
};
