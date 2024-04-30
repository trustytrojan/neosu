//================ Copyright (c) 2019, PG, All rights reserved. =================//
//
// Purpose:		top plays list for weighted pp/acc
//
// $NoKeywords: $
//===============================================================================//

#include "UserStatsScreen.h"

#include "CBaseUIContainer.h"
#include "CBaseUILabel.h"
#include "CBaseUIScrollView.h"
#include "ConVar.h"
#include "Database.h"
#include "DatabaseBeatmap.h"
#include "Engine.h"
#include "HUD.h"
#include "Icons.h"
#include "Keyboard.h"
#include "ModSelector.h"
#include "Mouse.h"
#include "OptionsMenu.h"
#include "Osu.h"
#include "Replay.h"
#include "ResourceManager.h"
#include "Skin.h"
#include "SongBrowser.h"
#include "SoundEngine.h"
#include "TooltipOverlay.h"
#include "UIContextMenu.h"
#include "UISongBrowserScoreButton.h"
#include "UISongBrowserUserButton.h"
#include "UIUserStatsScreenLabel.h"

ConVar osu_ui_top_ranks_max("osu_ui_top_ranks_max", 200, FCVAR_NONE,
                            "maximum number of displayed scores, to keep the ui/scrollbar manageable");

class UserStatsScreenMenuButton : public CBaseUIButton {
   public:
    UserStatsScreenMenuButton(float xPos = 0, float yPos = 0, float xSize = 0, float ySize = 0, UString name = "",
                              UString text = "")
        : CBaseUIButton(xPos, yPos, xSize, ySize, name, text) {}

    virtual void drawText(Graphics *g) {
        // HACKHACK: force update string height to non-average line height for icon
        m_fStringHeight = m_font->getStringHeight(m_sText);

        if(m_font != NULL && m_sText.length() > 0) {
            g->pushClipRect(McRect(m_vPos.x + 1, m_vPos.y + 1, m_vSize.x - 1, m_vSize.y - 1));
            {
                g->setColor(m_textColor);
                g->pushTransform();
                {
                    g->translate((int)(m_vPos.x + m_vSize.x / 2.0f - (m_fStringWidth / 2.0f)),
                                 (int)(m_vPos.y + m_vSize.y / 2.0f + (m_fStringHeight / 2.0f)));
                    g->drawString(m_font, m_sText);
                }
                g->popTransform();
            }
            g->popClipRect();
        }
    }
};

class UserStatsScreenBackgroundPPRecalculator : public Resource {
   public:
    UserStatsScreenBackgroundPPRecalculator(Osu *osu, UString userName, bool importLegacyScores) : Resource() {
        m_osu = osu;
        m_sUserName = userName;
        m_bImportLegacyScores = importLegacyScores;

        m_iNumScoresToRecalculate = 0;
        m_iNumScoresRecalculated = 0;
    }

    inline int getNumScoresToRecalculate() const { return m_iNumScoresToRecalculate.load(); }
    inline int getNumScoresRecalculated() const { return m_iNumScoresRecalculated.load(); }

   private:
    virtual void init() { m_bReady = true; }

