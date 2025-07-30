#pragma once
#include "HitObject.h"

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

    void onClickEvent(std::vector<Click> &clicks) override;
    void onReset(long curPos) override;

   private:
    void onHit();
    void rotate(float rad);

    Vector2 vRawPos;
    Vector2 vOriginalRawPos;

    bool bClickedOnce;
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
