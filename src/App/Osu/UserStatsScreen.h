//================ Copyright (c) 2019, PG, All rights reserved. =================//
//
// Purpose:		top plays list for weighted pp/acc
//
// $NoKeywords: $
//===============================================================================//

#ifndef OSUUSERSTATSSCREEN_H
#define OSUUSERSTATSSCREEN_H

#include "ScreenBackable.h"

class ConVar;

class CBaseUIContainer;
class CBaseUIScrollView;

class UIContextMenu;
class UserButton;
class ScoreButton;
class UIUserStatsScreenLabel;

class UserStatsScreenBackgroundPPRecalculator;

class UserStatsScreen : public ScreenBackable {
   public:
    UserStatsScreen(Osu *osu);
    virtual ~UserStatsScreen();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    virtual CBaseUIContainer *setVisible(bool visible);

    void onScoreContextMenu(ScoreButton *scoreButton, int id);

   private:
    virtual void updateLayout();

    virtual void onBack();

    void rebuildScoreButtons(UString playerName);

    void onUserClicked(CBaseUIButton *button);
    void onUserButtonChange(UString text, int id);
    void onScoreClicked(CBaseUIButton *button);
    void onMenuClicked(CBaseUIButton *button);
    void onMenuSelected(UString text, int id);
    void onRecalculatePPImportLegacyScoresClicked();
    void onRecalculatePPImportLegacyScoresConfirmed(UString text, int id);
    void onRecalculatePP(bool importLegacyScores);
    void onCopyAllScoresClicked();
    void onCopyAllScoresUserSelected(UString text, int id);
    void onCopyAllScoresConfirmed(UString text, int id);
    void onDeleteAllScoresClicked();
    void onDeleteAllScoresConfirmed(UString text, int id);

    ConVar *m_name_ref;

    UIContextMenu *m_contextMenu;

    UIUserStatsScreenLabel *m_ppVersionInfoLabel;

    UserButton *m_userButton;

    CBaseUIScrollView *m_scores;
    std::vector<ScoreButton *> m_scoreButtons;

    CBaseUIButton *m_menuButton;

    UString m_sCopyAllScoresFromUser;

    bool m_bRecalculatingPP;
    UserStatsScreenBackgroundPPRecalculator *m_backgroundPPRecalculator;
};

#endif
