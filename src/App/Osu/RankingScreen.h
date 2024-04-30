//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		score/results/ranking screen
//
// $NoKeywords: $osuss
//===============================================================================//

#ifndef OSU_OSURANKINGSCREEN_H
#define OSU_OSURANKINGSCREEN_H

#include "Database.h"
#include "ScreenBackable.h"
#include "score.h"

class CBaseUIContainer;
class CBaseUIScrollView;
class CBaseUIImage;
class CBaseUILabel;

class Beatmap;
class DatabaseBeatmap;
class SkinImage;

class UIRankingScreenInfoLabel;
class UIRankingScreenRankingPanel;

class RankingScreenIndexLabel;
class RankingScreenBottomElement;
class RankingScreenScrollDownInfoButton;

class ConVar;

class RankingScreen : public ScreenBackable {
   public:
    RankingScreen(Osu *osu);

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    virtual CBaseUIContainer *setVisible(bool visible);

    void setScore(LiveScore *score);
    void setScore(FinishedScore score, UString dateTime);
    void setBeatmapInfo(Beatmap *beatmap, DatabaseBeatmap *diff2);

   private:
    virtual void updateLayout();
    virtual void onBack();

    void drawModImage(Graphics *g, SkinImage *image, Vector2 &pos, Vector2 &max);

    void onScrollDownClicked();

    void setGrade(FinishedScore::Grade grade);
    void setIndex(int index);

    UString getPPString();
    Vector2 getPPPosRaw();
    Vector2 getPPPosCenterRaw();

    ConVar *m_osu_scores_enabled;

    CBaseUIScrollView *m_rankings;

    UIRankingScreenInfoLabel *m_songInfo;
    UIRankingScreenRankingPanel *m_rankingPanel;
    CBaseUIImage *m_rankingTitle;
    CBaseUIImage *m_rankingGrade;
    RankingScreenIndexLabel *m_rankingIndex;
    RankingScreenBottomElement *m_rankingBottom;

    RankingScreenScrollDownInfoButton *m_rankingScrollDownInfoButton;
    float m_fRankingScrollDownInfoButtonAlphaAnim;

    FinishedScore::Grade m_grade;
    float m_fUnstableRate;
    float m_fHitErrorAvgMin;
    float m_fHitErrorAvgMax;

    float m_fStarsTomTotal;
    float m_fStarsTomAim;
    float m_fStarsTomSpeed;
    float m_fPPv2;

    float m_fSpeedMultiplier;
    float m_fCS;
    float m_fAR;
    float m_fOD;
    float m_fHP;

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
    Vector2 m_vPPCursorMagnetAnimation;
    bool m_bIsLegacyScore;
    bool m_bIsImportedLegacyScore;
    bool m_bIsUnranked;
};

#endif
