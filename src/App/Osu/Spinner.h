#pragma once
#include "HitObject.h"

class Spinner : public HitObject {
   public:
    Spinner(int x, int y, long time, int sampleType, bool isEndOfCombo, long endTime, BeatmapInterface *beatmap);
    virtual ~Spinner();

    virtual void draw(Graphics *g);
    virtual void update(long curPos);

    HitObjectType type = HitObjectType::SPINNER;

    void updateStackPosition(float stackOffset) { ; }
    void miss(long curPos) { ; }

    Vector2 getRawPosAt(long pos) { return m_vRawPos; }
    Vector2 getOriginalRawPosAt(long pos) { return m_vOriginalRawPos; }
    Vector2 getAutoCursorPos(long curPos);

    virtual void onClickEvent(std::vector<Click> &clicks);
    virtual void onReset(long curPos);

   private:
    void onHit();
    void rotate(float rad);

    Vector2 m_vRawPos;
    Vector2 m_vOriginalRawPos;

    bool m_bClickedOnce;
    float m_fPercent;

    float m_fDrawRot;
    float m_fRotations;
    float m_fRotationsNeeded;
    float m_fDeltaOverflow;
    float m_fSumDeltaAngle;

    int m_iMaxStoredDeltaAngles;
    float *m_storedDeltaAngles;
    int m_iDeltaAngleIndex;
    float m_fDeltaAngleOverflow;

    float m_fRPM;

    float m_fLastMouseAngle;
    float m_fRatio;
};
