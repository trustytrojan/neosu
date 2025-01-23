#pragma once
#include "HitObject.h"

class Spinner : public HitObject {
   public:
    Spinner(int x, int y, long time, int sampleType, bool isEndOfCombo, long endTime, BeatmapInterface *beatmap);
    virtual ~Spinner();

    virtual void draw(Graphics *g);
    virtual void update(long curPos, f64 frame_time);

    void updateStackPosition(float stackOffset) { ; }
    void miss(long curPos) { ; }

    Vector2 getRawPosAt(long pos) { return this->vRawPos; }
    Vector2 getOriginalRawPosAt(long pos) { return this->vOriginalRawPos; }
    Vector2 getAutoCursorPos(long curPos);

    virtual void onClickEvent(std::vector<Click> &clicks);
    virtual void onReset(long curPos);

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