    virtual void initAsync() {
        std::unordered_map<MD5Hash, std::vector<FinishedScore>> *scores =
            m_osu->getSongBrowser()->getDatabase()->getScores();

        // count number of scores to recalculate for UI
        std::string sUsername(m_sUserName.toUtf8());
        size_t numScoresToRecalculate = 0;
        for(const auto &kv : *scores) {
            for(size_t i = 0; i < kv.second.size(); i++) {
                const FinishedScore &score = kv.second[i];

                if((!score.isLegacyScore || m_bImportLegacyScores) && score.playerName == sUsername)
                    numScoresToRecalculate++;
            }
        }
        m_iNumScoresToRecalculate = numScoresToRecalculate;

        debugLog("PPRecalc will recalculate %i scores ...\n", (int)numScoresToRecalculate);

        // actually recalculate them
        for(auto &kv : *scores) {
            for(size_t i = 0; i < kv.second.size(); i++) {
                FinishedScore &score = kv.second[i];

                if((!score.isLegacyScore || m_bImportLegacyScores) && score.playerName == sUsername) {
                    if(m_bInterrupted.load()) break;
                    if(score.md5hash.hash[0] == 0) continue;

                    // NOTE: avoid importing the same score twice
                    if(m_bImportLegacyScores && score.isLegacyScore) {
                        const std::vector<FinishedScore> &otherScores = (*scores)[score.md5hash];

                        bool isScoreAlreadyImported = false;
                        for(size_t s = 0; s < otherScores.size(); s++) {
                            if(score.isLegacyScoreEqualToImportedLegacyScore(otherScores[s])) {
                                isScoreAlreadyImported = true;
                                break;
                            }
                        }

                        if(isScoreAlreadyImported) continue;
                    }

                    // 1) get matching beatmap from db
                    DatabaseBeatmap *diff2 =
                        m_osu->getSongBrowser()->getDatabase()->getBeatmapDifficulty(score.md5hash);
                    if(diff2 == NULL) {
                        if(Osu::debug->getBool()) debugLog("PPRecalc couldn't find %s\n", score.md5hash.toUtf8());

                        continue;
                    }

                    // 1.5) reload metadata for sanity (maybe osu!.db has outdated AR/CS/OD/HP or some other shit)
                    if(!DatabaseBeatmap::loadMetadata(diff2)) continue;

                    const Replay::BEATMAP_VALUES legacyValues = Replay::getBeatmapValuesForModsLegacy(
                        score.modsLegacy, diff2->getAR(), diff2->getCS(), diff2->getOD(), diff2->getHP());
                    const std::string &osuFilePath = diff2->getFilePath();
                    const float AR = (score.isLegacyScore ? legacyValues.AR : score.AR);
                    const float CS = (score.isLegacyScore ? legacyValues.CS : score.CS);
                    const float OD = (score.isLegacyScore ? legacyValues.OD : score.OD);
                    const float HP = (score.isLegacyScore ? legacyValues.HP : score.HP);
                    const float speedMultiplier =
                        (score.isLegacyScore ? legacyValues.speedMultiplier : score.speedMultiplier);
                    const float relax = score.modsLegacy & ModFlags::Relax;
                    const float touchDevice = score.modsLegacy & ModFlags::TouchDevice;

                    // 2) load hitobjects for diffcalc
                    DatabaseBeatmap::LOAD_DIFFOBJ_RESULT diffres =
                        DatabaseBeatmap::loadDifficultyHitObjects(osuFilePath, AR, CS, speedMultiplier);
                    if(diffres.diffobjects.size() < 1) {
                        if(Osu::debug->getBool()) debugLog("PPRecalc couldn't load %s\n", osuFilePath.c_str());

                        continue;
                    }

                    // 3) calculate stars
                    double aimStars = 0.0;
                    double aimSliderFactor = 0.0;
                    double speedStars = 0.0;
                    double speedNotes = 0.0;
                    const double totalStars = DifficultyCalculator::calculateStarDiffForHitObjects(
                        diffres.diffobjects, CS, OD, speedMultiplier, relax, touchDevice, &aimStars, &aimSliderFactor,
                        &speedStars, &speedNotes);

                    // 4) calculate pp
                    double pp = 0.0;
                    int numHitObjects = 0;
                    int numSpinners = 0;
                    int numCircles = 0;
                    int numSliders = 0;
                    int maxPossibleCombo = 0;
                    {
                        // calculate a few values fresh from the beatmap data necessary for pp calculation
                        numHitObjects = diffres.diffobjects.size();

                        for(size_t h = 0; h < diffres.diffobjects.size(); h++) {
                            if(diffres.diffobjects[h].type == OsuDifficultyHitObject::TYPE::CIRCLE) numCircles++;
                            if(diffres.diffobjects[h].type == OsuDifficultyHitObject::TYPE::SLIDER) numSliders++;
                            if(diffres.diffobjects[h].type == OsuDifficultyHitObject::TYPE::SPINNER) numSpinners++;
                        }

                        maxPossibleCombo = diffres.maxPossibleCombo;
                        if(maxPossibleCombo < 1) continue;

                        pp = DifficultyCalculator::calculatePPv2(
                            score.modsLegacy, speedMultiplier, AR, OD, aimStars, aimSliderFactor, speedStars,
                            speedNotes, numHitObjects, numCircles, numSliders, numSpinners, maxPossibleCombo,
                            score.comboMax, score.numMisses, score.num300s, score.num100s, score.num50s);
                    }

                    // 5) overwrite score with new pp data (and handle imports)
                    if(pp > 0.0f) {
                        score.pp = pp;
                        score.version = LiveScore::VERSION;

                        if(m_bImportLegacyScores && score.isLegacyScore) {
                            score.isLegacyScore = false;  // convert to neosu (pp) score
                            score.isImportedLegacyScore =
                                true;  // but remember that this score does not have all play data
                            {
                                score.numSliderBreaks = 0;
                                score.unstableRate = 0.0f;
                                score.hitErrorAvgMin = 0.0f;
                                score.hitErrorAvgMax = 0.0f;
                            }
                            score.starsTomTotal = totalStars;
                            score.starsTomAim = aimStars;
                            score.starsTomSpeed = speedStars;
                            score.speedMultiplier = speedMultiplier;
                            score.CS = CS;
                            score.AR = AR;
                            score.OD = OD;
                            score.HP = HP;
                            score.maxPossibleCombo = maxPossibleCombo;
                            score.numHitObjects = numHitObjects;
                            score.numCircles = numCircles;
                        }
                    }

                    m_iNumScoresRecalculated++;
                }
            }

            if(m_bInterrupted.load()) break;
        }

        m_bAsyncReady = true;
    }

