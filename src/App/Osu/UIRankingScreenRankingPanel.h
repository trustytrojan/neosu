#pragma once
#include "CBaseUIImage.h"
#include "Database.h"

class Osu;
class FinishedScore;
class LiveScore;
class SkinImage;

class UIRankingScreenRankingPanel : public CBaseUIImage {
   public:
    UIRankingScreenRankingPanel(Osu *osu);

    virtual void draw(Graphics *g);

    void setScore(LiveScore *score);
    void setScore(FinishedScore score);

   private:
    void drawHitImage(Graphics *g, SkinImage *img, float scale, Vector2 pos);
    void drawNumHits(Graphics *g, int numHits, float scale, Vector2 pos);

    Osu *m_osu;

    unsigned long long m_iScore;
    int m_iNum300s;
    int m_iNum300gs;
    int m_iNum100s;
    int m_iNum100ks;
    int m_iNum50s;
    int m_iNumMisses;
    int m_iCombo;
    float m_fAccuracy;
    bool m_bPerfect;
};
