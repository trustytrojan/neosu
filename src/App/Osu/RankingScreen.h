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

    CBaseUIScrollView *m_rankings;

    UIRankingScreenInfoLabel *m_songInfo;
    UIRankingScreenRankingPanel *m_rankingPanel;
    CBaseUIImage *m_rankingTitle;
    CBaseUIImage *m_rankingGrade;
    RankingScreenIndexLabel *m_rankingIndex;
    RankingScreenBottomElement *m_rankingBottom;

    UIButton *m_retry_btn;
    UIButton *m_watch_btn;

    FinishedScore::Grade m_grade;
    float m_fUnstableRate;
    float m_fHitErrorAvgMin;
    float m_fHitErrorAvgMax;

    UString m_sMods;
    bool m_bModSS;
    bool m_bModSD;
    bool m_bModEZ;
    bool m_bModHD;
    bool m_bModHR;
    bool m_bModNC;
    bool m_bModDT;
    bool m_bModNightmare;
    bool m_bModScorev2;
    bool m_bModTarget;
    bool m_bModSpunout;
    bool m_bModRelax;
    bool m_bModNF;
    bool m_bModHT;
    bool m_bModAutopilot;
    bool m_bModAuto;
    bool m_bModTD;

    std::vector<ConVar *> m_enabledExperimentalMods;

    // custom
    uwu::lazy_promise<std::function<pp_info()>, pp_info> m_ppv2_calc{pp_info{}};
    FinishedScore m_score;
    bool m_bIsUnranked;
};
