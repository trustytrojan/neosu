#pragma once
#include "Beatmap.h"

class ConVar;

class ModFPoSu;
class Beatmap;

enum class HitObjectType {
    CIRCLE,
    SLIDER,
    SPINNER,
};

class HitObject {
   public:
    static void drawHitResult(Graphics *g, Beatmap *beatmap, Vector2 rawPos, LiveScore::HIT result,
                              float animPercentInv, float hitDeltaRangePercent);
    static void drawHitResult(Graphics *g, Skin *skin, float hitcircleDiameter, float rawHitcircleDiameter,
                              Vector2 rawPos, LiveScore::HIT result, float animPercentInv, float hitDeltaRangePercent);

   public:
    HitObject(long time, int sampleType, int comboNumber, bool isEndOfCombo, int colorCounter, int colorOffset,
              BeatmapInterface *beatmap);
    virtual ~HitObject() { ; }

    virtual void draw(Graphics *g) { ; }
    virtual void draw2(Graphics *g);
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

    virtual Vector2 getRawPosAt(long pos) = 0;          // with stack calculation modifications
    virtual Vector2 getOriginalRawPosAt(long pos) = 0;  // without stack calculations
    virtual Vector2 getAutoCursorPos(long curPos) = 0;

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

    virtual void onClickEvent(std::vector<Click> &clicks) { ; }
    virtual void onReset(long curPos);

   protected:
    BeatmapInterface *bi = NULL;
    Beatmap *bm = NULL;  // NULL when simulating

    bool bVisible;
    bool bFinished;

    int iSampleType;
    int iColorCounter;
    int iColorOffset;

    float fAlpha;
    float fAlphaWithoutHidden;
    float fAlphaForApproachCircle;
    float fApproachScale;
    float fHittableDimRGBColorMultiplierPercent;
    long iDelta;  // this must be signed
    long iApproachTime;
    long iFadeInTime;  // extra time added before the approachTime to let the object smoothly become visible

    int iStack;

    bool bBlocked;
    bool bOverrideHDApproachCircle;
    bool bMisAim;
    long iAutopilotDelta;
    bool bUseFadeInTimeAsApproachTime;

   private:
    static float lerp3f(float a, float b, float c, float percent);

    struct HITRESULTANIM {
        float time;
        Vector2 rawPos;
        LiveScore::HIT result;
        long delta;
        bool addObjectDurationToSkinAnimationTimeStartOffset;
    };

    void drawHitResultAnim(Graphics *g, const HITRESULTANIM &hitresultanim);

    HITRESULTANIM hitresultanim1;
    HITRESULTANIM hitresultanim2;
};
