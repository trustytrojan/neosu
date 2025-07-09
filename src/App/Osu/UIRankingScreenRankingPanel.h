#pragma once
#include "CBaseUIImage.h"
#include "Database.h"

class LiveScore;
class SkinImage;
struct FinishedScore;

class UIRankingScreenRankingPanel : public CBaseUIImage {
   public:
    UIRankingScreenRankingPanel();

    void draw() override;

    void setScore(LiveScore *score);
    void setScore(const FinishedScore& score);

   private:
    void drawHitImage(SkinImage *img, float scale, Vector2 pos);
    void drawNumHits(int numHits, float scale, Vector2 pos);

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