    virtual void destroy() { ; }

    Osu *m_osu;
    UString m_sUserName;
    bool m_bImportLegacyScores;

    std::atomic<int> m_iNumScoresToRecalculate;
    std::atomic<int> m_iNumScoresRecalculated;
};

UserStatsScreen::UserStatsScreen(Osu *osu) : ScreenBackable(osu) {
    m_name_ref = convar->getConVarByName("name");

    m_contextMenu = new UIContextMenu(m_osu);
    m_contextMenu->setVisible(true);

    m_ppVersionInfoLabel = new UIUserStatsScreenLabel(m_osu);
    m_ppVersionInfoLabel->setText(UString::format("pp Version: %i", DifficultyCalculator::PP_ALGORITHM_VERSION));
    m_ppVersionInfoLabel->setTooltipText(
        "NOTE: This version number does NOT mean your scores have already been recalculated!\nNOTE: Click the gear "
        "button on the right and \"Recalculate pp\".\n \nThis version number reads as the year YYYY and then month MM "
        "and then day DD.\nThat date specifies when the last pp/star algorithm changes were done/released by "
        "peppy.\nneosu always uses the in-use-for-public-global-online-rankings algorithms if possible.");
    m_ppVersionInfoLabel->setTextColor(0x77888888 /*0xbbbb0000*/);
    m_ppVersionInfoLabel->setDrawBackground(false);
    m_ppVersionInfoLabel->setDrawFrame(false);
    addBaseUIElement(m_ppVersionInfoLabel);

    m_userButton = new UISongBrowserUserButton(m_osu);
    m_userButton->addTooltipLine("Click to change [User]");
    m_userButton->setClickCallback(fastdelegate::MakeDelegate(this, &UserStatsScreen::onUserClicked));
    addBaseUIElement(m_userButton);

    m_scores = new CBaseUIScrollView();
    m_scores->setBackgroundColor(0xff222222);
    m_scores->setHorizontalScrolling(false);
    m_scores->setVerticalScrolling(true);
    addBaseUIElement(m_scores);

    m_menuButton = new UserStatsScreenMenuButton();
    m_menuButton->setFont(m_osu->getFontIcons());
    {
        UString iconString;
        iconString.insert(0, Icons::GEAR);
        m_menuButton->setText(iconString);
    }
    m_menuButton->setClickCallback(fastdelegate::MakeDelegate(this, &UserStatsScreen::onMenuClicked));
    addBaseUIElement(m_menuButton);

    m_bRecalculatingPP = false;
    m_backgroundPPRecalculator = NULL;

    // TODO: (statistics panel with values (how many plays, average UR/offset+-/score/pp/etc.))
}

UserStatsScreen::~UserStatsScreen() {
    if(m_backgroundPPRecalculator != NULL) {
        m_backgroundPPRecalculator->interruptLoad();
        engine->getResourceManager()->destroyResource(m_backgroundPPRecalculator);
        m_backgroundPPRecalculator = NULL;
    }
}

