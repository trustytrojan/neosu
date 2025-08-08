#pragma once
#include "Beatmap.h"

class ConVar;

class ModFPoSu;
class Beatmap;
class SkinImage;
class SliderCurve;
class VertexArrayObject;

enum class HitObjectType : uint8_t {
    CIRCLE,
    SLIDER,
    SPINNER,
};

class HitObject {
   public:
    static void drawHitResult(Beatmap *beatmap, Vector2 rawPos, LiveScore::HIT result, float animPercentInv,
                              float hitDeltaRangePercent);
    static void drawHitResult(Skin *skin, float hitcircleDiameter, float rawHitcircleDiameter, Vector2 rawPos,
                              LiveScore::HIT result, float animPercentInv, float hitDeltaRangePercent);

   public:
    HitObject(long time, int sampleType, int comboNumber, bool isEndOfCombo, int colorCounter, int colorOffset,
              BeatmapInterface *beatmap);
    virtual ~HitObject() { ; }

    virtual void draw() { ; }
    virtual void draw2();
    virtual void update(long curPos, f64 frame_time);

    virtual void updateStackPosition(float stackOffset) = 0;
    virtual void miss(long curPos) = 0;  // only used by notelock

    virtual int getCombo() { return 1; }  // how much combo this hitobject is "worth"

    // Gameplay logic
    HitObjectType type;
    i64 click_time;
    i64 duration;

    // Visual
    i32 combo_number;
    bool is_end_of_combo = false;

    void addHitResult(LiveScore::HIT result, long delta, bool isEndOfCombo, Vector2 posRaw, float targetDelta = 0.0f,
                      float targetAngle = 0.0f, bool ignoreOnHitErrorBar = false, bool ignoreCombo = false,
                      bool ignoreHealth = false, bool addObjectDurationToSkinAnimationTimeStartOffset = true);
    void misAimed() { this->bMisAim = true; }

    void setStack(int stack) { this->iStack = stack; }
    void setForceDrawApproachCircle(bool firstNote) { this->bOverrideHDApproachCircle = firstNote; }
    void setAutopilotDelta(long delta) { this->iAutopilotDelta = delta; }
    void setBlocked(bool blocked) { this->bBlocked = blocked; }

    [[nodiscard]] virtual Vector2 getRawPosAt(long pos) const = 0;          // with stack calculation modifications
    [[nodiscard]] virtual Vector2 getOriginalRawPosAt(long pos) const = 0;  // without stack calculations
    [[nodiscard]] virtual Vector2 getAutoCursorPos(long curPos) const = 0;

    [[nodiscard]] inline int getStack() const { return this->iStack; }
    [[nodiscard]] inline int getColorCounter() const { return this->iColorCounter; }
    [[nodiscard]] inline int getColorOffset() const { return this->iColorOffset; }
    [[nodiscard]] inline float getApproachScale() const { return this->fApproachScale; }
    [[nodiscard]] inline long getDelta() const { return this->iDelta; }
    [[nodiscard]] inline long getApproachTime() const { return this->iApproachTime; }
    [[nodiscard]] inline long getAutopilotDelta() const { return this->iAutopilotDelta; }

    [[nodiscard]] inline bool isVisible() const { return this->bVisible; }
    [[nodiscard]] inline bool isFinished() const { return this->bFinished; }
    [[nodiscard]] inline bool isBlocked() const { return this->bBlocked; }
    [[nodiscard]] inline bool hasMisAimed() const { return this->bMisAim; }

    virtual void onClickEvent(std::vector<Click> & /*clicks*/) { ; }
    virtual void onReset(long curPos);

   private:
   private:
    static float lerp3f(float a, float b, float c, float percent);

    struct HITRESULTANIM {
        Vector2 rawPos;
        long delta;
        float time;
        LiveScore::HIT result;
        bool addObjectDurationToSkinAnimationTimeStartOffset;
    };

    void drawHitResultAnim(const HITRESULTANIM &hitresultanim);

    HITRESULTANIM hitresultanim1;
    HITRESULTANIM hitresultanim2;

   protected:
    BeatmapInterface* bi = nullptr;
    Beatmap* bm = nullptr;  // NULL when simulating

    long iDelta;  // this must be signed
    long iApproachTime;
    long iFadeInTime;  // extra time added before the approachTime to let the object smoothly become visible
    long iAutopilotDelta;

    int iSampleType;
    int iColorCounter;
    int iColorOffset;

    int iStack;

    float fAlpha;
    float fAlphaWithoutHidden;
    float fAlphaForApproachCircle;
    float fApproachScale;
    float fHittableDimRGBColorMultiplierPercent;

    unsigned bBlocked : 1;
    unsigned bOverrideHDApproachCircle : 1;
    unsigned bMisAim : 1;
    unsigned bUseFadeInTimeAsApproachTime : 1;
    unsigned bVisible : 1;
    unsigned bFinished : 1;
};

