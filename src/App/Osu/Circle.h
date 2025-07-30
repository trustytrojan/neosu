#pragma once
#include "HitObject.h"

class ModFPoSu;
class SkinImage;

class Circle final : public HitObject {
   public:
    // main
    static void drawApproachCircle(Beatmap *beatmap, Vector2 rawPos, int number, int colorCounter,
                                   int colorOffset, float colorRGBMultiplier, float approachScale, float alpha,
                                   bool overrideHDApproachCircle = false);
    static void drawCircle(Beatmap *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset,
                           float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha,
                           bool drawNumber = true, bool overrideHDApproachCircle = false);
    static void drawCircle(Skin *skin, Vector2 pos, float hitcircleDiameter, float numberScale,
                           float overlapScale, int number, int colorCounter, int colorOffset, float colorRGBMultiplier,
                           float approachScale, float alpha, float numberAlpha, bool drawNumber = true,
                           bool overrideHDApproachCircle = false);
    static void drawCircle(Skin *skin, Vector2 pos, float hitcircleDiameter, Color color,
                           float alpha = 1.0f);
    static void drawSliderStartCircle(Beatmap *beatmap, Vector2 rawPos, int number, int colorCounter,
                                      int colorOffset, float colorRGBMultiplier, float approachScale, float alpha,
                                      float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
    static void drawSliderStartCircle(Skin *skin, Vector2 pos, float hitcircleDiameter, float numberScale,
                                      float hitcircleOverlapScale, int number, int colorCounter = 0,
                                      int colorOffset = 0, float colorRGBMultiplier = 1.0f, float approachScale = 1.0f,
                                      float alpha = 1.0f, float numberAlpha = 1.0f, bool drawNumber = true,
                                      bool overrideHDApproachCircle = false);
    static void drawSliderEndCircle(Beatmap *beatmap, Vector2 rawPos, int number, int colorCounter,
                                    int colorOffset, float colorRGBMultiplier, float approachScale, float alpha,
                                    float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
    static void drawSliderEndCircle(Skin *skin, Vector2 pos, float hitcircleDiameter, float numberScale,
                                    float overlapScale, int number = 0, int colorCounter = 0, int colorOffset = 0,
                                    float colorRGBMultiplier = 1.0f, float approachScale = 1.0f, float alpha = 1.0f,
                                    float numberAlpha = 1.0f, bool drawNumber = true,
                                    bool overrideHDApproachCircle = false);

    // split helper functions
    static void drawApproachCircle(Skin *skin, Vector2 pos, Color comboColor, float hitcircleDiameter,
                                   float approachScale, float alpha, bool modHD, bool overrideHDApproachCircle);
    static void drawHitCircleOverlay(SkinImage *hitCircleOverlayImage, Vector2 pos,
                                     float circleOverlayImageScale, float alpha, float colorRGBMultiplier);
    static void drawHitCircle(Image *hitCircleImage, Vector2 pos, Color comboColor, float circleImageScale,
                              float alpha);
    static void drawHitCircleNumber(Skin *skin, float numberScale, float overlapScale, Vector2 pos,
                                    int number, float numberAlpha, float colorRGBMultiplier);

   public:
    Circle(int x, int y, long time, int sampleType, int comboNumber, bool isEndOfCombo, int colorCounter,
           int colorOffset, BeatmapInterface *beatmap);
    ~Circle() override;

    void draw() override;
    void draw2() override;
    void update(long curPos, f64 frame_time) override;

    void updateStackPosition(float stackOffset) override;
    void miss(long curPos) override;

    [[nodiscard]] Vector2 getRawPosAt(long  /*pos*/) const override { return this->vRawPos; }
    [[nodiscard]] Vector2 getOriginalRawPosAt(long  /*pos*/) const override { return this->vOriginalRawPos; }
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