void UserStatsScreen::draw(Graphics *g) {
    if(!m_bVisible) return;

    if(m_bRecalculatingPP) {
        if(m_backgroundPPRecalculator != NULL) {
            const float numScoresToRecalculate = (float)m_backgroundPPRecalculator->getNumScoresToRecalculate();
            const float percentFinished =
                (numScoresToRecalculate > 0
                     ? (float)m_backgroundPPRecalculator->getNumScoresRecalculated() / numScoresToRecalculate
                     : 0.0f);
            UString loadingMessage =
                UString::format("Recalculating scores ... (%i %%, %i/%i)", (int)(percentFinished * 100.0f),
                                m_backgroundPPRecalculator->getNumScoresRecalculated(),
                                m_backgroundPPRecalculator->getNumScoresToRecalculate());

            g->setColor(0xffffffff);
            g->pushTransform();
            {
                g->translate(
                    (int)(m_osu->getScreenWidth() / 2 - m_osu->getSubTitleFont()->getStringWidth(loadingMessage) / 2),
                    m_osu->getScreenHeight() - 15);
                g->drawString(m_osu->getSubTitleFont(), loadingMessage);
            }
            g->popTransform();
        }

        m_osu->getHUD()->drawBeatmapImportSpinner(g);

        ScreenBackable::draw(g);
        return;
    }

    ScreenBackable::draw(g);
    m_contextMenu->draw(g);
}

void UserStatsScreen::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible) return;
    ScreenBackable::mouse_update(propagate_clicks);

    if(m_bRecalculatingPP) {
        // wait until recalc is finished
        if(m_backgroundPPRecalculator != NULL && m_backgroundPPRecalculator->isReady()) {
            // force recalc + refresh UI
            m_osu->getSongBrowser()->getDatabase()->forceScoreUpdateOnNextCalculatePlayerStats();
            m_osu->getSongBrowser()->getDatabase()->forceScoresSaveOnNextShutdown();

            rebuildScoreButtons(m_name_ref->getString());

            m_bRecalculatingPP = false;
        } else
            return;  // don't update rest of UI while recalcing
    }

    m_contextMenu->mouse_update(propagate_clicks);

    updateLayout();
}

CBaseUIContainer *UserStatsScreen::setVisible(bool visible) {
    ScreenBackable::setVisible(visible);

    if(m_bVisible) {
        rebuildScoreButtons(m_name_ref->getString());
    } else {
        m_contextMenu->setVisible2(false);
    }

    return this;
}

void UserStatsScreen::onScoreContextMenu(UISongBrowserScoreButton *scoreButton, int id) {
    // NOTE: see UISongBrowserScoreButton::onContextMenu()

    if(id == 2) rebuildScoreButtons(m_name_ref->getString());
}

void UserStatsScreen::onBack() {
    if(m_bRecalculatingPP) {
        if(m_backgroundPPRecalculator != NULL)
            m_backgroundPPRecalculator->interruptLoad();
        else
            m_bRecalculatingPP = false;  // this should never happen
    } else {
        engine->getSound()->play(m_osu->getSkin()->getMenuClick());
        m_osu->toggleSongBrowser();
    }
}

void UserStatsScreen::rebuildScoreButtons(UString playerName) {
    // since this score list can grow very large, UI elements are not cached, but rebuilt completely every time

    // hard reset (delete)
    m_scores->getContainer()->clear();
    m_scoreButtons.clear();

    Database *db = m_osu->getSongBrowser()->getDatabase();
    std::vector<FinishedScore *> scores = db->getPlayerPPScores(playerName).ppScores;
    for(int i = scores.size() - 1; i >= std::max(0, (int)scores.size() - osu_ui_top_ranks_max.getInt()); i--) {
        const float weight = Database::getWeightForIndex(scores.size() - 1 - i);

        const DatabaseBeatmap *diff = db->getBeatmapDifficulty(scores[i]->md5hash);

        UString title = "...";
        if(diff != NULL) {
            title = diff->getArtist().c_str();
            title.append(" - ");
            title.append(diff->getTitle().c_str());
            title.append(" [");
            title.append(diff->getDifficultyName().c_str());
            title.append("]");
        }

        UISongBrowserScoreButton *button = new UISongBrowserScoreButton(m_osu, m_contextMenu, 0, 0, 300, 100,
                                                                        UISongBrowserScoreButton::STYLE::TOP_RANKS);
        button->map_hash = scores[i]->md5hash;
        button->setScore(*scores[i], NULL, scores.size() - i, title, weight);
        button->setClickCallback(fastdelegate::MakeDelegate(this, &UserStatsScreen::onScoreClicked));

        m_scoreButtons.push_back(button);
        m_scores->getContainer()->addBaseUIElement(button);
    }

    m_userButton->setText(playerName);
    m_osu->getOptionsMenu()->setUsername(
        playerName);  // NOTE: force update options textbox to avoid shutdown inconsistency
    m_userButton->updateUserStats();

    updateLayout();
}