class Circle final : public HitObject {
   public:
    // main
    static void drawApproachCircle(Beatmap *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset,
                                   float colorRGBMultiplier, float approachScale, float alpha,
                                   bool overrideHDApproachCircle = false);
    static void drawCircle(Beatmap *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset,
                           float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha,
                           bool drawNumber = true, bool overrideHDApproachCircle = false);
    static void drawCircle(Skin *skin, Vector2 pos, float hitcircleDiameter, float numberScale, float overlapScale,
                           int number, int colorCounter, int colorOffset, float colorRGBMultiplier, float approachScale,
                           float alpha, float numberAlpha, bool drawNumber = true,
                           bool overrideHDApproachCircle = false);
    static void drawCircle(Skin *skin, Vector2 pos, float hitcircleDiameter, Color color, float alpha = 1.0f);
    static void drawSliderStartCircle(Beatmap *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset,
                                      float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha,
                                      bool drawNumber = true, bool overrideHDApproachCircle = false);
    static void drawSliderStartCircle(Skin *skin, Vector2 pos, float hitcircleDiameter, float numberScale,
                                      float hitcircleOverlapScale, int number, int colorCounter = 0,
                                      int colorOffset = 0, float colorRGBMultiplier = 1.0f, float approachScale = 1.0f,
                                      float alpha = 1.0f, float numberAlpha = 1.0f, bool drawNumber = true,
                                      bool overrideHDApproachCircle = false);
    static void drawSliderEndCircle(Beatmap *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset,
                                    float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha,
                                    bool drawNumber = true, bool overrideHDApproachCircle = false);
    static void drawSliderEndCircle(Skin *skin, Vector2 pos, float hitcircleDiameter, float numberScale,
                                    float overlapScale, int number = 0, int colorCounter = 0, int colorOffset = 0,
                                    float colorRGBMultiplier = 1.0f, float approachScale = 1.0f, float alpha = 1.0f,
                                    float numberAlpha = 1.0f, bool drawNumber = true,
                                    bool overrideHDApproachCircle = false);

    // split helper functions
    static void drawApproachCircle(Skin *skin, Vector2 pos, Color comboColor, float hitcircleDiameter,
                                   float approachScale, float alpha, bool modHD, bool overrideHDApproachCircle);
    static void drawHitCircleOverlay(SkinImage *hitCircleOverlayImage, Vector2 pos, float circleOverlayImageScale,
                                     float alpha, float colorRGBMultiplier);
    static void drawHitCircle(Image *hitCircleImage, Vector2 pos, Color comboColor, float circleImageScale,
                              float alpha);
    static void drawHitCircleNumber(Skin *skin, float numberScale, float overlapScale, Vector2 pos, int number,
                                    float numberAlpha, float colorRGBMultiplier);

   public:
    Circle(int x, int y, long time, int sampleType, int comboNumber, bool isEndOfCombo, int colorCounter,
           int colorOffset, BeatmapInterface *beatmap);
    ~Circle() override;

    void draw() override;
    void draw2() override;
    void update(long curPos, f64 frame_time) override;

    void updateStackPosition(float stackOffset) override;
    void miss(long curPos) override;

    [[nodiscard]] Vector2 getRawPosAt(long /*pos*/) const override { return this->vRawPos; }
    [[nodiscard]] Vector2 getOriginalRawPosAt(long /*pos*/) const override { return this->vOriginalRawPos; }
    [[nodiscard]] Vector2 getAutoCursorPos(long curPos) const override;

    void onClickEvent(std::vector<Click> &clicks) override;
    void onReset(long curPos) override;

   private:
    // necessary due to the static draw functions
    static int rainbowNumber;
    static int rainbowColorCounter;

    void onHit(LiveScore::HIT result, long delta, float targetDelta = 0.0f, float targetAngle = 0.0f);

    bool bWaiting;

    Vector2 vRawPos;
    Vector2 vOriginalRawPos;  // for live mod changing

    float fHitAnimation;
    float fShakeAnimation;
};

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
    int getCombo() override {
        return 2 + std::max((this->iRepeat - 1), 0) + (std::max((this->iRepeat - 1), 0) + 1) * this->ticks.size();
    }

    [[nodiscard]] Vector2 getRawPosAt(long pos) const override;
    [[nodiscard]] Vector2 getOriginalRawPosAt(long pos) const override;
    [[nodiscard]] inline Vector2 getAutoCursorPos(long /*curPos*/) const override { return this->vCurPoint; }

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

class Spinner final : public HitObject {
   public:
    Spinner(int x, int y, long time, int sampleType, bool isEndOfCombo, long endTime, BeatmapInterface *beatmap);
    ~Spinner() override;

    void draw() override;
    void update(long curPos, f64 frame_time) override;

    void updateStackPosition(float /*stackOffset*/) override { ; }
    void miss(long /*curPos*/) override { ; }

    [[nodiscard]] Vector2 getRawPosAt(long /*pos*/) const override { return this->vRawPos; }
    [[nodiscard]] Vector2 getOriginalRawPosAt(long /*pos*/) const override { return this->vOriginalRawPos; }
    [[nodiscard]] Vector2 getAutoCursorPos(long curPos) const override;

    void onReset(long curPos) override;

   private:
    void onHit();
    void rotate(float rad);

    Vector2 vRawPos;
    Vector2 vOriginalRawPos;

    // bool bClickedOnce;
    float fPercent;

    float fDrawRot;
    float fRotations;
    float fRotationsNeeded;
    float fDeltaOverflow;
    float fSumDeltaAngle;

    int iMaxStoredDeltaAngles;
    float *storedDeltaAngles;
    int iDeltaAngleIndex;
    float fDeltaAngleOverflow;

    float fRPM;

    float fLastMouseAngle;
    float fRatio;
};
