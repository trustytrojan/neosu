#pragma once
#include "CBaseUIImage.h"
#include "Database.h"

class LiveScore;
class SkinImage;
struct FinishedScore;

class UIRankingScreenRankingPanel : public CBaseUIImage {
   public:
    UIRankingScreenRankingPanel();

    virtual void draw(Graphics *g);

    void setScore(LiveScore *score);
    void setScore(FinishedScore score);

   private:
    void drawHitImage(Graphics *g, SkinImage *img, float scale, Vector2 pos);
    void drawNumHits(Graphics *g, int numHits, float scale, Vector2 pos);

    unsigned long long iScore;
    int iNum300s;
    int iNum300gs;
    int iNum100s;
    int iNum100ks;
    int iNum50s;
    int iNumMisses;
    int iCombo;
    float fAccuracy;
    bool bPerfect;
};