void UserStatsScreen::onUserClicked(CBaseUIButton *button) {
    engine->getSound()->play(m_osu->getSkin()->getMenuClick());

    // NOTE: code duplication (see OsuSongbrowser2.cpp)
    std::vector<UString> names = m_osu->getSongBrowser()->getDatabase()->getPlayerNamesWithScoresForUserSwitcher();
    if(names.size() > 0) {
        m_contextMenu->setPos(m_userButton->getPos() + Vector2(0, m_userButton->getSize().y));
        m_contextMenu->setRelPos(m_userButton->getPos() + Vector2(0, m_userButton->getSize().y));
        m_contextMenu->begin(m_userButton->getSize().x);
        m_contextMenu->addButton("Switch User:", 0)
            ->setTextColor(0xff888888)
            ->setTextDarkColor(0xff000000)
            ->setTextLeft(false)
            ->setEnabled(false);
        // m_contextMenu->addButton("", 0)->setEnabled(false);
        for(int i = 0; i < names.size(); i++) {
            CBaseUIButton *button = m_contextMenu->addButton(names[i]);
            if(names[i] == m_name_ref->getString()) button->setTextBrightColor(0xff00ff00);
        }
        m_contextMenu->end(false, true);
        m_contextMenu->setClickCallback(fastdelegate::MakeDelegate(this, &UserStatsScreen::onUserButtonChange));
        UIContextMenu::clampToRightScreenEdge(m_contextMenu);
    }
}

void UserStatsScreen::onUserButtonChange(UString text, int id) {
    if(id == 0) return;

    if(text != m_name_ref->getString()) {
        m_name_ref->setValue(text);
        rebuildScoreButtons(m_name_ref->getString());
    }
}

void UserStatsScreen::onScoreClicked(CBaseUIButton *button) {
    m_osu->toggleSongBrowser();
    m_osu->getSongBrowser()->highlightScore(((UISongBrowserScoreButton *)button)->getScore().unixTimestamp);
}

void UserStatsScreen::onMenuClicked(CBaseUIButton *button) {
    m_contextMenu->setPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
    m_contextMenu->setRelPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
    m_contextMenu->begin();
    {
        m_contextMenu->addButton("Recalculate pp", 1);
        CBaseUIButton *spacer = m_contextMenu->addButton("---");
        spacer->setEnabled(false);
        spacer->setTextColor(0xff888888);
        spacer->setTextDarkColor(0xff000000);
        {
            UString importText = "Import osu! Scores of \"";
            importText.append(m_name_ref->getString());
            importText.append("\"");
            m_contextMenu->addButton(importText, 2);
        }
        spacer = m_contextMenu->addButton("-");
        spacer->setEnabled(false);
        spacer->setTextColor(0xff888888);
        spacer->setTextDarkColor(0xff000000);
        {
            UString copyText = "Copy All Scores from ...";
            m_contextMenu->addButton(copyText, 3);
        }
        spacer = m_contextMenu->addButton("---");
        spacer->setEnabled(false);
        spacer->setTextColor(0xff888888);
        spacer->setTextDarkColor(0xff000000);
        {
            UString deleteText = "Delete All Scores of \"";
            deleteText.append(m_name_ref->getString());
            deleteText.append("\"");
            m_contextMenu->addButton(deleteText, 4);
        }
    }
    m_contextMenu->end(false, false);
    m_contextMenu->setClickCallback(fastdelegate::MakeDelegate(this, &UserStatsScreen::onMenuSelected));
    UIContextMenu::clampToRightScreenEdge(m_contextMenu);
}

void UserStatsScreen::onMenuSelected(UString text, int id) {
    if(id == 1)
        onRecalculatePP(false);
    else if(id == 2)
        onRecalculatePPImportLegacyScoresClicked();
    else if(id == 3)
        onCopyAllScoresClicked();
    else if(id == 4)
        onDeleteAllScoresClicked();
}

