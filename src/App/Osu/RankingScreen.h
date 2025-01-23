#pragma once
#include "Database.h"
#include "DifficultyCalculator.h"
#include "ScreenBackable.h"
#include "score.h"
#include "uwu.h"

class CBaseUIContainer;
class CBaseUIScrollView;
class CBaseUIImage;
class CBaseUILabel;

class Beatmap;
class DatabaseBeatmap;
class SkinImage;
class UIButton;
class UIRankingScreenInfoLabel;
class UIRankingScreenRankingPanel;
class RankingScreenIndexLabel;
class RankingScreenBottomElement;

class ConVar;

class RankingScreen : public ScreenBackable {
   public:
    RankingScreen();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    virtual CBaseUIContainer *setVisible(bool visible);

    void onRetryClicked();
    void onWatchClicked();

    void setScore(FinishedScore score);
    void setBeatmapInfo(Beatmap *beatmap, DatabaseBeatmap *diff2);

   private:
    virtual void updateLayout();
    virtual void onBack();

    void drawModImage(Graphics *g, SkinImage *image, Vector2 &pos, Vector2 &max);

    void setGrade(FinishedScore::Grade grade);
    void setIndex(int index);

    UString getPPString();
    Vector2 getPPPosRaw();

    CBaseUIScrollView *rankings;

    UIRankingScreenInfoLabel *songInfo;
    UIRankingScreenRankingPanel *rankingPanel;
    CBaseUIImage *rankingTitle;
    CBaseUIImage *rankingGrade;
    RankingScreenIndexLabel *rankingIndex;
    RankingScreenBottomElement *rankingBottom;

    UIButton *retry_btn;
    UIButton *watch_btn;

    FinishedScore::Grade grade;
    float fUnstableRate;
    float fHitErrorAvgMin;
    float fHitErrorAvgMax;

    UString sMods;
    bool bModSS;
    bool bModSD;
    bool bModEZ;
    bool bModHD;
    bool bModHR;
    bool bModNightmare;
    bool bModScorev2;
    bool bModTarget;
    bool bModSpunout;
    bool bModRelax;
    bool bModNF;
    bool bModAutopilot;
    bool bModAuto;
    bool bModTD;

    std::vector<ConVar *> enabledExperimentalMods;

    // custom
    uwu::lazy_promise<std::function<pp_info()>, pp_info> ppv2_calc{pp_info{}};
    FinishedScore score;
    bool bIsUnranked;
};
