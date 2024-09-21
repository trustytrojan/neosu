#pragma once
#include "HitObject.h"

class ModFPoSu;
class SkinImage;

class Circle : public HitObject {
   public:
    // main
    static void drawApproachCircle(Graphics *g, Beatmap *beatmap, Vector2 rawPos, int number, int colorCounter,
                                   int colorOffset, float colorRGBMultiplier, float approachScale, float alpha,
                                   bool overrideHDApproachCircle = false);
    static void drawCircle(Graphics *g, Beatmap *beatmap, Vector2 rawPos, int number, int colorCounter, int colorOffset,
                           float colorRGBMultiplier, float approachScale, float alpha, float numberAlpha,
                           bool drawNumber = true, bool overrideHDApproachCircle = false);
    static void drawCircle(Graphics *g, Skin *skin, Vector2 pos, float hitcircleDiameter, float numberScale,
                           float overlapScale, int number, int colorCounter, int colorOffset, float colorRGBMultiplier,
                           float approachScale, float alpha, float numberAlpha, bool drawNumber = true,
                           bool overrideHDApproachCircle = false);
    static void drawCircle(Graphics *g, Skin *skin, Vector2 pos, float hitcircleDiameter, Color color,
                           float alpha = 1.0f);
    static void drawSliderStartCircle(Graphics *g, Beatmap *beatmap, Vector2 rawPos, int number, int colorCounter,
                                      int colorOffset, float colorRGBMultiplier, float approachScale, float alpha,
                                      float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
    static void drawSliderStartCircle(Graphics *g, Skin *skin, Vector2 pos, float hitcircleDiameter, float numberScale,
                                      float hitcircleOverlapScale, int number, int colorCounter = 0,
                                      int colorOffset = 0, float colorRGBMultiplier = 1.0f, float approachScale = 1.0f,
                                      float alpha = 1.0f, float numberAlpha = 1.0f, bool drawNumber = true,
                                      bool overrideHDApproachCircle = false);
    static void drawSliderEndCircle(Graphics *g, Beatmap *beatmap, Vector2 rawPos, int number, int colorCounter,
                                    int colorOffset, float colorRGBMultiplier, float approachScale, float alpha,
                                    float numberAlpha, bool drawNumber = true, bool overrideHDApproachCircle = false);
    static void drawSliderEndCircle(Graphics *g, Skin *skin, Vector2 pos, float hitcircleDiameter, float numberScale,
                                    float overlapScale, int number = 0, int colorCounter = 0, int colorOffset = 0,
                                    float colorRGBMultiplier = 1.0f, float approachScale = 1.0f, float alpha = 1.0f,
                                    float numberAlpha = 1.0f, bool drawNumber = true,
                                    bool overrideHDApproachCircle = false);

    // split helper functions
    static void drawApproachCircle(Graphics *g, Skin *skin, Vector2 pos, Color comboColor, float hitcircleDiameter,
                                   float approachScale, float alpha, bool modHD, bool overrideHDApproachCircle);
    static void drawHitCircleOverlay(Graphics *g, SkinImage *hitCircleOverlayImage, Vector2 pos,
                                     float circleOverlayImageScale, float alpha, float colorRGBMultiplier);
    static void drawHitCircle(Graphics *g, Image *hitCircleImage, Vector2 pos, Color comboColor, float circleImageScale,
                              float alpha);
    static void drawHitCircleNumber(Graphics *g, Skin *skin, float numberScale, float overlapScale, Vector2 pos,
                                    int number, float numberAlpha, float colorRGBMultiplier);

   public:
    Circle(int x, int y, long time, int sampleType, int comboNumber, bool isEndOfCombo, int colorCounter,
           int colorOffset, BeatmapInterface *beatmap);
    virtual ~Circle();

    virtual void draw(Graphics *g);
    virtual void draw2(Graphics *g);
    virtual void update(long curPos, f64 frame_time);

    void updateStackPosition(float stackOffset);
    void miss(long curPos);

    Vector2 getRawPosAt(long pos) { return m_vRawPos; }
    Vector2 getOriginalRawPosAt(long pos) { return m_vOriginalRawPos; }
    Vector2 getAutoCursorPos(long curPos);

    virtual void onClickEvent(std::vector<Click> &clicks);
    virtual void onReset(long curPos);

   private:
    // necessary due to the static draw functions
    static int rainbowNumber;
    static int rainbowColorCounter;

    void onHit(LiveScore::HIT result, long delta, float targetDelta = 0.0f, float targetAngle = 0.0f);

    Vector2 m_vRawPos;
    Vector2 m_vOriginalRawPos;  // for live mod changing

    bool m_bWaiting;
    float m_fHitAnimation;
    float m_fShakeAnimation;
};