void UserStatsScreen::onRecalculatePPImportLegacyScoresClicked() {
    m_contextMenu->setPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
    m_contextMenu->setRelPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
    m_contextMenu->begin();
    {
        {
            UString reallyText = "Really import all osu! scores of \"";
            reallyText.append(m_name_ref->getString());
            reallyText.append("\"?");
            m_contextMenu->addButton(reallyText)->setEnabled(false);
        }
        {
            UString reallyText2 = "NOTE: You can NOT mix-and-match-import.";
            CBaseUIButton *reallyText2Button = m_contextMenu->addButton(reallyText2);
            reallyText2Button->setEnabled(false);
            reallyText2Button->setTextColor(0xff555555);
            reallyText2Button->setTextDarkColor(0xff000000);
        }
        {
            UString reallyText3 = "Only scores matching the EXACT username";
            CBaseUIButton *reallyText3Button = m_contextMenu->addButton(reallyText3);
            reallyText3Button->setEnabled(false);
            reallyText3Button->setTextColor(0xff555555);
            reallyText3Button->setTextDarkColor(0xff000000);

            UString reallyText4 = "of your currently selected profile are imported.";
            CBaseUIButton *reallyText4Button = m_contextMenu->addButton(reallyText4);
            reallyText4Button->setEnabled(false);
            reallyText4Button->setTextColor(0xff555555);
            reallyText4Button->setTextDarkColor(0xff000000);
        }
        CBaseUIButton *spacer = m_contextMenu->addButton("---");
        spacer->setTextLeft(false);
        spacer->setEnabled(false);
        spacer->setTextColor(0xff888888);
        spacer->setTextDarkColor(0xff000000);
        m_contextMenu->addButton("Yes", 1)->setTextLeft(false);
        m_contextMenu->addButton("No")->setTextLeft(false);
    }
    m_contextMenu->end(false, false);
    m_contextMenu->setClickCallback(
        fastdelegate::MakeDelegate(this, &UserStatsScreen::onRecalculatePPImportLegacyScoresConfirmed));
    UIContextMenu::clampToRightScreenEdge(m_contextMenu);
}

void UserStatsScreen::onRecalculatePPImportLegacyScoresConfirmed(UString text, int id) {
    if(id != 1) return;

    onRecalculatePP(true);
}

void UserStatsScreen::onRecalculatePP(bool importLegacyScores) {
    m_bRecalculatingPP = true;

    if(m_backgroundPPRecalculator != NULL) {
        m_backgroundPPRecalculator->interruptLoad();
        engine->getResourceManager()->destroyResource(m_backgroundPPRecalculator);
        m_backgroundPPRecalculator = NULL;
    }

    m_backgroundPPRecalculator =
        new UserStatsScreenBackgroundPPRecalculator(m_osu, m_name_ref->getString(), importLegacyScores);

    // NOTE: force disable all runtime mods (including all experimental mods!), as they directly influence global
    // GameRules which are used during pp calculation
    m_osu->getModSelector()->resetMods();

    engine->getResourceManager()->requestNextLoadAsync();
    engine->getResourceManager()->loadResource(m_backgroundPPRecalculator);
}

void UserStatsScreen::onCopyAllScoresClicked() {
    std::vector<UString> names = m_osu->getSongBrowser()->getDatabase()->getPlayerNamesWithPPScores();
    {
        // remove ourself
        for(size_t i = 0; i < names.size(); i++) {
            if(names[i] == m_name_ref->getString()) {
                names.erase(names.begin() + i);
                i--;
            }
        }
    }

    if(names.size() < 1) {
        m_osu->getNotificationOverlay()->addNotification("There are no valid users/scores to copy from.");
        return;
    }

    m_contextMenu->setPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
    m_contextMenu->setRelPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
    m_contextMenu->begin();
    {
        m_contextMenu->addButton("Select user to copy from:", 0)
            ->setTextColor(0xff888888)
            ->setTextDarkColor(0xff000000)
            ->setTextLeft(false)
            ->setEnabled(false);
        for(int i = 0; i < names.size(); i++) {
            m_contextMenu->addButton(names[i]);
        }
    }
    m_contextMenu->end(false, true);
    m_contextMenu->setClickCallback(fastdelegate::MakeDelegate(this, &UserStatsScreen::onCopyAllScoresUserSelected));
    UIContextMenu::clampToRightScreenEdge(m_contextMenu);
}

void UserStatsScreen::onCopyAllScoresUserSelected(UString text, int id) {
    m_sCopyAllScoresFromUser = text;

    m_contextMenu->setPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
    m_contextMenu->setRelPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
    m_contextMenu->begin();
    {
        {
            UString reallyText = "Really copy all scores from \"";
            reallyText.append(m_sCopyAllScoresFromUser);
            reallyText.append("\" into \"");
            reallyText.append(m_name_ref->getString());
            reallyText.append("\"?");
            m_contextMenu->addButton(reallyText)->setEnabled(false);
        }
        CBaseUIButton *spacer = m_contextMenu->addButton("---");
        spacer->setTextLeft(false);
        spacer->setEnabled(false);
        spacer->setTextColor(0xff888888);
        spacer->setTextDarkColor(0xff000000);
        m_contextMenu->addButton("Yes", 1)->setTextLeft(false);
        m_contextMenu->addButton("No")->setTextLeft(false);
    }
    m_contextMenu->end(false, false);
    m_contextMenu->setClickCallback(fastdelegate::MakeDelegate(this, &UserStatsScreen::onCopyAllScoresConfirmed));
    UIContextMenu::clampToRightScreenEdge(m_contextMenu);
}

