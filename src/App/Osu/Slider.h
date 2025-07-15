#pragma once
#include "HitObject.h"

class SliderCurve;

class VertexArrayObject;

class Slider : public HitObject {
   public:
    struct SLIDERCLICK {
        long time;
        bool finished;
        bool successful;
        bool sliderend;
        int type;
        int tickIndex;
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

    Vector2 getRawPosAt(long pos) override;
    Vector2 getOriginalRawPosAt(long pos) override;
    inline Vector2 getAutoCursorPos(long  /*curPos*/) override { return this->vCurPoint; }

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

    float getT(long pos, bool raw);

    bool isClickHeldSlider();  // special logic to disallow hold tapping

    SliderCurve *curve;

    char cType;
    int iRepeat;
    float fPixelLength;
    std::vector<Vector2> points;
    std::vector<int> hitSounds;
    float fSliderTime;
    float fSliderTimeWithoutRepeats;

    struct SLIDERTICK {
        float percent;
        bool finished;
    };
    std::vector<SLIDERTICK> ticks;  // ticks (drawing)

    // TEMP: auto cursordance
    std::vector<SLIDERCLICK> clicks;  // repeats (type 0) + ticks (type 1)

    float fSlidePercent;        // 0.0f - 1.0f - 0.0f - 1.0f - etc.
    float fActualSlidePercent;  // 0.0f - 1.0f
    float fSliderSnakePercent;
    float fReverseArrowAlpha;
    float fBodyAlpha;

    Vector2 vCurPoint;
    Vector2 vCurPointRaw;

    LiveScore::HIT startResult;
    LiveScore::HIT endResult;
    bool bStartFinished;
    float fStartHitAnimation;
    bool bEndFinished;
    float fEndHitAnimation;
    float fEndSliderBodyFadeAnimation;
    long iStrictTrackingModLastClickHeldTime;
    int iFatFingerKey;
    int iPrevSliderSlideSoundSampleSet;
    bool bCursorLeft;
    bool bCursorInside;
    bool bHeldTillEnd;
    bool bHeldTillEndForLenienceHack;
    bool bHeldTillEndForLenienceHackCheck;
    float fFollowCircleTickAnimationScale;
    float fFollowCircleAnimationScale;
    float fFollowCircleAnimationAlpha;

    int iReverseArrowPos;
    int iCurRepeat;
    int iCurRepeatCounterForHitSounds;
    bool bInReverse;
    bool bHideNumberAfterFirstRepeatHit;

    VertexArrayObject *vao;
};