void UserStatsScreen::onCopyAllScoresConfirmed(UString text, int id) {
    if(id != 1) return;
    if(m_sCopyAllScoresFromUser.length() < 1) return;

    const UString &playerNameToCopyInto = m_name_ref->getString();
    std::string splayerNameToCopyInto(playerNameToCopyInto.toUtf8());

    if(playerNameToCopyInto.length() < 1 || m_sCopyAllScoresFromUser == playerNameToCopyInto) return;
    std::string sCopyAllScoresFromUser(m_sCopyAllScoresFromUser.toUtf8());

    debugLog("Copying all scores from \"%s\" into \"%s\"\n", m_sCopyAllScoresFromUser.toUtf8(),
             playerNameToCopyInto.toUtf8());

    std::unordered_map<MD5Hash, std::vector<FinishedScore>> *scores =
        m_osu->getSongBrowser()->getDatabase()->getScores();

    std::vector<FinishedScore> tempScoresToCopy;
    for(auto &kv : *scores) {
        tempScoresToCopy.clear();

        // collect all to-be-copied scores for this beatmap into tempScoresToCopy
        for(size_t i = 0; i < kv.second.size(); i++) {
            const FinishedScore &existingScore = kv.second[i];

            // NOTE: only ever copy neosu scores
            if(!existingScore.isLegacyScore) {
                if(existingScore.playerName == sCopyAllScoresFromUser) {
                    // check if this user already has this exact same score (copied previously) and don't copy if that
                    // is the case
                    bool alreadyCopied = false;
                    for(size_t j = 0; j < kv.second.size(); j++) {
                        const FinishedScore &alreadyCopiedScore = kv.second[j];

                        if(j == i) continue;

                        if(!alreadyCopiedScore.isLegacyScore) {
                            if(alreadyCopiedScore.playerName == splayerNameToCopyInto) {
                                if(existingScore.isScoreEqualToCopiedScoreIgnoringPlayerName(alreadyCopiedScore)) {
                                    alreadyCopied = true;
                                    break;
                                }
                            }
                        }
                    }

                    if(!alreadyCopied) tempScoresToCopy.push_back(existingScore);
                }
            }
        }

        // and copy them into the db
        if(tempScoresToCopy.size() > 0) {
            if(Osu::debug->getBool()) debugLog("Copying %i for %s\n", (int)tempScoresToCopy.size(), kv.first.toUtf8());

            for(size_t i = 0; i < tempScoresToCopy.size(); i++) {
                tempScoresToCopy[i].playerName = splayerNameToCopyInto;  // take ownership of this copied score
                tempScoresToCopy[i].sortHack =
                    m_osu->getSongBrowser()->getDatabase()->getAndIncrementScoreSortHackCounter();

                kv.second.push_back(tempScoresToCopy[i]);  // copy into db
            }
        }
    }

    // force recalc + refresh UI
    m_osu->getSongBrowser()->getDatabase()->forceScoreUpdateOnNextCalculatePlayerStats();
    m_osu->getSongBrowser()->getDatabase()->forceScoresSaveOnNextShutdown();

    rebuildScoreButtons(playerNameToCopyInto);
}

void UserStatsScreen::onDeleteAllScoresClicked() {
    m_contextMenu->setPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
    m_contextMenu->setRelPos(m_menuButton->getPos() + Vector2(0, m_menuButton->getSize().y));
    m_contextMenu->begin();
    {
        {
            UString reallyText = "Really delete all scores of \"";
            reallyText.append(m_name_ref->getString());
            reallyText.append("\"?");
            m_contextMenu->addButton(reallyText)->setEnabled(false);
        }
        CBaseUIButton *spacer = m_contextMenu->addButton("---");
        spacer->setTextLeft(false);
        spacer->setEnabled(false);
        spacer->setTextColor(0xff888888);
        spacer->setTextDarkColor(0xff000000);
        m_contextMenu->addButton("Yes", 1)->setTextLeft(false);
        m_contextMenu->addButton("No")->setTextLeft(false);
    }
    m_contextMenu->end(false, false);
    m_contextMenu->setClickCallback(fastdelegate::MakeDelegate(this, &UserStatsScreen::onDeleteAllScoresConfirmed));
    UIContextMenu::clampToRightScreenEdge(m_contextMenu);
}

void UserStatsScreen::onDeleteAllScoresConfirmed(UString text, int id) {
    if(id != 1) return;

    const UString &playerName = m_name_ref->getString();
    std::string splayerName(playerName.toUtf8());

    debugLog("Deleting all scores for \"%s\"\n", playerName.toUtf8());

    std::unordered_map<MD5Hash, std::vector<FinishedScore>> *scores =
        m_osu->getSongBrowser()->getDatabase()->getScores();

    // delete every score matching the current playerName
    for(auto &kv : *scores) {
        for(size_t i = 0; i < kv.second.size(); i++) {
            const FinishedScore &score = kv.second[i];

            // NOTE: only ever delete neosu scores
            if(!score.isLegacyScore) {
                if(score.playerName == splayerName) {
                    kv.second.erase(kv.second.begin() + i);
                    i--;
                }
            }
        }
    }

    // force recalc + refresh UI
    m_osu->getSongBrowser()->getDatabase()->forceScoreUpdateOnNextCalculatePlayerStats();
    m_osu->getSongBrowser()->getDatabase()->forceScoresSaveOnNextShutdown();

    rebuildScoreButtons(playerName);
}

void UserStatsScreen::updateLayout() {
    ScreenBackable::updateLayout();

    const float dpiScale = Osu::getUIScale(m_osu);

    setSize(m_osu->getScreenSize());

    const int scoreListHeight = m_osu->getScreenHeight() * 0.8f;
    m_scores->setSize(m_osu->getScreenWidth() * 0.6f, scoreListHeight);
    m_scores->setPos(m_osu->getScreenWidth() / 2 - m_scores->getSize().x / 2,
                     m_osu->getScreenHeight() - scoreListHeight);

    const int margin = 5 * dpiScale;
    const int padding = 5 * dpiScale;

    // NOTE: these can't really be uiScale'd, because of the fixed aspect ratio
    const int scoreButtonWidth = m_scores->getSize().x * 0.92f - 2 * margin;
    const int scoreButtonHeight = scoreButtonWidth * 0.065f;
    for(int i = 0; i < m_scoreButtons.size(); i++) {
        m_scoreButtons[i]->setSize(scoreButtonWidth - 2, scoreButtonHeight);
        m_scoreButtons[i]->setRelPos(margin, margin + i * (scoreButtonHeight + padding));
    }
    m_scores->getContainer()->update_pos();

    m_scores->setScrollSizeToContent();

    const int userButtonHeight = m_scores->getPos().y * 0.6f;
    m_userButton->setSize(userButtonHeight * 3.5f, userButtonHeight);
    m_userButton->setPos(m_osu->getScreenWidth() / 2 - m_userButton->getSize().x / 2,
                         m_scores->getPos().y / 2 - m_userButton->getSize().y / 2);

    m_menuButton->setSize(userButtonHeight * 0.9f, userButtonHeight * 0.9f);
    m_menuButton->setPos(
        std::max(m_userButton->getPos().x + m_userButton->getSize().x,
                 m_userButton->getPos().x + m_userButton->getSize().x +
                     (m_userButton->getPos().x - m_scores->getPos().x) / 2 - m_menuButton->getSize().x / 2),
        m_userButton->getPos().y + m_userButton->getSize().y / 2 - m_menuButton->getSize().y / 2);

    m_ppVersionInfoLabel->onResized();  // HACKHACK: framework bug (should update string metrics on setSizeToContent())
    m_ppVersionInfoLabel->setSizeToContent(1, 10);
    {
        const Vector2 center = m_userButton->getPos() + Vector2(0, m_userButton->getSize().y / 2) -
                               Vector2((m_userButton->getPos().x - m_scores->getPos().x) / 2, 0);
        const Vector2 topLeft = center - m_ppVersionInfoLabel->getSize() / 2;
        const float overflow = (center.x + m_ppVersionInfoLabel->getSize().x / 2) - m_userButton->getPos().x;

        m_ppVersionInfoLabel->setPos((int)(topLeft.x - std::max(overflow, 0.0f)), (int)(topLeft.y));
    }
}
