#ifdef _WIN32
#include "cbase.h"
#else
#include <sys/random.h>
#endif

#include <string.h>

#include "AnimationHandler.h"
#include "BackgroundImageHandler.h"
#include "Bancho.h"
#include "BanchoLeaderboard.h"
#include "BanchoNetworking.h"
#include "Beatmap.h"
#include "CBaseUIContainer.h"
#include "CBaseUIImageButton.h"
#include "CBaseUILabel.h"
#include "CBaseUIScrollView.h"
#include "Chat.h"
#include "CollectionButton.h"
#include "Collections.h"
#include "ConVar.h"
#include "Database.h"
#include "DatabaseBeatmap.h"
#include "Downloader.h"
#include "Engine.h"
#include "HUD.h"
#include "Icons.h"
#include "InfoLabel.h"
#include "KeyBindings.h"
#include "Keyboard.h"
#include "LeaderboardPPCalcThread.h"
#include "LoudnessCalcThread.h"
#include "MainMenu.h"
#include "MapCalcThread.h"
#include "ModSelector.h"
#include "Mouse.h"
#include "NotificationOverlay.h"
#include "OptionsMenu.h"
#include "Osu.h"
#include "RankingScreen.h"
#include "ResourceManager.h"
#include "RichPresence.h"
#include "RoomScreen.h"
#include "ScoreButton.h"
#include "ScoreConverterThread.h"
#include "Skin.h"
#include "SkinImage.h"
#include "SongBrowser.h"
#include "SongButton.h"
#include "SongDifficultyButton.h"
#include "SoundEngine.h"
#include "Timer.h"
#include "UIBackButton.h"
#include "UIContextMenu.h"
#include "UISearchOverlay.h"
#include "UISelectionButton.h"
#include "UserCard.h"
#include "VertexArrayObject.h"

class SongBrowserBackgroundSearchMatcher : public Resource {
   public:
    SongBrowserBackgroundSearchMatcher() : Resource() {
        m_bDead = true;  // NOTE: start dead! need to revive() before use
    }

    bool isDead() const { return m_bDead.load(); }
    void kill() { m_bDead = true; }
    void revive() { m_bDead = false; }

    void setSongButtonsAndSearchString(const std::vector<SongButton *> &songButtons, const UString &searchString,
                                       const UString &hardcodedSearchString) {
        m_songButtons = songButtons;

        m_sSearchString.clear();
        if(hardcodedSearchString.length() > 0) {
            m_sSearchString.append(hardcodedSearchString);
            m_sSearchString.append(" ");
        }
        m_sSearchString.append(searchString);
        m_sHardcodedSearchString = hardcodedSearchString;
    }

   protected:
    virtual void init() { m_bReady = true; }

    virtual void initAsync() {
        if(m_bDead.load()) {
            m_bAsyncReady = true;
            return;
        }

        // flag matches across entire database
        const std::vector<UString> searchStringTokens = m_sSearchString.split(" ");
        for(size_t i = 0; i < m_songButtons.size(); i++) {
            const auto &children = m_songButtons[i]->getChildren();
            if(children.size() > 0) {
                for(size_t c = 0; c < children.size(); c++) {
                    children[c]->setIsSearchMatch(
                        SongBrowser::searchMatcher(children[c]->getDatabaseBeatmap(), searchStringTokens));
                }
            } else
                m_songButtons[i]->setIsSearchMatch(
                    SongBrowser::searchMatcher(m_songButtons[i]->getDatabaseBeatmap(), searchStringTokens));

            // cancellation point
            if(m_bDead.load()) break;
        }

        m_bAsyncReady = true;
    }

    virtual void destroy() { ; }

   private:
    std::atomic<bool> m_bDead;

    UString m_sSearchString;
    UString m_sHardcodedSearchString;
    std::vector<SongButton *> m_songButtons;
};

class ScoresStillLoadingElement : public CBaseUILabel {
   public:
    ScoresStillLoadingElement(UString text) : CBaseUILabel(0, 0, 0, 0, "", text) {
        m_sIconString.insert(0, Icons::GLOBE);
    }

    virtual void drawText(Graphics *g) {
        // draw icon
        const float iconScale = 0.6f;
        McFont *iconFont = osu->getFontIcons();
        int iconWidth = 0;
        g->pushTransform();
        {
            const float scale = (m_vSize.y / iconFont->getHeight()) * iconScale;
            const float paddingLeft = scale * 15;

            iconWidth = paddingLeft + iconFont->getStringWidth(m_sIconString) * scale;

            g->scale(scale, scale);
            g->translate((int)(m_vPos.x + paddingLeft),
                         (int)(m_vPos.y + m_vSize.y / 2 + iconFont->getHeight() * scale / 2));
            g->setColor(0xffffffff);
            g->drawString(iconFont, m_sIconString);
        }
        g->popTransform();

        // draw text
        const float textScale = 0.4f;
        McFont *textFont = osu->getSongBrowserFont();
        g->pushTransform();
        {
            const float stringWidth = textFont->getStringWidth(m_sText);

            const float scale = ((m_vSize.x - iconWidth) / stringWidth) * textScale;

            g->scale(scale, scale);
            g->translate((int)(m_vPos.x + iconWidth + (m_vSize.x - iconWidth) / 2 - stringWidth * scale / 2),
                         (int)(m_vPos.y + m_vSize.y / 2 + textFont->getHeight() * scale / 2));
            g->setColor(0xff02c3e5);
            g->drawString(textFont, m_sText);
        }
        g->popTransform();
    }

   private:
    UString m_sIconString;
};

class NoRecordsSetElement : public CBaseUILabel {
   public:
    NoRecordsSetElement(UString text) : CBaseUILabel(0, 0, 0, 0, "", text) { m_sIconString.insert(0, Icons::TROPHY); }

    virtual void drawText(Graphics *g) {
        // draw icon
        const float iconScale = 0.6f;
        McFont *iconFont = osu->getFontIcons();
        int iconWidth = 0;
        g->pushTransform();
        {
            const float scale = (m_vSize.y / iconFont->getHeight()) * iconScale;
            const float paddingLeft = scale * 15;

            iconWidth = paddingLeft + iconFont->getStringWidth(m_sIconString) * scale;

            g->scale(scale, scale);
            g->translate((int)(m_vPos.x + paddingLeft),
                         (int)(m_vPos.y + m_vSize.y / 2 + iconFont->getHeight() * scale / 2));
            g->setColor(0xffffffff);
            g->drawString(iconFont, m_sIconString);
        }
        g->popTransform();

        // draw text
        const float textScale = 0.6f;
        McFont *textFont = osu->getSongBrowserFont();
        g->pushTransform();
        {
            const float stringWidth = textFont->getStringWidth(m_sText);

            const float scale = ((m_vSize.x - iconWidth) / stringWidth) * textScale;

            g->scale(scale, scale);
            g->translate((int)(m_vPos.x + iconWidth + (m_vSize.x - iconWidth) / 2 - stringWidth * scale / 2),
                         (int)(m_vPos.y + m_vSize.y / 2 + textFont->getHeight() * scale / 2));
            g->setColor(0xff02c3e5);
            g->drawString(textFont, m_sText);
        }
        g->popTransform();
    }

   private:
    UString m_sIconString;
};

bool sort_by_artist(SongButton const *a, SongButton const *b) {
    if(a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL) return a->getSortHack() < b->getSortHack();

    int res = strcasecmp(a->getDatabaseBeatmap()->getArtist().c_str(), b->getDatabaseBeatmap()->getArtist().c_str());
    if(res == 0) return a->getSortHack() < b->getSortHack();
    return res < 0;
}

bool sort_by_bpm(SongButton const *a, SongButton const *b) {
    if(a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL) return a->getSortHack() < b->getSortHack();

    int bpm1 = a->getDatabaseBeatmap()->getMostCommonBPM();
    int bpm2 = b->getDatabaseBeatmap()->getMostCommonBPM();
    if(bpm1 == bpm2) return a->getSortHack() < b->getSortHack();
    return bpm1 < bpm2;
}

bool sort_by_creator(SongButton const *a, SongButton const *b) {
    if(a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL) return a->getSortHack() < b->getSortHack();

    int res = strcasecmp(a->getDatabaseBeatmap()->getCreator().c_str(), b->getDatabaseBeatmap()->getCreator().c_str());
    if(res == 0) return a->getSortHack() < b->getSortHack();
    return res < 0;
}

bool sort_by_date_added(SongButton const *a, SongButton const *b) {
    if(a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL) return a->getSortHack() < b->getSortHack();

    long long time1 = a->getDatabaseBeatmap()->last_modification_time;
    long long time2 = b->getDatabaseBeatmap()->last_modification_time;
    if(time1 == time2) return a->getSortHack() > b->getSortHack();
    return time1 > time2;
}

bool sort_by_difficulty(SongButton const *a, SongButton const *b) {
    if(a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL) return a->getSortHack() < b->getSortHack();

    float stars1 = a->getDatabaseBeatmap()->getStarsNomod();
    float stars2 = b->getDatabaseBeatmap()->getStarsNomod();
    if(stars1 != stars2) return stars1 < stars2;

    float diff1 = (a->getDatabaseBeatmap()->getAR() + 1) * (a->getDatabaseBeatmap()->getCS() + 1) *
                  (a->getDatabaseBeatmap()->getHP() + 1) * (a->getDatabaseBeatmap()->getOD() + 1) *
                  (max(a->getDatabaseBeatmap()->getMostCommonBPM(), 1));
    float diff2 = (b->getDatabaseBeatmap()->getAR() + 1) * (b->getDatabaseBeatmap()->getCS() + 1) *
                  (b->getDatabaseBeatmap()->getHP() + 1) * (b->getDatabaseBeatmap()->getOD() + 1) *
                  (max(b->getDatabaseBeatmap()->getMostCommonBPM(), 1));
    if(diff1 != diff2) return diff1 < diff2;

    return a->getSortHack() < b->getSortHack();
}

bool sort_by_length(SongButton const *a, SongButton const *b) {
    if(a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL) return a->getSortHack() < b->getSortHack();

    unsigned long length1 = a->getDatabaseBeatmap()->getLengthMS();
    unsigned long length2 = b->getDatabaseBeatmap()->getLengthMS();
    if(length1 == length2) return a->getSortHack() < b->getSortHack();
    return length1 < length2;
}

bool sort_by_title(SongButton const *a, SongButton const *b) {
    if(a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL) return a->getSortHack() < b->getSortHack();

    int res = strcasecmp(a->getDatabaseBeatmap()->getTitle().c_str(), b->getDatabaseBeatmap()->getTitle().c_str());
    if(res == 0) return a->getSortHack() < b->getSortHack();
    return res < 0;
}

bool sort_by_grade(SongButton const *a, SongButton const *b) {
    if(a->m_grade == b->m_grade) return a->getSortHack() < b->getSortHack();
    return a->m_grade < b->m_grade;
}

SongBrowser::SongBrowser() : ScreenBackable() {
    // sorting/grouping + methods
    m_group = GROUP::GROUP_NO_GROUPING;
    {
        m_groupings.push_back({GROUP::GROUP_NO_GROUPING, "No Grouping", 0});
        m_groupings.push_back({GROUP::GROUP_ARTIST, "By Artist", 1});
        m_groupings.push_back({GROUP::GROUP_BPM, "By BPM", 2});
        m_groupings.push_back({GROUP::GROUP_CREATOR, "By Creator", 3});
        /// m_groupings.push_back({GROUP::GROUP_DATEADDED, "By Date Added", 4}); // not yet possible
        m_groupings.push_back({GROUP::GROUP_DIFFICULTY, "By Difficulty", 5});
        m_groupings.push_back({GROUP::GROUP_LENGTH, "By Length", 6});
        m_groupings.push_back({GROUP::GROUP_TITLE, "By Title", 7});
        m_groupings.push_back({GROUP::GROUP_COLLECTIONS, "Collections", 8});
    }

    m_sortingMethod = SORT::SORT_ARTIST;
    m_sortingComparator = sort_by_artist;

    m_sortingMethods.push_back({SORT::SORT_ARTIST, "By Artist", sort_by_artist});
    m_sortingMethods.push_back({SORT::SORT_BPM, "By BPM", sort_by_bpm});
    m_sortingMethods.push_back({SORT::SORT_CREATOR, "By Creator", sort_by_creator});
    m_sortingMethods.push_back({SORT::SORT_DATEADDED, "By Date Added", sort_by_date_added});
    m_sortingMethods.push_back({SORT::SORT_DIFFICULTY, "By Difficulty", sort_by_difficulty});
    m_sortingMethods.push_back({SORT::SORT_LENGTH, "By Length", sort_by_length});
    m_sortingMethods.push_back({SORT::SORT_TITLE, "By Title", sort_by_title});
    m_sortingMethods.push_back({SORT::SORT_RANKACHIEVED, "By Rank Achieved", sort_by_grade});

    // vars
    m_bSongBrowserRightClickScrollCheck = false;
    m_bSongBrowserRightClickScrolling = false;
    m_bNextScrollToSongButtonJumpFixScheduled = false;
    m_bNextScrollToSongButtonJumpFixUseScrollSizeDelta = false;
    m_fNextScrollToSongButtonJumpFixOldRelPosY = 0.0f;

    m_selectionPreviousSongButton = NULL;
    m_selectionPreviousSongDiffButton = NULL;
    m_selectionPreviousCollectionButton = NULL;

    m_bF1Pressed = false;
    m_bF2Pressed = false;
    m_bF3Pressed = false;
    m_bShiftPressed = false;
    m_bLeft = false;
    m_bRight = false;

    m_bRandomBeatmapScheduled = false;
    m_bPreviousRandomBeatmapScheduled = false;

    m_fSongSelectTopScale = 1.0f;

    // build topbar left
    m_topbarLeft = new CBaseUIContainer(0, 0, 0, 0, "");
    {
        m_songInfo = new InfoLabel(0, 0, 0, 0, "");

        m_topbarLeft->addBaseUIElement(m_songInfo);
    }

    m_scoreSortButton = addTopBarLeftTabButton("Sort By Score");
    m_scoreSortButton->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onSortScoresClicked));
    m_webButton = addTopBarLeftButton("Web");
    m_webButton->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onWebClicked));

    // build topbar right
    m_topbarRight = new CBaseUIContainer(0, 0, 0, 0, "");

    {
        m_groupLabel = new CBaseUILabel(0, 0, 0, 0, "", "Group:");
        m_groupLabel->setSizeToContent(3);
        m_groupLabel->setDrawFrame(false);
        m_groupLabel->setDrawBackground(false);
        m_topbarRight->addBaseUIElement(m_groupLabel);

        m_groupButton = new CBaseUIButton(0, 0, 0, 0, "", "No Grouping");
        m_groupButton->setDrawBackground(false);
        m_groupButton->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onGroupClicked));
        m_topbarRight->addBaseUIElement(m_groupButton);

        m_sortLabel = new CBaseUILabel(0, 0, 0, 0, "", "Sort:");
        m_sortLabel->setSizeToContent(3);
        m_sortLabel->setDrawFrame(false);
        m_sortLabel->setDrawBackground(false);
        m_topbarRight->addBaseUIElement(m_sortLabel);

        m_sortButton = new CBaseUIButton(0, 0, 0, 0, "", "By Date Added");
        m_sortButton->setDrawBackground(false);
        m_sortButton->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onSortClicked));
        m_topbarRight->addBaseUIElement(m_sortButton);
    }

    // context menu
    m_contextMenu = new UIContextMenu(50, 50, 150, 0, "");
    m_contextMenu->setVisible(true);

    // build bottombar
    m_bottombar = new CBaseUIContainer(0, 0, 0, 0, "");

    addBottombarNavButton([]() -> SkinImage * { return osu->getSkin()->getSelectionMode(); },
                          []() -> SkinImage * { return osu->getSkin()->getSelectionModeOver(); })
        ->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onSelectionMode));
    addBottombarNavButton([]() -> SkinImage * { return osu->getSkin()->getSelectionMods(); },
                          []() -> SkinImage * { return osu->getSkin()->getSelectionModsOver(); })
        ->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onSelectionMods));
    addBottombarNavButton([]() -> SkinImage * { return osu->getSkin()->getSelectionRandom(); },
                          []() -> SkinImage * { return osu->getSkin()->getSelectionRandomOver(); })
        ->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onSelectionRandom));
    addBottombarNavButton([]() -> SkinImage * { return osu->getSkin()->getSelectionOptions(); },
                          []() -> SkinImage * { return osu->getSkin()->getSelectionOptionsOver(); })
        ->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onSelectionOptions));

    m_userButton = new UserCard(bancho.user_id);
    m_bottombar->addBaseUIElement(m_userButton);

    // build scorebrowser
    m_scoreBrowser = new CBaseUIScrollView(0, 0, 0, 0, "");
    m_scoreBrowser->setDrawBackground(false);
    m_scoreBrowser->setDrawFrame(false);
    m_scoreBrowser->setHorizontalScrolling(false);
    m_scoreBrowser->setScrollbarSizeMultiplier(0.25f);
    m_scoreBrowser->setScrollResistance(15);
    m_scoreBrowser->m_bHorizontalClipping = false;
    m_scoreBrowserScoresStillLoadingElement = new ScoresStillLoadingElement("Loading...");
    m_scoreBrowserNoRecordsYetElement = new NoRecordsSetElement("No records set!");
    m_scoreBrowser->getContainer()->addBaseUIElement(m_scoreBrowserNoRecordsYetElement);

    m_localBestContainer = new CBaseUIContainer(0, 0, 0, 0, "");
    m_localBestContainer->setVisible(false);
    addBaseUIElement(m_localBestContainer);
    m_localBestLabel = new CBaseUILabel(0, 0, 0, 0, "", "Personal Best (from local scores)");
    m_localBestLabel->setDrawBackground(false);
    m_localBestLabel->setDrawFrame(false);
    m_localBestLabel->setTextJustification(CBaseUILabel::TEXT_JUSTIFICATION::TEXT_JUSTIFICATION_CENTERED);
    m_localBestLabel->setFont(osu->getSubTitleFont());

    // build songbrowser
    m_songBrowser = new CBaseUIScrollView(0, 0, 0, 0, "");
    m_songBrowser->setDrawBackground(false);
    m_songBrowser->setDrawFrame(false);
    m_songBrowser->setHorizontalScrolling(false);
    m_songBrowser->setScrollResistance(15);

    // beatmap database
    m_db = new Database();
    db = m_db;
    m_bBeatmapRefreshScheduled = true;

    // behaviour
    m_bHasSelectedAndIsPlaying = false;
    m_selectedBeatmap = new Beatmap();
    m_fPulseAnimation = 0.0f;
    m_fBackgroundFadeInTime = 0.0f;

    // search
    m_search = new UISearchOverlay(0, 0, 0, 0, "");
    m_search->setOffsetRight(10);
    m_fSearchWaitTime = 0.0f;
    m_bInSearch = (cv_songbrowser_search_hardcoded_filter.getString().length() > 0);
    m_searchPrevGroup = GROUP::GROUP_NO_GROUPING;
    m_backgroundSearchMatcher = new SongBrowserBackgroundSearchMatcher();

    updateLayout();
}

SongBrowser::~SongBrowser() {
    sct_abort();
    lct_set_map(NULL);
    loct_abort();
    mct_abort();
    checkHandleKillBackgroundSearchMatcher();

    engine->getResourceManager()->destroyResource(m_backgroundSearchMatcher);

    m_songBrowser->getContainer()->empty();

    for(size_t i = 0; i < m_songButtons.size(); i++) {
        delete m_songButtons[i];
    }
    for(size_t i = 0; i < m_collectionButtons.size(); i++) {
        delete m_collectionButtons[i];
    }
    for(size_t i = 0; i < m_artistCollectionButtons.size(); i++) {
        delete m_artistCollectionButtons[i];
    }
    for(size_t i = 0; i < m_bpmCollectionButtons.size(); i++) {
        delete m_bpmCollectionButtons[i];
    }
    for(size_t i = 0; i < m_difficultyCollectionButtons.size(); i++) {
        delete m_difficultyCollectionButtons[i];
    }
    for(size_t i = 0; i < m_creatorCollectionButtons.size(); i++) {
        delete m_creatorCollectionButtons[i];
    }
    for(size_t i = 0; i < m_dateaddedCollectionButtons.size(); i++) {
        delete m_dateaddedCollectionButtons[i];
    }
    for(size_t i = 0; i < m_lengthCollectionButtons.size(); i++) {
        delete m_lengthCollectionButtons[i];
    }
    for(size_t i = 0; i < m_titleCollectionButtons.size(); i++) {
        delete m_titleCollectionButtons[i];
    }

    m_scoreBrowser->getContainer()->empty();
    for(size_t i = 0; i < m_scoreButtonCache.size(); i++) {
        delete m_scoreButtonCache[i];
    }
    SAFE_DELETE(m_scoreBrowserScoresStillLoadingElement);
    SAFE_DELETE(m_scoreBrowserNoRecordsYetElement);

    SAFE_DELETE(m_selectedBeatmap);
    SAFE_DELETE(m_search);
    SAFE_DELETE(m_topbarLeft);
    SAFE_DELETE(m_topbarRight);
    SAFE_DELETE(m_bottombar);
    SAFE_DELETE(m_scoreBrowser);
    SAFE_DELETE(m_songBrowser);
    SAFE_DELETE(m_db);

    // Memory leak on shutdown, maybe
    empty();
}

void SongBrowser::draw(Graphics *g) {
    if(!m_bVisible) return;

    // draw background
    g->setColor(0xff000000);
    g->fillRect(0, 0, osu->getScreenWidth(), osu->getScreenHeight());

    // refreshing (blocks every other call in draw() below it!)
    if(m_bBeatmapRefreshScheduled) {
        UString loadingMessage = UString::format("Loading beatmaps ... (%i %%)", (int)(m_db->getProgress() * 100.0f));

        g->setColor(0xffffffff);
        g->pushTransform();
        {
            g->translate((int)(osu->getScreenWidth() / 2 - osu->getSubTitleFont()->getStringWidth(loadingMessage) / 2),
                         osu->getScreenHeight() - 15);
            g->drawString(osu->getSubTitleFont(), loadingMessage);
        }
        g->popTransform();

        osu->getHUD()->drawBeatmapImportSpinner(g);
        return;
    }

    // draw background image
    if(cv_draw_songbrowser_background_image.getBool()) {
        float alpha = 1.0f;
        if(cv_songbrowser_background_fade_in_duration.getFloat() > 0.0f) {
            // handle fadein trigger after handler is finished loading
            const bool ready = osu->getSelectedBeatmap()->getSelectedDifficulty2() != NULL &&
                               osu->getBackgroundImageHandler()->getLoadBackgroundImage(
                                   osu->getSelectedBeatmap()->getSelectedDifficulty2()) != NULL &&
                               osu->getBackgroundImageHandler()
                                   ->getLoadBackgroundImage(osu->getSelectedBeatmap()->getSelectedDifficulty2())
                                   ->isReady();

            if(!ready)
                m_fBackgroundFadeInTime = engine->getTime();
            else if(m_fBackgroundFadeInTime > 0.0f && engine->getTime() > m_fBackgroundFadeInTime) {
                alpha = clamp<float>((engine->getTime() - m_fBackgroundFadeInTime) /
                                         cv_songbrowser_background_fade_in_duration.getFloat(),
                                     0.0f, 1.0f);
                alpha = 1.0f - (1.0f - alpha) * (1.0f - alpha);
            }
        }

        drawSelectedBeatmapBackgroundImage(g, alpha);
    } else if(cv_draw_songbrowser_menu_background_image.getBool()) {
        // menu-background
        Image *backgroundImage = osu->getSkin()->getMenuBackground();
        if(backgroundImage != NULL && backgroundImage != osu->getSkin()->getMissingTexture() &&
           backgroundImage->isReady()) {
            const float scale = Osu::getImageScaleToFillResolution(backgroundImage, osu->getScreenSize());

            g->setColor(0xffffffff);
            g->pushTransform();
            {
                g->scale(scale, scale);
                g->translate(osu->getScreenWidth() / 2, osu->getScreenHeight() / 2);
                g->drawImage(backgroundImage);
            }
            g->popTransform();
        }
    }

    // draw score browser
    m_scoreBrowser->draw(g);

    // draw strain graph of currently selected beatmap
    if(cv_draw_songbrowser_strain_graph.getBool()) {
        const std::vector<double> &aimStrains = getSelectedBeatmap()->m_aimStrains;
        const std::vector<double> &speedStrains = getSelectedBeatmap()->m_speedStrains;
        const float speedMultiplier = osu->getSpeedMultiplier();

        if(aimStrains.size() > 0 && aimStrains.size() == speedStrains.size()) {
            const float strainStepMS = 400.0f * speedMultiplier;

            const unsigned long lengthMS = strainStepMS * aimStrains.size();

            // get highest strain values for normalization
            double highestAimStrain = 0.0;
            double highestSpeedStrain = 0.0;
            double highestStrain = 0.0;
            int highestStrainIndex = -1;
            for(int i = 0; i < aimStrains.size(); i++) {
                const double aimStrain = aimStrains[i];
                const double speedStrain = speedStrains[i];
                const double strain = aimStrain + speedStrain;

                if(strain > highestStrain) {
                    highestStrain = strain;
                    highestStrainIndex = i;
                }
                if(aimStrain > highestAimStrain) highestAimStrain = aimStrain;
                if(speedStrain > highestSpeedStrain) highestSpeedStrain = speedStrain;
            }

            // draw strain bar graph
            if(highestAimStrain > 0.0 && highestSpeedStrain > 0.0 && highestStrain > 0.0) {
                const float dpiScale = Osu::getUIScale();

                const float graphWidth = m_scoreBrowser->getSize().x;

                const float msPerPixel = (float)lengthMS / graphWidth;
                const float strainWidth = strainStepMS / msPerPixel;
                const float strainHeightMultiplier = cv_hud_scrubbing_timeline_strains_height.getFloat() * dpiScale;

                McRect graphRect(0, m_bottombar->getPos().y - strainHeightMultiplier, graphWidth,
                                 strainHeightMultiplier);

                const float alpha = (graphRect.contains(engine->getMouse()->getPos())
                                         ? 1.0f
                                         : cv_hud_scrubbing_timeline_strains_alpha.getFloat());

                const Color aimStrainColor =
                    COLORf(alpha, cv_hud_scrubbing_timeline_strains_aim_color_r.getInt() / 255.0f,
                           cv_hud_scrubbing_timeline_strains_aim_color_g.getInt() / 255.0f,
                           cv_hud_scrubbing_timeline_strains_aim_color_b.getInt() / 255.0f);
                const Color speedStrainColor =
                    COLORf(alpha, cv_hud_scrubbing_timeline_strains_speed_color_r.getInt() / 255.0f,
                           cv_hud_scrubbing_timeline_strains_speed_color_g.getInt() / 255.0f,
                           cv_hud_scrubbing_timeline_strains_speed_color_b.getInt() / 255.0f);

                g->setDepthBuffer(true);
                for(int i = 0; i < aimStrains.size(); i++) {
                    const double aimStrain = (aimStrains[i]) / highestStrain;
                    const double speedStrain = (speedStrains[i]) / highestStrain;
                    // const double strain = (aimStrains[i] + speedStrains[i]) / highestStrain;

                    const double aimStrainHeight = aimStrain * strainHeightMultiplier;
                    const double speedStrainHeight = speedStrain * strainHeightMultiplier;
                    // const double strainHeight = strain * strainHeightMultiplier;

                    if(!engine->getKeyboard()->isShiftDown()) {
                        g->setColor(aimStrainColor);
                        g->fillRect(i * strainWidth, m_bottombar->getPos().y - aimStrainHeight,
                                    max(1.0f, std::round(strainWidth + 0.5f)), aimStrainHeight);
                    }

                    if(!engine->getKeyboard()->isControlDown()) {
                        g->setColor(speedStrainColor);
                        g->fillRect(i * strainWidth,
                                    m_bottombar->getPos().y -
                                        (engine->getKeyboard()->isShiftDown() ? 0 : aimStrainHeight) -
                                        speedStrainHeight,
                                    max(1.0f, std::round(strainWidth + 0.5f)), speedStrainHeight + 1);
                    }
                }
                g->setDepthBuffer(false);

                // highlight highest total strain value (+- section block)
                if(highestStrainIndex > -1) {
                    const double aimStrain = (aimStrains[highestStrainIndex]) / highestStrain;
                    const double speedStrain = (speedStrains[highestStrainIndex]) / highestStrain;
                    // const double strain = (aimStrains[i] + speedStrains[i]) / highestStrain;

                    const double aimStrainHeight = aimStrain * strainHeightMultiplier;
                    const double speedStrainHeight = speedStrain * strainHeightMultiplier;
                    // const double strainHeight = strain * strainHeightMultiplier;

                    Vector2 topLeftCenter = Vector2(highestStrainIndex * strainWidth + strainWidth / 2.0f,
                                                    m_bottombar->getPos().y - aimStrainHeight - speedStrainHeight);

                    const float margin = 5.0f * dpiScale;

                    g->setColor(0xffffffff);
                    g->setAlpha(alpha);
                    g->drawRect(topLeftCenter.x - margin * strainWidth, topLeftCenter.y - margin * strainWidth,
                                strainWidth * 2 * margin,
                                aimStrainHeight + speedStrainHeight + 2 * margin * strainWidth);
                    g->setAlpha(alpha * 0.5f);
                    g->drawRect(topLeftCenter.x - margin * strainWidth - 2, topLeftCenter.y - margin * strainWidth - 2,
                                strainWidth * 2 * margin + 4,
                                aimStrainHeight + speedStrainHeight + 2 * margin * strainWidth + 4);
                    g->setAlpha(alpha * 0.25f);
                    g->drawRect(topLeftCenter.x - margin * strainWidth - 4, topLeftCenter.y - margin * strainWidth - 4,
                                strainWidth * 2 * margin + 8,
                                aimStrainHeight + speedStrainHeight + 2 * margin * strainWidth + 8);
                }
            }
        }
    }

    // draw song browser
    m_songBrowser->draw(g);

    // draw search
    m_search->setSearchString(m_sSearchString, cv_songbrowser_search_hardcoded_filter.getString());
    m_search->setDrawNumResults(m_bInSearch);
    m_search->setNumFoundResults(m_visibleSongButtons.size());
    m_search->setSearching(!m_backgroundSearchMatcher->isDead());
    m_search->draw(g);

    // draw top bar
    g->setColor(0xffffffff);
    g->pushTransform();
    {
        g->scale(m_fSongSelectTopScale, m_fSongSelectTopScale);
        g->translate((osu->getSkin()->getSongSelectTop()->getWidth() * m_fSongSelectTopScale) / 2,
                     (osu->getSkin()->getSongSelectTop()->getHeight() * m_fSongSelectTopScale) / 2);
        g->drawImage(osu->getSkin()->getSongSelectTop());
    }
    g->popTransform();

    m_topbarLeft->draw(g);
    if(cv_debug.getBool()) m_topbarLeft->draw_debug(g);
    m_topbarRight->draw(g);
    if(cv_debug.getBool()) m_topbarRight->draw_debug(g);

    // draw bottom bar
    float songSelectBottomScale = m_bottombar->getSize().y / osu->getSkin()->getSongSelectBottom()->getHeight();
    songSelectBottomScale *= 0.8f;

    g->setColor(0xff000000);
    g->fillRect(0, m_bottombar->getPos().y + 10, osu->getScreenWidth(), m_bottombar->getSize().y);

    g->setColor(0xffffffff);
    g->pushTransform();
    {
        g->scale(songSelectBottomScale, songSelectBottomScale);
        g->translate(0, (int)(m_bottombar->getPos().y) +
                            (int)((osu->getSkin()->getSongSelectBottom()->getHeight() * songSelectBottomScale) / 2) -
                            1);
        osu->getSkin()->getSongSelectBottom()->bind();
        {
            g->drawQuad(0, -(int)(m_bottombar->getSize().y * (1.0f / songSelectBottomScale) / 2),
                        (int)(osu->getScreenWidth() * (1.0f / songSelectBottomScale)),
                        (int)(m_bottombar->getSize().y * (1.0f / songSelectBottomScale)));
        }
        osu->getSkin()->getSongSelectBottom()->unbind();
    }
    g->popTransform();

    ScreenBackable::draw(g);
    m_bottombar->draw(g);
    if(cv_debug.getBool()) m_bottombar->draw_debug(g);

    // background task busy notification
    McFont *font = engine->getResourceManager()->getFont("FONT_DEFAULT");
    i32 calcx = m_userButton->getPos().x + m_userButton->getSize().x + 20;
    i32 calcy = m_userButton->getPos().y + 30;
    if(mct_total.load() > 0) {
        UString msg = UString::format("Calculating stars (%i/%i) ...", mct_computed.load(), mct_total.load());
        g->setColor(0xff333333);
        g->pushTransform();
        g->translate(calcx, calcy);
        g->drawString(font, msg);
        g->popTransform();
        calcy += font->getHeight() + 10;
    }
    if(cv_normalize_loudness.getBool() && loct_total.load() > 0 && loct_computed.load() < loct_total.load()) {
        UString msg = UString::format("Calculating loudness (%i/%i) ...", loct_computed.load(), loct_total.load());
        g->setColor(0xff333333);
        g->pushTransform();
        g->translate(calcx, calcy);
        g->drawString(font, msg);
        g->popTransform();
        calcy += font->getHeight() + 10;
    }
    if(sct_total.load() > 0 && sct_computed.load() < sct_total.load()) {
        UString msg = UString::format("Converting scores (%i/%i) ...", sct_computed.load(), sct_total.load());
        g->setColor(0xff333333);
        g->pushTransform();
        g->translate(calcx, calcy);
        g->drawString(font, msg);
        g->popTransform();
        calcy += font->getHeight() + 10;
    }

    // no beatmaps found (osu folder is probably invalid)
    if(m_beatmaps.size() == 0 && !m_bBeatmapRefreshScheduled) {
        UString errorMessage1 = "Invalid osu! folder (or no beatmaps found): ";
        errorMessage1.append(m_sLastOsuFolder);
        UString errorMessage2 = "Go to Options -> osu!folder";

        g->setColor(0xffff0000);
        g->pushTransform();
        {
            g->translate((int)(osu->getScreenWidth() / 2 - osu->getSubTitleFont()->getStringWidth(errorMessage1) / 2),
                         (int)(osu->getScreenHeight() / 2 + osu->getSubTitleFont()->getHeight()));
            g->drawString(osu->getSubTitleFont(), errorMessage1);
        }
        g->popTransform();

        g->setColor(0xff00ff00);
        g->pushTransform();
        {
            g->translate((int)(osu->getScreenWidth() / 2 - osu->getSubTitleFont()->getStringWidth(errorMessage2) / 2),
                         (int)(osu->getScreenHeight() / 2 + osu->getSubTitleFont()->getHeight() * 2 + 15));
            g->drawString(osu->getSubTitleFont(), errorMessage2);
        }
        g->popTransform();
    }

    // context menu
    m_contextMenu->draw(g);

    // click pulse animation overlay
    if(m_fPulseAnimation > 0.0f) {
        Color topColor = 0x00ffffff;
        Color bottomColor = COLOR((int)(25 * m_fPulseAnimation), 255, 255, 255);

        g->fillGradient(0, 0, osu->getScreenWidth(), osu->getScreenHeight(), topColor, topColor, bottomColor,
                        bottomColor);
    }
}

void SongBrowser::drawSelectedBeatmapBackgroundImage(Graphics *g, float alpha) {
    if(osu->getSelectedBeatmap()->getSelectedDifficulty2() != NULL) {
        Image *backgroundImage = osu->getBackgroundImageHandler()->getLoadBackgroundImage(
            osu->getSelectedBeatmap()->getSelectedDifficulty2());
        if(backgroundImage != NULL && backgroundImage->isReady()) {
            const float scale = Osu::getImageScaleToFillResolution(backgroundImage, osu->getScreenSize());

            g->setColor(0xff999999);
            g->setAlpha(alpha);
            g->pushTransform();
            {
                g->scale(scale, scale);
                g->translate(osu->getScreenWidth() / 2, osu->getScreenHeight() / 2);
                g->drawImage(backgroundImage);
            }
            g->popTransform();
        }
    }
}

bool SongBrowser::selectBeatmapset(i32 set_id) {
    auto beatmapset = db->getBeatmapSet(set_id);
    if(beatmapset == NULL) {
        // Pasted from Downloader::download_beatmap
        auto mapset_path = UString::format(MCENGINE_DATA_DIR "maps/%d/", set_id);
        db->addBeatmapSet(mapset_path.toUtf8());
        debugLog("Finished loading beatmapset %d.\n", set_id);

        beatmapset = db->getBeatmapSet(set_id);
    }

    if(beatmapset == NULL) {
        return false;
    }

    // Just picking the hardest diff for now
    DatabaseBeatmap *best_diff = NULL;
    const std::vector<DatabaseBeatmap *> &diffs = beatmapset->getDifficulties();
    for(size_t d = 0; d < diffs.size(); d++) {
        DatabaseBeatmap *diff = diffs[d];
        if(!best_diff || diff->getStarsNomod() > best_diff->getStarsNomod()) {
            best_diff = diff;
        }
    }

    if(best_diff == NULL) {
        osu->getNotificationOverlay()->addNotification("Beatmapset has no difficulties :/");
        return false;
    } else {
        onSelectionChange(hashToSongButton[best_diff->getMD5Hash()], false);
        onDifficultySelected(best_diff, false);
        selectSelectedBeatmapSongButton();
        return true;
    }
}

void SongBrowser::mouse_update(bool *propagate_clicks) {
    if(!m_bVisible) return;
    ScreenBackable::mouse_update(propagate_clicks);

    // refresh logic (blocks every other call in the update() function below it!)
    if(m_bBeatmapRefreshScheduled) {
        m_db->update();

        // check if we are finished loading
        if(m_db->isFinished()) {
            m_bBeatmapRefreshScheduled = false;
            onDatabaseLoadingFinished();
        }
        return;
    }

    // map star/bpm/other calc
    if(mct_total.load() > 0 && (mct_computed.load() == mct_total.load())) {
        mct_abort();  // join thread

        // XXX: this is mega freezing. maybe do in steps instead of all at once
        auto &maps = db->m_maps_to_recalc;
        for(int i = 0; i < mct_results.size(); i++) {
            // NOTE: not double checking md5 here. might be a mistake
            auto diff = maps[i];
            auto res = mct_results[i];
            diff->m_iNumCircles = res.nb_circles;
            diff->m_iNumSliders = res.nb_sliders;
            diff->m_iNumSpinners = res.nb_spinners;
            diff->m_fStarsNomod = res.star_rating;
            diff->m_iMinBPM = res.min_bpm;
            diff->m_iMaxBPM = res.max_bpm;
            diff->m_iMostCommonBPM = res.avg_bpm;
            diff->update_overrides();
        }

        maps.clear();
    }

    // leaderboard pp calc
    auto diff2 = m_selectedBeatmap->getSelectedDifficulty2();
    lct_set_map(diff2);
    if(diff2->m_pp_info.pp == -1.0) {
        auto mods = osu->getScore()->getMods();

        pp_calc_request request;
        request.mods_legacy = mods.to_legacy();
        request.speed = mods.speed;
        request.AR = diff2->getAR();
        request.CS = diff2->getCS();
        request.OD = diff2->getOD();
        if(mods.ar_override != -1.f) request.AR = mods.ar_override;
        if(mods.cs_override != -1.f) request.CS = mods.cs_override;
        if(mods.od_override != -1.f) request.OD = mods.od_override;
        request.rx = mods.flags & Replay::ModFlags::Relax;
        request.td = mods.flags & Replay::ModFlags::TouchDevice;
        request.comboMax = -1;
        request.numMisses = 0;
        request.num300s = diff2->getNumObjects();
        request.num100s = 0;
        request.num50s = 0;

        diff2->m_pp_info = lct_get_pp(request);
    }

    // auto-download
    if(map_autodl) {
        float progress = -1.f;
        auto beatmap = download_beatmap(map_autodl, set_autodl, &progress);
        if(progress == -1.f) {
            auto error_str = UString::format("Failed to download Beatmap #%d :(", map_autodl);
            osu->getNotificationOverlay()->addNotification(error_str);
            map_autodl = 0;
            set_autodl = 0;
        } else if(progress < 1.f) {
            // TODO @kiwec: this notification format is jank & laggy
            auto text = UString::format("Downloading... %.2f%%", progress * 100.f);
            osu->getNotificationOverlay()->addNotification(text);
        } else if(beatmap != NULL) {
            osu->m_songBrowser2->onDifficultySelected(beatmap, false);
            osu->m_songBrowser2->selectSelectedBeatmapSongButton();
            map_autodl = 0;
            set_autodl = 0;
        }
    } else if(set_autodl) {
        if(selectBeatmapset(set_autodl)) {
            map_autodl = 0;
            set_autodl = 0;
        } else {
            float progress = -1.f;
            download_beatmapset(set_autodl, &progress);
            if(progress == -1.f) {
                auto error_str = UString::format("Failed to download Beatmapset #%d :(", set_autodl);
                osu->getNotificationOverlay()->addNotification(error_str);
                map_autodl = 0;
                set_autodl = 0;
            } else if(progress < 1.f) {
                // TODO @kiwec: this notification format is jank & laggy
                auto text = UString::format("Downloading... %.2f%%", progress * 100.f);
                osu->getNotificationOverlay()->addNotification(text);
            } else {
                selectBeatmapset(set_autodl);

                map_autodl = 0;
                set_autodl = 0;
            }
        }
    }

    // update and focus handling
    m_contextMenu->mouse_update(propagate_clicks);
    m_songBrowser->mouse_update(propagate_clicks);
    m_songBrowser->getContainer()->update_pos();  // necessary due to constant animations
    m_bottombar->mouse_update(propagate_clicks);
    m_scoreBrowser->mouse_update(propagate_clicks);
    m_topbarLeft->mouse_update(propagate_clicks);
    m_topbarRight->mouse_update(propagate_clicks);

    // handle right click absolute scrolling
    {
        if(engine->getMouse()->isRightDown() && !m_contextMenu->isMouseInside()) {
            if(!m_bSongBrowserRightClickScrollCheck) {
                m_bSongBrowserRightClickScrollCheck = true;

                bool isMouseInsideAnySongButton = false;
                {
                    const std::vector<CBaseUIElement *> &elements = m_songBrowser->getContainer()->getElements();
                    for(CBaseUIElement *songButton : elements) {
                        if(songButton->isMouseInside()) {
                            isMouseInsideAnySongButton = true;
                            break;
                        }
                    }
                }

                if(m_songBrowser->isMouseInside() && !osu->getOptionsMenu()->isMouseInside() &&
                   !isMouseInsideAnySongButton)
                    m_bSongBrowserRightClickScrolling = true;
                else
                    m_bSongBrowserRightClickScrolling = false;
            }
        } else {
            m_bSongBrowserRightClickScrollCheck = false;
            m_bSongBrowserRightClickScrolling = false;
        }

        if(m_bSongBrowserRightClickScrolling)
            m_songBrowser->scrollToY(
                -((engine->getMouse()->getPos().y - 2 - m_songBrowser->getPos().y) / m_songBrowser->getSize().y) *
                m_songBrowser->getScrollSize().y);
    }

    // handle async random beatmap selection
    if(m_bRandomBeatmapScheduled) {
        m_bRandomBeatmapScheduled = false;
        selectRandomBeatmap();
    }
    if(m_bPreviousRandomBeatmapScheduled) {
        m_bPreviousRandomBeatmapScheduled = false;
        selectPreviousRandomBeatmap();
    }

    // if cursor is to the left edge of the screen, force center currently selected beatmap/diff
    // but only if the context menu is currently not visible (since we don't want move things while e.g. managing
    // collections etc.)
    if(engine->getMouse()->getPos().x < osu->getScreenWidth() * 0.1f && !m_contextMenu->isVisible()) {
        scrollToSongButton(m_selectedButton);
    }

    // handle searching
    if(m_fSearchWaitTime != 0.0f && engine->getTime() > m_fSearchWaitTime) {
        m_fSearchWaitTime = 0.0f;
        onSearchUpdate();
    }

    // handle background search matcher
    {
        if(!m_backgroundSearchMatcher->isDead() && m_backgroundSearchMatcher->isAsyncReady()) {
            // we have the results, now update the UI
            rebuildSongButtonsAndVisibleSongButtonsWithSearchMatchSupport(true);

            m_backgroundSearchMatcher->kill();
        }

        if(m_backgroundSearchMatcher->isDead()) {
            if(m_scheduled_scroll_to_selected_button) {
                m_scheduled_scroll_to_selected_button = false;

                // TODO @kiwec: This doesn't scroll if we switch between groupings
                scrollToSongButton(m_selectedButton);
            }
        }
    }
}

void SongBrowser::onKeyDown(KeyboardEvent &key) {
    OsuScreen::onKeyDown(key);  // only used for options menu
    if(!m_bVisible || key.isConsumed()) return;

    if(m_bVisible && m_bBeatmapRefreshScheduled && (key == KEY_ESCAPE || key == (KEYCODE)cv_GAME_PAUSE.getInt())) {
        m_db->cancel();
        key.consume();
        return;
    }

    if(m_bBeatmapRefreshScheduled) return;

    // context menu
    m_contextMenu->onKeyDown(key);
    if(key.isConsumed()) return;

    // searching text delete & escape key handling
    if(m_sSearchString.length() > 0) {
        switch(key.getKeyCode()) {
            case KEY_DELETE:
            case KEY_BACKSPACE:
                key.consume();
                if(m_sSearchString.length() > 0) {
                    if(engine->getKeyboard()->isControlDown()) {
                        // delete everything from the current caret position to the left, until after the first
                        // non-space character (but including it)
                        bool foundNonSpaceChar = false;
                        while(m_sSearchString.length() > 0) {
                            UString curChar = m_sSearchString.substr(m_sSearchString.length() - 1, 1);

                            if(foundNonSpaceChar && curChar.isWhitespaceOnly()) break;

                            if(!curChar.isWhitespaceOnly()) foundNonSpaceChar = true;

                            m_sSearchString.erase(m_sSearchString.length() - 1, 1);
                        }
                    } else
                        m_sSearchString = m_sSearchString.substr(0, m_sSearchString.length() - 1);

                    scheduleSearchUpdate(m_sSearchString.length() == 0);
                }
                break;

            case KEY_ESCAPE:
                key.consume();
                m_sSearchString = "";
                scheduleSearchUpdate(true);
                break;
        }
    } else if(!m_contextMenu->isVisible()) {
        if(key == KEY_ESCAPE)  // can't support GAME_PAUSE hotkey here because of text searching
            osu->toggleSongBrowser();
    }

    // paste clipboard support
    if(key == KEY_V) {
        if(engine->getKeyboard()->isControlDown()) {
            const UString clipstring = env->getClipBoardText();
            if(clipstring.length() > 0) {
                m_sSearchString.append(clipstring);
                scheduleSearchUpdate(false);
            }
        }
    }

    if(key == KEY_SHIFT) m_bShiftPressed = true;

    // function hotkeys
    if((key == KEY_F1 || key == (KEYCODE)cv_TOGGLE_MODSELECT.getInt()) && !m_bF1Pressed) {
        m_bF1Pressed = true;
        m_bottombarNavButtons[m_bottombarNavButtons.size() > 2 ? 1 : 0]->keyboardPulse();
        onSelectionMods();
    }
    if((key == KEY_F2 || key == (KEYCODE)cv_RANDOM_BEATMAP.getInt()) && !m_bF2Pressed) {
        m_bF2Pressed = true;
        m_bottombarNavButtons[m_bottombarNavButtons.size() > 2 ? 2 : 1]->keyboardPulse();
        onSelectionRandom();
    }
    if(key == KEY_F3 && !m_bF3Pressed) {
        m_bF3Pressed = true;
        m_bottombarNavButtons[m_bottombarNavButtons.size() > 3 ? 3 : 2]->keyboardPulse();
        onSelectionOptions();
    }

    if(key == KEY_F5) refreshBeatmaps();

    // selection move
    if(!engine->getKeyboard()->isAltDown() && key == KEY_DOWN) {
        const std::vector<CBaseUIElement *> &elements = m_songBrowser->getContainer()->getElements();

        // get bottom selection
        int selectedIndex = -1;
        for(int i = 0; i < elements.size(); i++) {
            Button *button = dynamic_cast<Button *>(elements[i]);
            if(button != NULL && button->isSelected()) selectedIndex = i;
        }

        // select +1
        if(selectedIndex > -1 && selectedIndex + 1 < elements.size()) {
            int nextSelectionIndex = selectedIndex + 1;
            Button *nextButton = dynamic_cast<Button *>(elements[nextSelectionIndex]);
            SongButton *songButton = dynamic_cast<SongButton *>(elements[nextSelectionIndex]);
            if(nextButton != NULL) {
                nextButton->select(true, false);

                // if this is a song button, select top child
                if(songButton != NULL) {
                    const auto &children = songButton->getChildren();
                    if(children.size() > 0 && !children[0]->isSelected()) children[0]->select(true, false, false);
                }
            }
        }
    }

    if(!engine->getKeyboard()->isAltDown() && key == KEY_UP) {
        const std::vector<CBaseUIElement *> &elements = m_songBrowser->getContainer()->getElements();

        // get bottom selection
        int selectedIndex = -1;
        for(int i = 0; i < elements.size(); i++) {
            Button *button = dynamic_cast<Button *>(elements[i]);
            if(button != NULL && button->isSelected()) selectedIndex = i;
        }

        // select -1
        if(selectedIndex > -1 && selectedIndex - 1 > -1) {
            int nextSelectionIndex = selectedIndex - 1;
            Button *nextButton = dynamic_cast<Button *>(elements[nextSelectionIndex]);
            bool isCollectionButton = dynamic_cast<CollectionButton *>(elements[nextSelectionIndex]);

            if(nextButton != NULL) {
                nextButton->select();

                // automatically open collection on top of this one and go to bottom child
                if(isCollectionButton && nextSelectionIndex - 1 > -1) {
                    nextSelectionIndex = nextSelectionIndex - 1;
                    CollectionButton *nextCollectionButton =
                        dynamic_cast<CollectionButton *>(elements[nextSelectionIndex]);
                    if(nextCollectionButton != NULL) {
                        nextCollectionButton->select();

                        const auto &children = nextCollectionButton->getChildren();
                        if(children.size() > 0 && !children[children.size() - 1]->isSelected())
                            children[children.size() - 1]->select();
                    }
                }
            }
        }
    }

    if(key == KEY_LEFT && !m_bLeft) {
        m_bLeft = true;

        const bool jumpToNextGroup = engine->getKeyboard()->isShiftDown();

        const std::vector<CBaseUIElement *> &elements = m_songBrowser->getContainer()->getElements();

        bool foundSelected = false;
        for(int i = elements.size() - 1; i >= 0; i--) {
            const SongDifficultyButton *diffButtonPointer = dynamic_cast<SongDifficultyButton *>(elements[i]);
            const CollectionButton *collectionButtonPointer = dynamic_cast<CollectionButton *>(elements[i]);

            Button *button = dynamic_cast<Button *>(elements[i]);
            const bool isSongDifficultyButtonAndNotIndependent =
                (diffButtonPointer != NULL && !diffButtonPointer->isIndependentDiffButton());

            if(foundSelected && button != NULL && !button->isSelected() && !isSongDifficultyButtonAndNotIndependent &&
               (!jumpToNextGroup || collectionButtonPointer != NULL)) {
                m_bNextScrollToSongButtonJumpFixUseScrollSizeDelta = true;
                {
                    button->select();

                    if(!jumpToNextGroup || collectionButtonPointer == NULL) {
                        // automatically open collection below and go to bottom child
                        CollectionButton *collectionButton = dynamic_cast<CollectionButton *>(elements[i]);
                        if(collectionButton != NULL) {
                            const auto &children = collectionButton->getChildren();
                            if(children.size() > 0 && !children[children.size() - 1]->isSelected())
                                children[children.size() - 1]->select();
                        }
                    }
                }
                m_bNextScrollToSongButtonJumpFixUseScrollSizeDelta = false;

                break;
            }

            if(button != NULL && button->isSelected()) foundSelected = true;
        }
    }

    if(key == KEY_RIGHT && !m_bRight) {
        m_bRight = true;

        const bool jumpToNextGroup = engine->getKeyboard()->isShiftDown();

        const std::vector<CBaseUIElement *> &elements = m_songBrowser->getContainer()->getElements();

        // get bottom selection
        int selectedIndex = -1;
        for(size_t i = 0; i < elements.size(); i++) {
            Button *button = dynamic_cast<Button *>(elements[i]);
            if(button != NULL && button->isSelected()) selectedIndex = i;
        }

        if(selectedIndex > -1) {
            for(size_t i = selectedIndex; i < elements.size(); i++) {
                const SongDifficultyButton *diffButtonPointer = dynamic_cast<SongDifficultyButton *>(elements[i]);
                const CollectionButton *collectionButtonPointer = dynamic_cast<CollectionButton *>(elements[i]);

                Button *button = dynamic_cast<Button *>(elements[i]);
                const bool isSongDifficultyButtonAndNotIndependent =
                    (diffButtonPointer != NULL && !diffButtonPointer->isIndependentDiffButton());

                if(button != NULL && !button->isSelected() && !isSongDifficultyButtonAndNotIndependent &&
                   (!jumpToNextGroup || collectionButtonPointer != NULL)) {
                    button->select();
                    break;
                }
            }
        }
    }

    if(key == KEY_PAGEUP) m_songBrowser->scrollY(m_songBrowser->getSize().y);
    if(key == KEY_PAGEDOWN) m_songBrowser->scrollY(-m_songBrowser->getSize().y);

    // group open/close
    // NOTE: only closing works atm (no "focus" state on buttons yet)
    if(key == KEY_ENTER && engine->getKeyboard()->isShiftDown()) {
        const std::vector<CBaseUIElement *> &elements = m_songBrowser->getContainer()->getElements();

        for(int i = 0; i < elements.size(); i++) {
            const CollectionButton *collectionButtonPointer = dynamic_cast<CollectionButton *>(elements[i]);

            Button *button = dynamic_cast<Button *>(elements[i]);

            if(collectionButtonPointer != NULL && button != NULL && button->isSelected()) {
                button->select();  // deselect

                scrollToSongButton(button);

                break;
            }
        }
    }

    // selection select
    if(key == KEY_ENTER && !engine->getKeyboard()->isShiftDown()) playSelectedDifficulty();

    // toggle auto
    if(key == KEY_A && engine->getKeyboard()->isControlDown()) osu->getModSelector()->toggleAuto();

    key.consume();
}

void SongBrowser::onKeyUp(KeyboardEvent &key) {
    // context menu
    m_contextMenu->onKeyUp(key);
    if(key.isConsumed()) return;

    if(key == KEY_SHIFT) m_bShiftPressed = false;
    if(key == KEY_LEFT) m_bLeft = false;
    if(key == KEY_RIGHT) m_bRight = false;

    if(key == KEY_F1 || key == (KEYCODE)cv_TOGGLE_MODSELECT.getInt()) m_bF1Pressed = false;
    if(key == KEY_F2 || key == (KEYCODE)cv_RANDOM_BEATMAP.getInt()) m_bF2Pressed = false;
    if(key == KEY_F3) m_bF3Pressed = false;
}

void SongBrowser::onChar(KeyboardEvent &e) {
    // context menu
    m_contextMenu->onChar(e);
    if(e.isConsumed()) return;

    if(e.getCharCode() < 32 || !m_bVisible || m_bBeatmapRefreshScheduled ||
       (engine->getKeyboard()->isControlDown() && !engine->getKeyboard()->isAltDown()))
        return;
    if(m_bF1Pressed || m_bF2Pressed || m_bF3Pressed) return;

    // handle searching
    KEYCODE charCode = e.getCharCode();
    UString stringChar = "";
    stringChar.insert(0, charCode);
    m_sSearchString.append(stringChar);

    scheduleSearchUpdate();
}

void SongBrowser::onResolutionChange(Vector2 newResolution) { ScreenBackable::onResolutionChange(newResolution); }

CBaseUIContainer *SongBrowser::setVisible(bool visible) {
    if(visible == m_bVisible) return this;

    m_bVisible = visible;
    m_bShiftPressed = false;  // seems to get stuck sometimes otherwise

    if(m_bVisible) {
        engine->getSound()->play(osu->getSkin()->m_expand);
        RichPresence::onSongBrowser();

        updateLayout();

        // we have to re-select the current beatmap to start playing music again
        if(m_selectedBeatmap != NULL) m_selectedBeatmap->select();

        m_bHasSelectedAndIsPlaying = false;  // sanity

        // try another refresh, maybe the osu!folder has changed
        if(m_beatmaps.size() == 0) {
            refreshBeatmaps();
        }

        // update user name/stats
        onUserCardChange(cv_name.getString());

        // HACKHACK: workaround for BaseUI framework deficiency (missing mouse events. if a mouse button is being held,
        // and then suddenly a BaseUIElement gets put under it and set visible, and then the mouse button is released,
        // that "incorrectly" fires onMouseUpInside/onClicked/etc.)
        engine->getMouse()->onLeftChange(false);
        engine->getMouse()->onRightChange(false);

        if(m_selectedBeatmap != NULL) {
            // For multiplayer: if the host exits song selection without selecting a song, we want to be able to revert
            // to that previous song.
            m_lastSelectedBeatmap = m_selectedBeatmap->getSelectedDifficulty2();

            // Select button matching current song preview
            selectSelectedBeatmapSongButton();
        }
    } else {
        m_contextMenu->setVisible2(false);
    }

    osu->m_chat->updateVisibility();
    return this;
}

void SongBrowser::selectSelectedBeatmapSongButton() {
    if(m_selectedBeatmap == NULL) return;

    auto diff = m_selectedBeatmap->getSelectedDifficulty2();
    if(diff == NULL) return;

    auto it = hashToSongButton.find(diff->getMD5Hash());
    if(it == hashToSongButton.end()) {
        debugLog("No song button found for currently selected beatmap...\n");
        return;
    }

    it->second->deselect();  // if we select() it when already selected, it would start playing!
    it->second->select();
}

void SongBrowser::onPlayEnd(bool quit) {
    m_bHasSelectedAndIsPlaying = false;

    // update score displays
    if(!quit) {
        rebuildScoreButtons();

        SongDifficultyButton *selectedSongDiffButton = dynamic_cast<SongDifficultyButton *>(m_selectedButton);
        if(selectedSongDiffButton != NULL) selectedSongDiffButton->updateGrade();
    }

    // update song info
    if(m_selectedBeatmap != NULL && m_selectedBeatmap->getSelectedDifficulty2() != NULL)
        m_songInfo->setFromBeatmap(m_selectedBeatmap, m_selectedBeatmap->getSelectedDifficulty2());
}

void SongBrowser::onSelectionChange(Button *button, bool rebuild) {
    m_selectedButton = button;
    if(button == NULL) return;

    m_contextMenu->setVisible2(false);

    // keep track and update all selection states
    // I'm still not happy with this, but at least all state update logic is localized in this function instead of
    // spread across all buttons

    SongButton *songButtonPointer = dynamic_cast<SongButton *>(button);
    SongDifficultyButton *songDiffButtonPointer = dynamic_cast<SongDifficultyButton *>(button);
    CollectionButton *collectionButtonPointer = dynamic_cast<CollectionButton *>(button);

    if(songDiffButtonPointer != NULL) {
        if(m_selectionPreviousSongDiffButton != NULL && m_selectionPreviousSongDiffButton != songDiffButtonPointer)
            m_selectionPreviousSongDiffButton->deselect();

        // support individual diffs independent from their parent song button container
        {
            // if the new diff has a parent song button, then update its selection state (select it to stay consistent)
            if(songDiffButtonPointer->getParentSongButton() != NULL &&
               !songDiffButtonPointer->getParentSongButton()->isSelected()) {
                songDiffButtonPointer->getParentSongButton()
                    ->sortChildren();  // NOTE: workaround for disabled callback firing in select()
                songDiffButtonPointer->getParentSongButton()->select(false);
                onSelectionChange(songDiffButtonPointer->getParentSongButton(), false);  // NOTE: recursive call
            }

            // if the new diff does not have a parent song button, but the previous diff had, then update the previous
            // diff parent song button selection state (to deselect it)
            if(songDiffButtonPointer->getParentSongButton() == NULL) {
                if(m_selectionPreviousSongDiffButton != NULL &&
                   m_selectionPreviousSongDiffButton->getParentSongButton() != NULL)
                    m_selectionPreviousSongDiffButton->getParentSongButton()->deselect();
            }
        }

        m_selectionPreviousSongDiffButton = songDiffButtonPointer;
    } else if(songButtonPointer != NULL) {
        if(m_selectionPreviousSongButton != NULL && m_selectionPreviousSongButton != songButtonPointer)
            m_selectionPreviousSongButton->deselect();
        if(m_selectionPreviousSongDiffButton != NULL) m_selectionPreviousSongDiffButton->deselect();

        m_selectionPreviousSongButton = songButtonPointer;
    } else if(collectionButtonPointer != NULL) {
        // TODO: maybe expand this logic with per-group-type last-open-collection memory

        // logic for allowing collections to be deselected by clicking on the same button (contrary to how beatmaps
        // work)
        const bool isTogglingCollection = (m_selectionPreviousCollectionButton != NULL &&
                                           m_selectionPreviousCollectionButton == collectionButtonPointer);

        if(m_selectionPreviousCollectionButton != NULL) m_selectionPreviousCollectionButton->deselect();

        m_selectionPreviousCollectionButton = collectionButtonPointer;

        if(isTogglingCollection) m_selectionPreviousCollectionButton = NULL;
    }

    if(rebuild) rebuildSongButtons();
}

void SongBrowser::onDifficultySelected(DatabaseBeatmap *diff2, bool play) {
    // deselect = unload
    auto prev_diff2 = m_selectedBeatmap->getSelectedDifficulty2();
    m_selectedBeatmap->deselect();
    if(diff2 != prev_diff2 && !diff2->do_not_store) {
        m_previousRandomBeatmaps.push_back(diff2);
    }

    // select = play preview music
    m_selectedBeatmap->selectDifficulty2(diff2);

    // update song info
    m_songInfo->setFromBeatmap(m_selectedBeatmap, diff2);

    // start playing
    if(play) {
        if(bancho.is_in_a_multi_room()) {
            bancho.room.map_name = UString::format("%s - %s [%s]", diff2->getArtist().c_str(),
                                                   diff2->getTitle().c_str(), diff2->getDifficultyName().c_str());
            bancho.room.map_md5 = diff2->getMD5Hash();
            bancho.room.map_id = diff2->getID();

            Packet packet;
            packet.id = MATCH_CHANGE_SETTINGS;
            bancho.room.pack(&packet);
            send_packet(packet);

            osu->m_room->on_map_change();

            setVisible(false);
        } else {
            // CTRL + click = auto
            if(engine->getKeyboard()->isControlDown()) {
                osu->m_bModAutoTemp = true;
                osu->getModSelector()->enableAuto();
            }

            if(m_selectedBeatmap->play()) {
                m_bHasSelectedAndIsPlaying = true;
                setVisible(false);
            }
        }
    }

    // animate
    m_fPulseAnimation = 1.0f;
    anim->moveLinear(&m_fPulseAnimation, 0.0f, 0.55f, true);

    // update score display
    rebuildScoreButtons();

    // update web button
    m_webButton->setVisible(m_songInfo->getBeatmapID() > 0);

    // trigger dynamic star calc (including current mods etc.)
    recalculateStarsForSelectedBeatmap();
}

void SongBrowser::refreshBeatmaps() {
    if(!m_bVisible || m_bHasSelectedAndIsPlaying) return;

    // reset
    checkHandleKillBackgroundSearchMatcher();

    // don't pause the music the first time we load the song database
    static bool first_refresh = true;
    if(first_refresh) {
        m_selectedBeatmap->m_music = NULL;
        first_refresh = false;
    }

    auto diff2 = m_selectedBeatmap->getSelectedDifficulty2();
    if(diff2) {
        beatmap_to_reselect_after_db_load = diff2->getMD5Hash();
    }

    m_selectedBeatmap->pausePreviewMusic();
    m_selectedBeatmap->deselect();
    SAFE_DELETE(m_selectedBeatmap);
    m_selectedBeatmap = new Beatmap();

    m_selectionPreviousSongButton = NULL;
    m_selectionPreviousSongDiffButton = NULL;
    m_selectionPreviousCollectionButton = NULL;

    // delete local database and UI
    m_songBrowser->getContainer()->empty();

    for(size_t i = 0; i < m_songButtons.size(); i++) {
        delete m_songButtons[i];
    }
    m_songButtons.clear();
    hashToSongButton.clear();
    for(size_t i = 0; i < m_collectionButtons.size(); i++) {
        delete m_collectionButtons[i];
    }
    m_collectionButtons.clear();
    for(size_t i = 0; i < m_artistCollectionButtons.size(); i++) {
        delete m_artistCollectionButtons[i];
    }
    m_artistCollectionButtons.clear();
    for(size_t i = 0; i < m_bpmCollectionButtons.size(); i++) {
        delete m_bpmCollectionButtons[i];
    }
    m_bpmCollectionButtons.clear();
    for(size_t i = 0; i < m_difficultyCollectionButtons.size(); i++) {
        delete m_difficultyCollectionButtons[i];
    }
    m_difficultyCollectionButtons.clear();
    for(size_t i = 0; i < m_creatorCollectionButtons.size(); i++) {
        delete m_creatorCollectionButtons[i];
    }
    m_creatorCollectionButtons.clear();
    for(size_t i = 0; i < m_dateaddedCollectionButtons.size(); i++) {
        delete m_dateaddedCollectionButtons[i];
    }
    m_dateaddedCollectionButtons.clear();
    for(size_t i = 0; i < m_lengthCollectionButtons.size(); i++) {
        delete m_lengthCollectionButtons[i];
    }
    m_lengthCollectionButtons.clear();
    for(size_t i = 0; i < m_titleCollectionButtons.size(); i++) {
        delete m_titleCollectionButtons[i];
    }
    m_titleCollectionButtons.clear();

    m_visibleSongButtons.clear();
    m_beatmaps.clear();
    m_previousRandomBeatmaps.clear();

    m_contextMenu->setVisible2(false);

    // clear potentially active search
    m_bInSearch = false;
    m_sSearchString = "";
    m_sPrevSearchString = "";
    m_fSearchWaitTime = 0.0f;
    m_searchPrevGroup = GROUP::GROUP_NO_GROUPING;

    // force no grouping
    if(m_group != GROUP::GROUP_NO_GROUPING) onGroupChange("", 0);

    // start loading
    m_bBeatmapRefreshScheduled = true;
    m_db->load();
}

void SongBrowser::addBeatmapSet(BeatmapSet *mapset) {
    if(mapset->getDifficulties().size() < 1) return;

    SongButton *songButton;
    if(mapset->getDifficulties().size() > 1) {
        songButton =
            new SongButton(this, m_songBrowser, m_contextMenu, 250, 250 + m_beatmaps.size() * 50, 200, 50, "", mapset);
    } else {
        songButton = new SongDifficultyButton(this, m_songBrowser, m_contextMenu, 250, 250 + m_beatmaps.size() * 50,
                                              200, 50, "", mapset->getDifficulties()[0], NULL);
    }

    m_songButtons.push_back(songButton);
    for(auto diff : mapset->getDifficulties()) {
        hashToSongButton[diff->getMD5Hash()] = songButton;
    }

    // prebuild temporary list of all relevant buttons, used by some groups
    std::vector<SongButton *> tempChildrenForGroups;
    {
        if(songButton->getChildren().size() > 0) {
            for(SongButton *child : songButton->getChildren()) {
                tempChildrenForGroups.push_back(child);
            }
        } else {
            tempChildrenForGroups.push_back(songButton);
        }
    }

    // add mapset to all necessary groups
    {
        addSongButtonToAlphanumericGroup(songButton, m_artistCollectionButtons, mapset->getArtist());
        addSongButtonToAlphanumericGroup(songButton, m_creatorCollectionButtons, mapset->getCreator());
        addSongButtonToAlphanumericGroup(songButton, m_titleCollectionButtons, mapset->getTitle());

        // difficulty
        if(m_difficultyCollectionButtons.size() == 12) {
            for(SongButton *diff_btn : tempChildrenForGroups) {
                const int index = clamp<int>((int)diff_btn->getDatabaseBeatmap()->getStarsNomod(), 0, 11);
                auto &children = m_difficultyCollectionButtons[index]->getChildren();
                auto it = std::lower_bound(children.begin(), children.end(), diff_btn, sort_by_difficulty);
                children.insert(it, diff_btn);
            }
        }

        // bpm
        if(m_bpmCollectionButtons.size() == 6) {
            for(SongButton *diff_btn : tempChildrenForGroups) {
                auto bpm = diff_btn->getDatabaseBeatmap()->getMostCommonBPM();
                int index;
                if(bpm < 60) {
                    index = 0;
                } else if(bpm < 120) {
                    index = 1;
                } else if(bpm < 180) {
                    index = 2;
                } else if(bpm < 240) {
                    index = 3;
                } else if(bpm < 300) {
                    index = 4;
                } else {
                    index = 5;
                }
                auto &children = m_bpmCollectionButtons[index]->getChildren();
                auto it = std::lower_bound(children.begin(), children.end(), diff_btn, sort_by_bpm);
                children.insert(it, diff_btn);
            }
        }

        // dateadded
        {
            // TODO: extremely annoying
        }

        // length
        if(m_lengthCollectionButtons.size() == 7) {
            for(auto diff_btn : tempChildrenForGroups) {
                const u32 lengthMS = diff_btn->getDatabaseBeatmap()->getLengthMS();

                std::vector<SongButton *> *children = NULL;
                if(lengthMS <= 1000 * 60) {
                    children = &m_lengthCollectionButtons[0]->getChildren();
                } else if(lengthMS <= 1000 * 60 * 2) {
                    children = &m_lengthCollectionButtons[1]->getChildren();
                } else if(lengthMS <= 1000 * 60 * 3) {
                    children = &m_lengthCollectionButtons[2]->getChildren();
                } else if(lengthMS <= 1000 * 60 * 4) {
                    children = &m_lengthCollectionButtons[3]->getChildren();
                } else if(lengthMS <= 1000 * 60 * 5) {
                    children = &m_lengthCollectionButtons[4]->getChildren();
                } else if(lengthMS <= 1000 * 60 * 10) {
                    children = &m_lengthCollectionButtons[5]->getChildren();
                } else {
                    children = &m_lengthCollectionButtons[6]->getChildren();
                }

                auto it = std::lower_bound(children->begin(), children->end(), songButton, sort_by_length);
                children->insert(it, diff_btn);
            }
        }
    }
}

void SongBrowser::addSongButtonToAlphanumericGroup(SongButton *btn, std::vector<CollectionButton *> &group,
                                                   const std::string &name) {
    if(group.size() != 28) {
        debugLog("Alphanumeric group wasn't initialized!\n");
        return;
    }

    const char firstChar = name.length() == 0 ? '#' : name[0];
    const bool isNumber = (firstChar >= '0' && firstChar <= '9');
    const bool isLowerCase = (firstChar >= 'a' && firstChar <= 'z');
    const bool isUpperCase = (firstChar >= 'A' && firstChar <= 'Z');

    std::vector<SongButton *> *children = NULL;
    if(isNumber) {
        children = &group[0]->getChildren();
    } else if(isLowerCase || isUpperCase) {
        const int index = 1 + (25 - (isLowerCase ? 'z' - firstChar : 'Z' - firstChar));
        children = &group[index]->getChildren();
    } else {
        children = &group[27]->getChildren();
    }

    auto it = std::lower_bound(children->begin(), children->end(), btn, m_sortingComparator);
    if(cv_debug.getBool()) debugLog("Inserting %s at index %d\n", name.c_str(), it - children->begin());
    children->insert(it, btn);
}

void SongBrowser::requestNextScrollToSongButtonJumpFix(SongDifficultyButton *diffButton) {
    if(diffButton == NULL) return;

    m_bNextScrollToSongButtonJumpFixScheduled = true;
    m_fNextScrollToSongButtonJumpFixOldRelPosY =
        (diffButton->getParentSongButton() != NULL ? diffButton->getParentSongButton()->getRelPos().y
                                                   : diffButton->getRelPos().y);
    m_fNextScrollToSongButtonJumpFixOldScrollSizeY = m_songBrowser->getScrollSize().y;
}

void SongBrowser::scrollToSongButton(Button *songButton, bool alignOnTop) {
    if(songButton == NULL) {
        debugLog("scrollToSongButton(): songButton == NULL\n");
        m_songBrowser->scrollToTop();
        return;
    }

    // NOTE: compensate potential scroll jump due to added/removed elements (feels a lot better this way, also easier on
    // the eyes)
    if(m_bNextScrollToSongButtonJumpFixScheduled) {
        m_bNextScrollToSongButtonJumpFixScheduled = false;

        float delta = 0.0f;
        {
            if(!m_bNextScrollToSongButtonJumpFixUseScrollSizeDelta)
                delta = (songButton->getRelPos().y - m_fNextScrollToSongButtonJumpFixOldRelPosY);  // (default case)
            else
                delta = m_songBrowser->getScrollSize().y -
                        m_fNextScrollToSongButtonJumpFixOldScrollSizeY;  // technically not correct but feels a lot
                                                                         // better for KEY_LEFT navigation
        }
        m_songBrowser->scrollToY(m_songBrowser->getRelPosY() - delta, false);
    }

    m_songBrowser->scrollToY(-songButton->getRelPos().y +
                             (alignOnTop ? (0) : (m_songBrowser->getSize().y / 2 - songButton->getSize().y / 2)));
}

void SongBrowser::rebuildSongButtons() {
    m_songBrowser->getContainer()->empty();

    // NOTE: currently supports 3 depth layers (collection > beatmap > diffs)
    for(size_t i = 0; i < m_visibleSongButtons.size(); i++) {
        Button *button = m_visibleSongButtons[i];
        button->resetAnimations();

        if(!(button->isSelected() && button->isHiddenIfSelected()))
            m_songBrowser->getContainer()->addBaseUIElement(button);

        // children
        if(button->isSelected()) {
            const auto &children = m_visibleSongButtons[i]->getChildren();
            for(size_t c = 0; c < children.size(); c++) {
                Button *button2 = children[c];

                bool isButton2SearchMatch = false;
                if(button2->getChildren().size() > 0) {
                    const auto &children2 = button2->getChildren();
                    for(size_t c2 = 0; c2 < children2.size(); c2++) {
                        const Button *button3 = children2[c2];
                        if(button3->isSearchMatch()) {
                            isButton2SearchMatch = true;
                            break;
                        }
                    }
                } else
                    isButton2SearchMatch = button2->isSearchMatch();

                if(m_bInSearch && !isButton2SearchMatch) continue;

                button2->resetAnimations();

                if(!(button2->isSelected() && button2->isHiddenIfSelected()))
                    m_songBrowser->getContainer()->addBaseUIElement(button2);

                // child children
                if(button2->isSelected()) {
                    const auto &children2 = button2->getChildren();
                    for(size_t c2 = 0; c2 < children2.size(); c2++) {
                        Button *button3 = children2[c2];

                        if(m_bInSearch && !button3->isSearchMatch()) continue;

                        button3->resetAnimations();

                        if(!(button3->isSelected() && button3->isHiddenIfSelected()))
                            m_songBrowser->getContainer()->addBaseUIElement(button3);
                    }
                }
            }
        }
    }

    // TODO: regroup diffs which are next to each other into one song button (parent button)
    // TODO: regrouping is non-deterministic, depending on the searching method used.
    //       meaning that any number of "clusters" of diffs belonging to the same beatmap could build, requiring
    //       multiple song "parent" buttons for the same beatmap (if touching group size >= 2)
    //       when regrouping, these "fake" parent buttons have to be deleted on every reload. this means that the
    //       selection state logic has to be kept cleared of any invalid pointers!
    //       (including everything else which would rely on having a permanent pointer to an SongButton)

    updateSongButtonLayout();
}

void SongBrowser::updateSongButtonLayout() {
    // this rebuilds the entire songButton layout (songButtons in relation to others)
    // only the y axis is set, because the x axis is constantly animated and handled within the button classes
    // themselves
    const std::vector<CBaseUIElement *> &elements = m_songBrowser->getContainer()->getElements();

    int yCounter = m_songBrowser->getSize().y / 4;
    if(elements.size() <= 1) yCounter = m_songBrowser->getSize().y / 2;

    bool isSelected = false;
    bool inOpenCollection = false;
    for(size_t i = 0; i < elements.size(); i++) {
        Button *songButton = dynamic_cast<Button *>(elements[i]);

        if(songButton != NULL) {
            const SongDifficultyButton *diffButtonPointer = dynamic_cast<SongDifficultyButton *>(songButton);

            // depending on the object type, layout differently
            const bool isCollectionButton = dynamic_cast<CollectionButton *>(songButton) != NULL;
            const bool isDiffButton = diffButtonPointer != NULL;
            const bool isIndependentDiffButton = isDiffButton && diffButtonPointer->isIndependentDiffButton();

            // give selected items & diffs a bit more spacing, to make them stand out
            if(((songButton->isSelected() && !isCollectionButton) || isSelected ||
                (isDiffButton && !isIndependentDiffButton)))
                yCounter += songButton->getSize().y * 0.1f;

            isSelected = songButton->isSelected() || (isDiffButton && !isIndependentDiffButton);

            // give collections a bit more spacing at start & end
            if((songButton->isSelected() && isCollectionButton)) yCounter += songButton->getSize().y * 0.2f;
            if(inOpenCollection && isCollectionButton && !songButton->isSelected())
                yCounter += songButton->getSize().y * 0.2f;
            if(isCollectionButton) {
                if(songButton->isSelected())
                    inOpenCollection = true;
                else
                    inOpenCollection = false;
            }

            songButton->setTargetRelPosY(yCounter);
            songButton->updateLayoutEx();

            yCounter += songButton->getActualSize().y;
        }
    }
    m_songBrowser->setScrollSizeToContent(m_songBrowser->getSize().y / 2);
}

bool SongBrowser::searchMatcher(const DatabaseBeatmap *databaseBeatmap,
                                const std::vector<UString> &searchStringTokens) {
    if(databaseBeatmap == NULL) return false;

    const std::vector<DatabaseBeatmap *> &diffs = databaseBeatmap->getDifficulties();
    const bool isContainer = (diffs.size() > 0);
    const int numDiffs = (isContainer ? diffs.size() : 1);

    // TODO: optimize this dumpster fire. can at least cache the parsed tokens and literal strings array instead of
    // parsing every single damn time

    // intelligent search parser
    // all strings which are not expressions get appended with spaces between, then checked with one call to
    // findSubstringInDifficulty() the rest is interpreted NOTE: this code is quite shitty. the order of the operators
    // array does matter, because find() is used to detect their presence (and '=' would then break '<=' etc.)
    enum operatorId { EQ, LT, GT, LE, GE, NE };
    static const std::vector<std::pair<UString, operatorId>> operators = {
        std::pair<UString, operatorId>("<=", LE), std::pair<UString, operatorId>(">=", GE),
        std::pair<UString, operatorId>("<", LT),  std::pair<UString, operatorId>(">", GT),
        std::pair<UString, operatorId>("!=", NE), std::pair<UString, operatorId>("==", EQ),
        std::pair<UString, operatorId>("=", EQ),
    };

    enum keywordId {
        AR,
        CS,
        OD,
        HP,
        BPM,
        OPM,
        CPM,
        SPM,
        OBJECTS,
        CIRCLES,
        SLIDERS,
        SPINNERS,
        LENGTH,
        STARS,
    };
    static const std::vector<std::pair<UString, keywordId>> keywords = {
        std::pair<UString, keywordId>("ar", AR),
        std::pair<UString, keywordId>("cs", CS),
        std::pair<UString, keywordId>("od", OD),
        std::pair<UString, keywordId>("hp", HP),
        std::pair<UString, keywordId>("bpm", BPM),
        std::pair<UString, keywordId>("opm", OPM),
        std::pair<UString, keywordId>("cpm", CPM),
        std::pair<UString, keywordId>("spm", SPM),
        std::pair<UString, keywordId>("object", OBJECTS),
        std::pair<UString, keywordId>("objects", OBJECTS),
        std::pair<UString, keywordId>("circle", CIRCLES),
        std::pair<UString, keywordId>("circles", CIRCLES),
        std::pair<UString, keywordId>("slider", SLIDERS),
        std::pair<UString, keywordId>("sliders", SLIDERS),
        std::pair<UString, keywordId>("spinner", SPINNERS),
        std::pair<UString, keywordId>("spinners", SPINNERS),
        std::pair<UString, keywordId>("length", LENGTH),
        std::pair<UString, keywordId>("len", LENGTH),
        std::pair<UString, keywordId>("stars", STARS),
        std::pair<UString, keywordId>("star", STARS)};

    // split search string into tokens
    // parse over all difficulties
    bool expressionMatches = false;  // if any diff matched all expressions
    std::vector<UString> literalSearchStrings;
    for(size_t d = 0; d < numDiffs; d++) {
        const DatabaseBeatmap *diff = (isContainer ? diffs[d] : databaseBeatmap);

        bool expressionsMatch = true;  // if the current search string (meaning only the expressions in this case)
                                       // matches the current difficulty

        for(size_t i = 0; i < searchStringTokens.size(); i++) {
            // debugLog("token[%i] = %s\n", i, tokens[i].toUtf8());
            //  determine token type, interpret expression
            bool expression = false;
            for(size_t o = 0; o < operators.size(); o++) {
                if(searchStringTokens[i].find(operators[o].first) != -1) {
                    // split expression into left and right parts (only accept singular expressions, things like
                    // "0<bpm<1" will not work with this)
                    // debugLog("splitting by string %s\n", operators[o].first.toUtf8());
                    std::vector<UString> values = searchStringTokens[i].split(operators[o].first);
                    if(values.size() == 2 && values[0].length() > 0 && values[1].length() > 0) {
                        const UString lvalue = values[0];
                        const int rvaluePercentIndex = values[1].find("%");
                        const bool rvalueIsPercent = (rvaluePercentIndex != -1);
                        const float rvalue =
                            (rvaluePercentIndex == -1
                                 ? values[1].toFloat()
                                 : values[1]
                                       .substr(0, rvaluePercentIndex)
                                       .toFloat());  // this must always be a number (at least, assume it is)

                        // find lvalue keyword in array (only continue if keyword exists)
                        for(size_t k = 0; k < keywords.size(); k++) {
                            if(keywords[k].first == lvalue) {
                                expression = true;

                                // we now have a valid expression: the keyword, the operator and the value

                                // solve keyword
                                float compareValue = 5.0f;
                                switch(keywords[k].second) {
                                    case AR:
                                        compareValue = diff->getAR();
                                        break;
                                    case CS:
                                        compareValue = diff->getCS();
                                        break;
                                    case OD:
                                        compareValue = diff->getOD();
                                        break;
                                    case HP:
                                        compareValue = diff->getHP();
                                        break;
                                    case BPM:
                                        compareValue = diff->getMostCommonBPM();
                                        break;
                                    case OPM:
                                        compareValue =
                                            (diff->getLengthMS() > 0 ? ((float)diff->getNumObjects() /
                                                                        (float)(diff->getLengthMS() / 1000.0f / 60.0f))
                                                                     : 0.0f) *
                                            osu->getSpeedMultiplier();
                                        break;
                                    case CPM:
                                        compareValue =
                                            (diff->getLengthMS() > 0 ? ((float)diff->getNumCircles() /
                                                                        (float)(diff->getLengthMS() / 1000.0f / 60.0f))
                                                                     : 0.0f) *
                                            osu->getSpeedMultiplier();
                                        break;
                                    case SPM:
                                        compareValue =
                                            (diff->getLengthMS() > 0 ? ((float)diff->getNumSliders() /
                                                                        (float)(diff->getLengthMS() / 1000.0f / 60.0f))
                                                                     : 0.0f) *
                                            osu->getSpeedMultiplier();
                                        break;
                                    case OBJECTS:
                                        compareValue = diff->getNumObjects();
                                        break;
                                    case CIRCLES:
                                        compareValue =
                                            (rvalueIsPercent
                                                 ? ((float)diff->getNumCircles() / (float)diff->getNumObjects()) *
                                                       100.0f
                                                 : diff->getNumCircles());
                                        break;
                                    case SLIDERS:
                                        compareValue =
                                            (rvalueIsPercent
                                                 ? ((float)diff->getNumSliders() / (float)diff->getNumObjects()) *
                                                       100.0f
                                                 : diff->getNumSliders());
                                        break;
                                    case SPINNERS:
                                        compareValue =
                                            (rvalueIsPercent
                                                 ? ((float)diff->getNumSpinners() / (float)diff->getNumObjects()) *
                                                       100.0f
                                                 : diff->getNumSpinners());
                                        break;
                                    case LENGTH:
                                        compareValue = diff->getLengthMS() / 1000;
                                        break;
                                    case STARS:
                                        compareValue = std::round(diff->getStarsNomod() * 100.0f) /
                                                       100.0f;  // round to 2 decimal places
                                        break;
                                }

                                // solve operator
                                bool matches = false;
                                switch(operators[o].second) {
                                    case LE:
                                        if(compareValue <= rvalue) matches = true;
                                        break;
                                    case GE:
                                        if(compareValue >= rvalue) matches = true;
                                        break;
                                    case LT:
                                        if(compareValue < rvalue) matches = true;
                                        break;
                                    case GT:
                                        if(compareValue > rvalue) matches = true;
                                        break;
                                    case NE:
                                        if(compareValue != rvalue) matches = true;
                                        break;
                                    case EQ:
                                        if(compareValue == rvalue) matches = true;
                                        break;
                                }

                                // debugLog("comparing %f %s %f (operatorId = %i) = %i\n", compareValue,
                                // operators[o].first.toUtf8(), rvalue, (int)operators[o].second, (int)matches);

                                if(!matches)  // if a single expression doesn't match, then the whole diff doesn't match
                                    expressionsMatch = false;

                                break;
                            }
                        }
                    }

                    break;
                }
            }

            // if this is not an expression, add the token to the literalSearchStrings array
            if(!expression) {
                // only add it if it doesn't exist yet
                // this check is only necessary due to multiple redundant parser executions (one per diff!)
                bool exists = false;
                for(size_t l = 0; l < literalSearchStrings.size(); l++) {
                    if(literalSearchStrings[l] == searchStringTokens[i]) {
                        exists = true;
                        break;
                    }
                }

                if(!exists) {
                    const UString litAdd = searchStringTokens[i].trim();
                    if(litAdd.length() > 0 && !litAdd.isWhitespaceOnly()) literalSearchStrings.push_back(litAdd);
                }
            }
        }

        if(expressionsMatch)  // as soon as one difficulty matches all expressions, we are done here
        {
            expressionMatches = true;
            break;
        }
    }

    // if no diff matched any expression, then we can already stop here
    if(!expressionMatches) return false;

    bool hasAnyValidLiteralSearchString = false;
    for(size_t i = 0; i < literalSearchStrings.size(); i++) {
        if(literalSearchStrings[i].length() > 0) {
            hasAnyValidLiteralSearchString = true;
            break;
        }
    }

    // early return here for literal match/contains
    if(hasAnyValidLiteralSearchString) {
        for(size_t i = 0; i < numDiffs; i++) {
            const DatabaseBeatmap *diff = (isContainer ? diffs[i] : databaseBeatmap);

            bool atLeastOneFullMatch = true;

            for(size_t s = 0; s < literalSearchStrings.size(); s++) {
                if(!findSubstringInDifficulty(diff, literalSearchStrings[s])) atLeastOneFullMatch = false;
            }

            // as soon as one diff matches all strings, we are done
            if(atLeastOneFullMatch) return true;
        }

        // expression may have matched, but literal didn't match, so the entire beatmap doesn't match
        return false;
    }

    return expressionMatches;
}

bool SongBrowser::findSubstringInDifficulty(const DatabaseBeatmap *diff, const UString &searchString) {
    if(diff->getTitle().length() > 0) {
        if(strcasestr(diff->getTitle().c_str(), searchString.toUtf8()) != NULL) return true;
    }

    if(diff->getArtist().length() > 0) {
        if(strcasestr(diff->getArtist().c_str(), searchString.toUtf8()) != NULL) return true;
    }

    if(diff->getCreator().length() > 0) {
        if(strcasestr(diff->getCreator().c_str(), searchString.toUtf8()) != NULL) return true;
    }

    if(diff->getDifficultyName().length() > 0) {
        if(strcasestr(diff->getDifficultyName().c_str(), searchString.toUtf8()) != NULL) return true;
    }

    if(diff->getSource().length() > 0) {
        if(strcasestr(diff->getSource().c_str(), searchString.toUtf8()) != NULL) return true;
    }

    if(diff->getTags().length() > 0) {
        if(strcasestr(diff->getTags().c_str(), searchString.toUtf8()) != NULL) return true;
    }

    if(diff->getID() > 0) {
        auto id = std::to_string(diff->getID());
        if(strcasestr(id.c_str(), searchString.toUtf8()) != NULL) return true;
    }

    if(diff->getSetID() > 0) {
        auto set_id = std::to_string(diff->getSetID());
        if(strcasestr(set_id.c_str(), searchString.toUtf8()) != NULL) return true;
    }

    return false;
}

void SongBrowser::updateLayout() {
    ScreenBackable::updateLayout();

    const float uiScale = cv_ui_scale.getFloat();
    const float dpiScale = Osu::getUIScale();

    const int margin = 5 * dpiScale;

    // top bar
    m_fSongSelectTopScale = Osu::getImageScaleToFitResolution(osu->getSkin()->getSongSelectTop(), osu->getScreenSize());
    const float songSelectTopHeightScaled =
        max(osu->getSkin()->getSongSelectTop()->getHeight() * m_fSongSelectTopScale,
            m_songInfo->getMinimumHeight() * 1.5f + margin);  // NOTE: the height is a heuristic here more or less
    m_fSongSelectTopScale =
        max(m_fSongSelectTopScale, songSelectTopHeightScaled / osu->getSkin()->getSongSelectTop()->getHeight());
    m_fSongSelectTopScale *=
        uiScale;  // NOTE: any user osu_ui_scale below 1.0 will break things (because songSelectTop image)

    // topbar left (NOTE: the right side of the max() width is commented to keep the scorebrowser width consistent,
    // and because it's not really needed anyway)
    m_topbarLeft->setSize(max(osu->getSkin()->getSongSelectTop()->getWidth() * m_fSongSelectTopScale *
                                      cv_songbrowser_topbar_left_width_percent.getFloat() +
                                  margin,
                              /*m_songInfo->getMinimumWidth() + margin*/ 0.0f),
                          max(osu->getSkin()->getSongSelectTop()->getHeight() * m_fSongSelectTopScale *
                                  cv_songbrowser_topbar_left_percent.getFloat(),
                              m_songInfo->getMinimumHeight() + margin));
    m_songInfo->setRelPos(margin, margin);
    m_songInfo->setSize(m_topbarLeft->getSize().x - margin,
                        max(m_topbarLeft->getSize().y * 0.75f, m_songInfo->getMinimumHeight() + margin));

    const int topbarLeftButtonMargin = 5 * dpiScale;
    const int topbarLeftButtonHeight = 30 * dpiScale;
    const int topbarLeftButtonWidth = 55 * dpiScale;
    for(int i = 0; i < m_topbarLeftButtons.size(); i++) {
        m_topbarLeftButtons[i]->onResized();  // HACKHACK: framework bug (should update string metrics on setSize())
        m_topbarLeftButtons[i]->setSize(topbarLeftButtonWidth, topbarLeftButtonHeight);
        m_topbarLeftButtons[i]->setRelPos(
            m_topbarLeft->getSize().x - (i + 1) * (topbarLeftButtonMargin + topbarLeftButtonWidth),
            m_topbarLeft->getSize().y - m_topbarLeftButtons[i]->getSize().y);
    }

    const int topbarLeftTabButtonMargin = topbarLeftButtonMargin;
    const int topbarLeftTabButtonHeight = topbarLeftButtonHeight;
    const int topbarLeftTabButtonWidth = m_topbarLeft->getSize().x - 3 * topbarLeftTabButtonMargin -
                                         m_topbarLeftButtons.size() * (topbarLeftButtonWidth + topbarLeftButtonMargin);
    for(int i = 0; i < m_topbarLeftTabButtons.size(); i++) {
        m_topbarLeftTabButtons[i]->onResized();  // HACKHACK: framework bug (should update string metrics on setSize())
        m_topbarLeftTabButtons[i]->setSize(topbarLeftTabButtonWidth, topbarLeftTabButtonHeight);
        m_topbarLeftTabButtons[i]->setRelPos((topbarLeftTabButtonMargin + i * topbarLeftTabButtonWidth),
                                             m_topbarLeft->getSize().y - m_topbarLeftTabButtons[i]->getSize().y);
    }

    m_topbarLeft->update_pos();

    // topbar right
    m_topbarRight->setPosX(osu->getSkin()->getSongSelectTop()->getWidth() * m_fSongSelectTopScale *
                           cv_songbrowser_topbar_right_percent.getFloat());
    m_topbarRight->setSize(osu->getScreenWidth() - m_topbarRight->getPos().x,
                           osu->getSkin()->getSongSelectTop()->getHeight() * m_fSongSelectTopScale *
                               cv_songbrowser_topbar_right_height_percent.getFloat());

    float btn_margin = 10.f * dpiScale;
    m_sortButton->setSize(200.f * dpiScale, 30.f * dpiScale);
    m_sortButton->setRelPos(m_topbarRight->getSize().x - (m_sortButton->getSize().x + btn_margin), btn_margin);

    m_sortLabel->onResized();  // HACKHACK: framework bug (should update string metrics on setSizeToContent())
    m_sortLabel->setSizeToContent(3 * dpiScale);
    m_sortLabel->setRelPos(m_sortButton->getRelPos().x - (m_sortLabel->getSize().x + btn_margin),
                           (m_sortLabel->getSize().y + btn_margin) / 2.f);

    m_groupButton->setSize(m_sortButton->getSize());
    m_groupButton->setRelPos(m_sortLabel->getRelPos().x - (m_sortButton->getSize().x + 30.f * dpiScale + btn_margin),
                             btn_margin);

    m_groupLabel->onResized();  // HACKHACK: framework bug (should update string metrics on setSizeToContent())
    m_groupLabel->setSizeToContent(3 * dpiScale);
    m_groupLabel->setRelPos(m_groupButton->getRelPos().x - (m_groupLabel->getSize().x + btn_margin),
                            (m_groupLabel->getSize().y + btn_margin) / 2.f);

    m_topbarRight->update_pos();

    // bottombar
    const int bottomBarHeight = osu->getScreenHeight() * cv_songbrowser_bottombar_percent.getFloat() * uiScale;

    m_bottombar->setPosY(osu->getScreenHeight() - bottomBarHeight);
    m_bottombar->setSize(osu->getScreenWidth(), bottomBarHeight);

    // nav bar
    const bool isWidescreen =
        ((int)(max(0, (int)((osu->getScreenWidth() - (osu->getScreenHeight() * 4.0f / 3.0f)) / 2.0f))) > 0);
    const float navBarXCounter = Osu::getUIScale((isWidescreen ? 140.0f : 120.0f));

    // bottombar cont
    for(int i = 0; i < m_bottombarNavButtons.size(); i++) {
        m_bottombarNavButtons[i]->setSize(osu->getScreenWidth(), bottomBarHeight);
    }
    for(int i = 0; i < m_bottombarNavButtons.size(); i++) {
        const int gap = (i == 1 ? Osu::getUIScale(3.0f) : 0) + (i == 2 ? Osu::getUIScale(2.0f) : 0);

        // new, hardcoded offsets instead of dynamically using the button skin image widths (except starting at 3rd
        // button)
        m_bottombarNavButtons[i]->setRelPosX(
            navBarXCounter + gap + (i > 0 ? Osu::getUIScale(57.6f) : 0) +
            (i > 1 ? max((i - 1) * Osu::getUIScale(48.0f), m_bottombarNavButtons[i - 1]->getSize().x) : 0));

        // old, overflows with some skins (e.g. kyu)
        // m_bottombarNavButtons[i]->setRelPosX((i == 0 ? navBarXCounter : 0) + gap + (i > 0 ?
        // m_bottombarNavButtons[i-1]->getRelPos().x + m_bottombarNavButtons[i-1]->getSize().x : 0));
    }

    const int userButtonHeight = m_bottombar->getSize().y * 0.9f;
    m_userButton->setSize(userButtonHeight * 3.5f, userButtonHeight);
    m_userButton->setRelPos(max(m_bottombar->getSize().x / 2 - m_userButton->getSize().x / 2,
                                m_bottombarNavButtons[m_bottombarNavButtons.size() - 1]->getRelPos().x +
                                    m_bottombarNavButtons[m_bottombarNavButtons.size() - 1]->getSize().x + 10),
                            m_bottombar->getSize().y - m_userButton->getSize().y - 1);

    m_bottombar->update_pos();

    // score browser
    const int scoreBrowserExtraPaddingRight = 5 * dpiScale;  // duplication, see below
    updateScoreBrowserLayout();

    // song browser
    m_songBrowser->setPos(m_topbarLeft->getPos().x + m_topbarLeft->getSize().x + 1 + scoreBrowserExtraPaddingRight,
                          m_topbarRight->getPos().y + m_topbarRight->getSize().y + 2);
    m_songBrowser->setSize(
        osu->getScreenWidth() - (m_topbarLeft->getPos().x + m_topbarLeft->getSize().x + scoreBrowserExtraPaddingRight),
        osu->getScreenHeight() - m_songBrowser->getPos().y - m_bottombar->getSize().y + 2);
    updateSongButtonLayout();

    m_search->setPos(m_songBrowser->getPos());
    m_search->setSize(m_songBrowser->getSize());
}

void SongBrowser::onBack() { osu->toggleSongBrowser(); }

void SongBrowser::updateScoreBrowserLayout() {
    const float dpiScale = Osu::getUIScale();

    const bool shouldScoreBrowserBeVisible =
        (cv_scores_enabled.getBool() && cv_songbrowser_scorebrowser_enabled.getBool());
    if(shouldScoreBrowserBeVisible != m_scoreBrowser->isVisible())
        m_scoreBrowser->setVisible(shouldScoreBrowserBeVisible);

    const int scoreBrowserExtraPaddingRight = 5 * dpiScale;  // duplication, see above
    const int scoreButtonWidthMax = m_topbarLeft->getSize().x + 2 * dpiScale;
    const float browserHeight =
        m_bottombar->getPos().y - (m_topbarLeft->getPos().y + m_topbarLeft->getSize().y) + 2 * dpiScale;
    m_scoreBrowser->setPos(m_topbarLeft->getPos().x - 2 * dpiScale,
                           m_topbarLeft->getPos().y + m_topbarLeft->getSize().y);
    m_scoreBrowser->setSize(scoreButtonWidthMax + scoreBrowserExtraPaddingRight, browserHeight);
    int scoreHeight = 100;
    {
        Image *menuButtonBackground = osu->getSkin()->getMenuButtonBackground();
        Vector2 minimumSize = Vector2(699.0f, 103.0f) * (osu->getSkin()->isMenuButtonBackground2x() ? 2.0f : 1.0f);
        float minimumScale = Osu::getImageScaleToFitResolution(menuButtonBackground, minimumSize);
        float scoreScale = Osu::getImageScale(menuButtonBackground->getSize() * minimumScale, 64.0f);
        scoreScale *= 0.5f;
        scoreHeight = (int)(menuButtonBackground->getHeight() * scoreScale);

        float scale =
            Osu::getImageScaleToFillResolution(menuButtonBackground, Vector2(scoreButtonWidthMax, scoreHeight));
        scoreHeight = max(scoreHeight, (int)(menuButtonBackground->getHeight() * scale));

        // limit to scrollview width (while keeping the aspect ratio)
        const float ratio = minimumSize.x / minimumSize.y;
        if(scoreHeight * ratio > scoreButtonWidthMax) scoreHeight = m_scoreBrowser->getSize().x / ratio;
    }

    if(m_localBestContainer->isVisible()) {
        float local_best_size = scoreHeight + 40;
        m_scoreBrowser->setSize(m_scoreBrowser->getSize().x, browserHeight - local_best_size);
        m_scoreBrowser->setScrollSizeToContent();
        m_localBestContainer->setPos(m_scoreBrowser->getPos().x,
                                     m_scoreBrowser->getPos().y + m_scoreBrowser->getSize().y);
        m_localBestContainer->setSize(m_scoreBrowser->getPos().x, local_best_size);
        m_localBestLabel->setRelPos(m_scoreBrowser->getPos().x, 0);
        m_localBestLabel->setSize(m_scoreBrowser->getSize().x, 40);
        if(m_localBestButton) {
            m_localBestButton->setRelPos(m_scoreBrowser->getPos().x, 40);
            m_localBestButton->setSize(m_scoreBrowser->getSize().x, scoreHeight);
        }
    }

    const std::vector<CBaseUIElement *> &elements = m_scoreBrowser->getContainer()->getElements();
    for(size_t i = 0; i < elements.size(); i++) {
        CBaseUIElement *scoreButton = elements[i];
        scoreButton->setSize(m_scoreBrowser->getSize().x, scoreHeight);
        scoreButton->setRelPos(scoreBrowserExtraPaddingRight, i * scoreButton->getSize().y + 5 * dpiScale);
    }
    m_scoreBrowserScoresStillLoadingElement->setSize(m_scoreBrowser->getSize().x * 0.9f, scoreHeight * 0.75f);
    m_scoreBrowserScoresStillLoadingElement->setRelPos(
        m_scoreBrowser->getSize().x / 2 - m_scoreBrowserScoresStillLoadingElement->getSize().x / 2,
        (browserHeight / 2) * 0.65f - m_scoreBrowserScoresStillLoadingElement->getSize().y / 2);
    m_scoreBrowserNoRecordsYetElement->setSize(m_scoreBrowser->getSize().x * 0.9f, scoreHeight * 0.75f);
    if(elements[0] == m_scoreBrowserNoRecordsYetElement) {
        m_scoreBrowserNoRecordsYetElement->setRelPos(
            m_scoreBrowser->getSize().x / 2 - m_scoreBrowserScoresStillLoadingElement->getSize().x / 2,
            (browserHeight / 2) * 0.65f - m_scoreBrowserScoresStillLoadingElement->getSize().y / 2);
    } else {
        m_scoreBrowserNoRecordsYetElement->setRelPos(
            m_scoreBrowser->getSize().x / 2 - m_scoreBrowserScoresStillLoadingElement->getSize().x / 2, 45);
    }
    m_localBestContainer->update_pos();
    m_scoreBrowser->getContainer()->update_pos();
    m_scoreBrowser->setScrollSizeToContent();
}

void SongBrowser::rebuildScoreButtons() {
    if(!isVisible()) return;

    // XXX: When online, it would be nice to scroll to the current user's highscore

    // reset
    m_scoreBrowser->getContainer()->empty();
    m_localBestContainer->empty();
    m_localBestContainer->setVisible(false);

    const bool validBeatmap = (m_selectedBeatmap != NULL && m_selectedBeatmap->getSelectedDifficulty2() != NULL);
    bool is_online = cv_songbrowser_scores_sortingtype.getString() == UString("Online Leaderboard");

    std::vector<FinishedScore> scores;
    if(validBeatmap) {
        std::lock_guard<std::mutex> lock(m_db->m_scores_mtx);
        auto diff2 = m_selectedBeatmap->getSelectedDifficulty2();
        auto local_scores = (*m_db->getScores())[diff2->getMD5Hash()];
        auto local_best = max_element(local_scores.begin(), local_scores.end(),
                                      [](FinishedScore const &a, FinishedScore const &b) { return a.score < b.score; });

        if(is_online) {
            auto search = m_db->m_online_scores.find(diff2->getMD5Hash());
            if(search != m_db->m_online_scores.end()) {
                scores = search->second;

                if(local_best == local_scores.end()) {
                    if(!scores.empty()) {
                        // We only want to display "No scores" if there are online scores present
                        // Otherwise, it would be displayed twice
                        SAFE_DELETE(m_localBestButton);
                        m_localBestContainer->addBaseUIElement(m_localBestLabel);
                        m_localBestContainer->addBaseUIElement(m_scoreBrowserNoRecordsYetElement);
                        m_localBestContainer->setVisible(true);
                    }
                } else {
                    SAFE_DELETE(m_localBestButton);
                    m_localBestButton = new ScoreButton(m_contextMenu, 0, 0, 0, 0);
                    m_localBestButton->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onScoreClicked));
                    m_localBestButton->map_hash = diff2->getMD5Hash();
                    m_localBestButton->setScore(*local_best, diff2);
                    m_localBestButton->resetHighlight();
                    m_localBestContainer->addBaseUIElement(m_localBestLabel);
                    m_localBestContainer->addBaseUIElement(m_localBestButton);
                    m_localBestContainer->setVisible(true);
                }

                // We have already fetched the scores so there's no point in showing "Loading...".
                // When there are no online scores for this map, let's treat it as if we are
                // offline in order to show "No records set!" instead.
                is_online = false;
            } else {
                // We haven't fetched the scores yet, do so now
                fetch_online_scores(diff2);

                // Display local best while scores are loading
                if(local_best != local_scores.end()) {
                    SAFE_DELETE(m_localBestButton);
                    m_localBestButton = new ScoreButton(m_contextMenu, 0, 0, 0, 0);
                    m_localBestButton->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onScoreClicked));
                    m_localBestButton->map_hash = diff2->getMD5Hash();
                    m_localBestButton->setScore(*local_best, diff2);
                    m_localBestButton->resetHighlight();
                    m_localBestContainer->addBaseUIElement(m_localBestLabel);
                    m_localBestContainer->addBaseUIElement(m_localBestButton);
                    m_localBestContainer->setVisible(true);
                }
            }
        } else {
            scores = local_scores;
        }
    }

    const int numScores = scores.size();

    // top up cache as necessary
    if(numScores > m_scoreButtonCache.size()) {
        const int numNewButtons = numScores - m_scoreButtonCache.size();
        for(size_t i = 0; i < numNewButtons; i++) {
            ScoreButton *scoreButton = new ScoreButton(m_contextMenu, 0, 0, 0, 0);
            scoreButton->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onScoreClicked));
            m_scoreButtonCache.push_back(scoreButton);
        }
    }

    // and build the ui
    if(numScores < 1) {
        if(validBeatmap && is_online) {
            m_scoreBrowser->getContainer()->addBaseUIElement(m_scoreBrowserScoresStillLoadingElement,
                                                             m_scoreBrowserScoresStillLoadingElement->getRelPos().x,
                                                             m_scoreBrowserScoresStillLoadingElement->getRelPos().y);
        } else {
            m_scoreBrowser->getContainer()->addBaseUIElement(m_scoreBrowserNoRecordsYetElement,
                                                             m_scoreBrowserScoresStillLoadingElement->getRelPos().x,
                                                             m_scoreBrowserScoresStillLoadingElement->getRelPos().y);
        }
    } else {
        // sort
        m_db->sortScores(m_selectedBeatmap->getSelectedDifficulty2()->getMD5Hash());

        // build
        std::vector<ScoreButton *> scoreButtons;
        for(size_t i = 0; i < numScores; i++) {
            ScoreButton *button = m_scoreButtonCache[i];
            button->map_hash = m_selectedBeatmap->getSelectedDifficulty2()->getMD5Hash();
            button->setScore(scores[i], m_selectedBeatmap->getSelectedDifficulty2(), i + 1);
            scoreButtons.push_back(button);
        }

        // add
        for(size_t i = 0; i < numScores; i++) {
            scoreButtons[i]->setIndex(i + 1);
            m_scoreBrowser->getContainer()->addBaseUIElement(scoreButtons[i]);
        }

        // reset
        for(size_t i = 0; i < scoreButtons.size(); i++) {
            scoreButtons[i]->resetHighlight();
        }
    }

    // layout
    updateScoreBrowserLayout();
}

void SongBrowser::scheduleSearchUpdate(bool immediately) {
    m_fSearchWaitTime = engine->getTime() + (immediately ? 0.0f : cv_songbrowser_search_delay.getFloat());
}

void SongBrowser::checkHandleKillBackgroundSearchMatcher() {
    if(!m_backgroundSearchMatcher->isDead()) {
        m_backgroundSearchMatcher->kill();

        const double startTime = engine->getTimeReal();
        while(!m_backgroundSearchMatcher->isAsyncReady()) {
            if(engine->getTimeReal() - startTime > 2) {
                debugLog("WARNING: Ignoring stuck SearchMatcher thread!\n");
                break;
            }
        }
    }
}

UISelectionButton *SongBrowser::addBottombarNavButton(std::function<SkinImage *()> getImageFunc,
                                                      std::function<SkinImage *()> getImageOverFunc) {
    UISelectionButton *btn = new UISelectionButton(getImageFunc, getImageOverFunc, 0, 0, 0, 0, "");
    m_bottombar->addBaseUIElement(btn);
    m_bottombarNavButtons.push_back(btn);
    return btn;
}

CBaseUIButton *SongBrowser::addTopBarLeftTabButton(UString text) {
    CBaseUIButton *btn = new CBaseUIButton(0, 0, 0, 0, "", text);
    btn->setDrawBackground(false);
    m_topbarLeft->addBaseUIElement(btn);
    m_topbarLeftTabButtons.push_back(btn);
    return btn;
}

CBaseUIButton *SongBrowser::addTopBarLeftButton(UString text) {
    CBaseUIButton *btn = new CBaseUIButton(0, 0, 0, 0, "", text);
    btn->setDrawBackground(false);
    m_topbarLeft->addBaseUIElement(btn);
    m_topbarLeftButtons.push_back(btn);
    return btn;
}

void SongBrowser::onDatabaseLoadingFinished() {
    // having a copy of the vector in here is actually completely unnecessary
    m_beatmaps = std::vector<DatabaseBeatmap *>(m_db->getDatabaseBeatmaps());

    debugLog("SongBrowser::onDatabaseLoadingFinished() : %i beatmapsets.\n", m_beatmaps.size());

    // initialize all collection (grouped) buttons
    {
        // artist
        {
            // 0-9
            {
                CollectionButton *b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "",
                                                           "0-9", std::vector<SongButton *>());
                m_artistCollectionButtons.push_back(b);
            }

            // A-Z
            for(size_t i = 0; i < 26; i++) {
                UString artistCollectionName = UString::format("%c", 'A' + i);

                CollectionButton *b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "",
                                                           artistCollectionName, std::vector<SongButton *>());
                m_artistCollectionButtons.push_back(b);
            }

            // Other
            {
                CollectionButton *b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "",
                                                           "Other", std::vector<SongButton *>());
                m_artistCollectionButtons.push_back(b);
            }
        }

        // difficulty
        for(size_t i = 0; i < 12; i++) {
            UString difficultyCollectionName = UString::format(i == 1 ? "%i star" : "%i stars", i);
            if(i < 1) difficultyCollectionName = "Below 1 star";
            if(i > 10) difficultyCollectionName = "Above 10 stars";

            std::vector<SongButton *> children;

            CollectionButton *b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "",
                                                       difficultyCollectionName, children);
            m_difficultyCollectionButtons.push_back(b);
        }

        // bpm
        {
            CollectionButton *b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "",
                                                       "Under 60 BPM", std::vector<SongButton *>());
            m_bpmCollectionButtons.push_back(b);
            b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "Under 120 BPM",
                                     std::vector<SongButton *>());
            m_bpmCollectionButtons.push_back(b);
            b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "Under 180 BPM",
                                     std::vector<SongButton *>());
            m_bpmCollectionButtons.push_back(b);
            b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "Under 240 BPM",
                                     std::vector<SongButton *>());
            m_bpmCollectionButtons.push_back(b);
            b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "Under 300 BPM",
                                     std::vector<SongButton *>());
            m_bpmCollectionButtons.push_back(b);
            b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "Over 300 BPM",
                                     std::vector<SongButton *>());
            m_bpmCollectionButtons.push_back(b);
        }

        // creator
        {
            // 0-9
            {
                CollectionButton *b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "",
                                                           "0-9", std::vector<SongButton *>());
                m_creatorCollectionButtons.push_back(b);
            }

            // A-Z
            for(size_t i = 0; i < 26; i++) {
                UString artistCollectionName = UString::format("%c", 'A' + i);

                CollectionButton *b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "",
                                                           artistCollectionName, std::vector<SongButton *>());
                m_creatorCollectionButtons.push_back(b);
            }

            // Other
            {
                CollectionButton *b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "",
                                                           "Other", std::vector<SongButton *>());
                m_creatorCollectionButtons.push_back(b);
            }
        }

        // dateadded
        {
            // TODO: annoying
        }

        // length
        {
            CollectionButton *b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "",
                                                       "1 minute or less", std::vector<SongButton *>());
            m_lengthCollectionButtons.push_back(b);
            b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "2 minutes or less",
                                     std::vector<SongButton *>());
            m_lengthCollectionButtons.push_back(b);
            b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "3 minutes or less",
                                     std::vector<SongButton *>());
            m_lengthCollectionButtons.push_back(b);
            b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "4 minutes or less",
                                     std::vector<SongButton *>());
            m_lengthCollectionButtons.push_back(b);
            b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "5 minutes or less",
                                     std::vector<SongButton *>());
            m_lengthCollectionButtons.push_back(b);
            b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "10 minutes or less",
                                     std::vector<SongButton *>());
            m_lengthCollectionButtons.push_back(b);
            b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "", "Over 10 minutes",
                                     std::vector<SongButton *>());
            m_lengthCollectionButtons.push_back(b);
        }

        // title
        {
            // 0-9
            {
                CollectionButton *b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "",
                                                           "0-9", std::vector<SongButton *>());
                m_titleCollectionButtons.push_back(b);
            }

            // A-Z
            for(size_t i = 0; i < 26; i++) {
                UString artistCollectionName = UString::format("%c", 'A' + i);

                CollectionButton *b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "",
                                                           artistCollectionName, std::vector<SongButton *>());
                m_titleCollectionButtons.push_back(b);
            }

            // Other
            {
                CollectionButton *b = new CollectionButton(this, m_songBrowser, m_contextMenu, 250, 250, 200, 50, "",
                                                           "Other", std::vector<SongButton *>());
                m_titleCollectionButtons.push_back(b);
            }
        }
    }

    // add all beatmaps (build buttons)
    for(size_t i = 0; i < m_beatmaps.size(); i++) {
        addBeatmapSet(m_beatmaps[i]);
    }

    // build collections
    recreateCollectionsButtons();

    onSortChange(cv_songbrowser_sortingtype.getString());
    onSortScoresChange(cv_songbrowser_scores_sortingtype.getString());

    // update rich presence (discord total pp)
    RichPresence::onSongBrowser();

    // update user name/stats
    onUserCardChange(cv_name.getString());

    if(cv_songbrowser_search_hardcoded_filter.getString().length() > 0) onSearchUpdate();

    if(beatmap_to_reselect_after_db_load.hash[0] != 0) {
        auto beatmap = db->getBeatmapDifficulty(beatmap_to_reselect_after_db_load);
        if(beatmap) {
            onDifficultySelected(beatmap, false);
            selectSelectedBeatmapSongButton();
        }

        SAFE_DELETE(osu->m_mainMenu->preloaded_beatmapset);
    }

    // ok, if we still haven't selected a song, do so now
    if(m_selectedBeatmap->getSelectedDifficulty2() == NULL) {
        selectRandomBeatmap();
    }
}

void SongBrowser::onSearchUpdate() {
    const bool hasHardcodedSearchStringChanged =
        (m_sPrevHardcodedSearchString != cv_songbrowser_search_hardcoded_filter.getString());
    const bool hasSearchStringChanged = (m_sPrevSearchString != m_sSearchString);

    const bool prevInSearch = m_bInSearch;
    m_bInSearch = (m_sSearchString.length() > 0 || cv_songbrowser_search_hardcoded_filter.getString().length() > 0);
    const bool hasInSearchChanged = (prevInSearch != m_bInSearch);

    if(m_bInSearch) {
        m_searchPrevGroup = m_group;

        // flag all search matches across entire database
        if(hasSearchStringChanged || hasHardcodedSearchStringChanged || hasInSearchChanged) {
            // stop potentially running async search
            checkHandleKillBackgroundSearchMatcher();

            m_backgroundSearchMatcher->revive();
            m_backgroundSearchMatcher->release();
            m_backgroundSearchMatcher->setSongButtonsAndSearchString(
                m_songButtons, m_sSearchString, cv_songbrowser_search_hardcoded_filter.getString());

            engine->getResourceManager()->requestNextLoadAsync();
            engine->getResourceManager()->loadResource(m_backgroundSearchMatcher);
        } else
            rebuildSongButtonsAndVisibleSongButtonsWithSearchMatchSupport(true);

        // (results are handled in update() once available)
    } else  // exit search
    {
        // exiting the search does not need any async work, so we can just directly do it in here

        // stop potentially running async search
        checkHandleKillBackgroundSearchMatcher();

        // reset container and visible buttons list
        m_songBrowser->getContainer()->empty();
        m_visibleSongButtons.clear();

        // reset all search flags
        for(size_t i = 0; i < m_songButtons.size(); i++) {
            const auto &children = m_songButtons[i]->getChildren();
            if(children.size() > 0) {
                for(size_t c = 0; c < children.size(); c++) {
                    children[c]->setIsSearchMatch(true);
                }
            } else
                m_songButtons[i]->setIsSearchMatch(true);
        }

        // remember which tab was selected, instead of defaulting back to no grouping
        // this also rebuilds the visible buttons list
        for(size_t i = 0; i < m_groupings.size(); i++) {
            if(m_groupings[i].type == m_searchPrevGroup) onGroupChange("", m_groupings[i].id);
        }
    }

    m_sPrevSearchString = m_sSearchString;
    m_sPrevHardcodedSearchString = cv_songbrowser_search_hardcoded_filter.getString();
}

void SongBrowser::rebuildSongButtonsAndVisibleSongButtonsWithSearchMatchSupport(bool scrollToTop,
                                                                                bool doRebuildSongButtons) {
    // reset container and visible buttons list
    m_songBrowser->getContainer()->empty();
    m_visibleSongButtons.clear();

    // use flagged search matches to rebuild visible song buttons
    {
        if(m_group == GROUP::GROUP_NO_GROUPING) {
            for(size_t i = 0; i < m_songButtons.size(); i++) {
                const auto &children = m_songButtons[i]->getChildren();
                if(children.size() > 0) {
                    // if all children match, then we still want to display the parent wrapper button (without expanding
                    // all diffs)
                    bool allChildrenMatch = true;
                    for(size_t c = 0; c < children.size(); c++) {
                        if(!children[c]->isSearchMatch()) allChildrenMatch = false;
                    }

                    if(allChildrenMatch)
                        m_visibleSongButtons.push_back(m_songButtons[i]);
                    else {
                        // rip matching children from parent
                        for(size_t c = 0; c < children.size(); c++) {
                            if(children[c]->isSearchMatch()) m_visibleSongButtons.push_back(children[c]);
                        }
                    }
                } else if(m_songButtons[i]->isSearchMatch())
                    m_visibleSongButtons.push_back(m_songButtons[i]);
            }
        } else {
            std::vector<CollectionButton *> *groupButtons = NULL;
            {
                switch(m_group) {
                    case GROUP::GROUP_NO_GROUPING:
                        break;
                    case GROUP::GROUP_ARTIST:
                        groupButtons = &m_artistCollectionButtons;
                        break;
                    case GROUP::GROUP_BPM:
                        groupButtons = &m_bpmCollectionButtons;
                        break;
                    case GROUP::GROUP_CREATOR:
                        groupButtons = &m_creatorCollectionButtons;
                        break;
                    case GROUP::GROUP_DIFFICULTY:
                        groupButtons = &m_difficultyCollectionButtons;
                        break;
                    case GROUP::GROUP_LENGTH:
                        groupButtons = &m_lengthCollectionButtons;
                        break;
                    case GROUP::GROUP_TITLE:
                        groupButtons = &m_titleCollectionButtons;
                        break;
                    case GROUP::GROUP_COLLECTIONS:
                        groupButtons = &m_collectionButtons;
                        break;
                }
            }

            if(groupButtons != NULL) {
                for(size_t i = 0; i < groupButtons->size(); i++) {
                    bool isAnyMatchInGroup = false;

                    const auto &children = (*groupButtons)[i]->getChildren();
                    for(size_t c = 0; c < children.size(); c++) {
                        const auto &childrenChildren = children[c]->getChildren();
                        if(childrenChildren.size() > 0) {
                            for(size_t cc = 0; cc < childrenChildren.size(); cc++) {
                                if(childrenChildren[cc]->isSearchMatch()) {
                                    isAnyMatchInGroup = true;
                                    break;
                                }
                            }

                            if(isAnyMatchInGroup) break;
                        } else if(children[c]->isSearchMatch()) {
                            isAnyMatchInGroup = true;
                            break;
                        }
                    }

                    if(isAnyMatchInGroup || !m_bInSearch) m_visibleSongButtons.push_back((*groupButtons)[i]);
                }
            }
        }

        if(doRebuildSongButtons) rebuildSongButtons();

        // scroll to top search result, or auto select the only result
        if(scrollToTop) {
            if(m_visibleSongButtons.size() > 1)
                scrollToSongButton(m_visibleSongButtons[0]);
            else if(m_visibleSongButtons.size() > 0) {
                selectSongButton(m_visibleSongButtons[0]);
                m_songBrowser->scrollY(1);
            }
        }
    }
}

void SongBrowser::onSortScoresClicked(CBaseUIButton *button) {
    m_contextMenu->setPos(button->getPos());
    m_contextMenu->setRelPos(button->getRelPos());
    m_contextMenu->begin(button->getSize().x);
    {
        CBaseUIButton *button = m_contextMenu->addButton("Online Leaderboard");
        if(cv_songbrowser_scores_sortingtype.getString() == UString("Online Leaderboard"))
            button->setTextBrightColor(0xff00ff00);

        const std::vector<Database::SCORE_SORTING_METHOD> &scoreSortingMethods = m_db->getScoreSortingMethods();
        for(size_t i = 0; i < scoreSortingMethods.size(); i++) {
            CBaseUIButton *button = m_contextMenu->addButton(scoreSortingMethods[i].name);
            if(scoreSortingMethods[i].name == cv_songbrowser_scores_sortingtype.getString())
                button->setTextBrightColor(0xff00ff00);
        }
    }
    m_contextMenu->end(false, false);
    m_contextMenu->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onSortScoresChange));
}

void SongBrowser::onSortScoresChange(UString text, int id) {
    cv_songbrowser_scores_sortingtype.setValue(text);  // NOTE: remember
    m_scoreSortButton->setText(text);
    rebuildScoreButtons();
    m_scoreBrowser->scrollToTop();

    // update grades of all visible songdiffbuttons
    if(m_selectedBeatmap != NULL) {
        for(size_t i = 0; i < m_visibleSongButtons.size(); i++) {
            if(m_visibleSongButtons[i]->getDatabaseBeatmap() == m_selectedBeatmap->getSelectedDifficulty2()) {
                SongButton *songButtonPointer = dynamic_cast<SongButton *>(m_visibleSongButtons[i]);
                if(songButtonPointer != NULL) {
                    for(Button *diffButton : songButtonPointer->getChildren()) {
                        SongButton *diffButtonPointer = dynamic_cast<SongButton *>(diffButton);
                        if(diffButtonPointer != NULL) diffButtonPointer->updateGrade();
                    }
                }
            }
        }
    }
}

void SongBrowser::onWebClicked(CBaseUIButton *button) {
    if(m_songInfo->getBeatmapID() > 0) {
        env->openURLInDefaultBrowser(UString::format("https://osu.ppy.sh/b/%ld", m_songInfo->getBeatmapID()));
        osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
    }
}

void SongBrowser::onGroupClicked(CBaseUIButton *button) {
    m_contextMenu->setPos(button->getPos());
    m_contextMenu->setRelPos(button->getRelPos());
    m_contextMenu->begin(button->getSize().x);
    {
        for(size_t i = 0; i < m_groupings.size(); i++) {
            CBaseUIButton *button = m_contextMenu->addButton(m_groupings[i].name, m_groupings[i].id);
            if(m_groupings[i].type == m_group) button->setTextBrightColor(0xff00ff00);
        }
    }
    m_contextMenu->end(false, false);
    m_contextMenu->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onGroupChange));
}

void SongBrowser::onGroupChange(UString text, int id) {
    GROUPING *grouping = (m_groupings.size() > 0 ? &m_groupings[0] : NULL);
    for(size_t i = 0; i < m_groupings.size(); i++) {
        if(m_groupings[i].id == id || (text.length() > 1 && m_groupings[i].name == text)) {
            grouping = &m_groupings[i];
            break;
        }
    }
    if(grouping == NULL) return;

    const Color highlightColor = COLOR(255, 0, 255, 0);
    const Color defaultColor = COLOR(255, 255, 255, 255);

    // update group combobox button text
    m_groupButton->setText(grouping->name);

    // and update the actual songbrowser contents
    switch(grouping->type) {
        case GROUP::GROUP_NO_GROUPING:
            onGroupNoGrouping();
            break;
        case GROUP::GROUP_ARTIST:
            onGroupArtist();
            break;
        case GROUP::GROUP_BPM:
            onGroupBPM();
            break;
        case GROUP::GROUP_CREATOR:
            onGroupCreator();
            break;
        case GROUP::GROUP_DATEADDED:
            onGroupDateadded();
            break;
        case GROUP::GROUP_DIFFICULTY:
            onGroupDifficulty();
            break;
        case GROUP::GROUP_LENGTH:
            onGroupLength();
            break;
        case GROUP::GROUP_TITLE:
            onGroupTitle();
            break;
        case GROUP::GROUP_COLLECTIONS:
            onGroupCollections();
            break;
    }
}

void SongBrowser::onSortClicked(CBaseUIButton *button) {
    m_contextMenu->setPos(button->getPos());
    m_contextMenu->setRelPos(button->getRelPos());
    m_contextMenu->begin(button->getSize().x);
    {
        for(size_t i = 0; i < m_sortingMethods.size(); i++) {
            CBaseUIButton *button = m_contextMenu->addButton(m_sortingMethods[i].name);
            if(m_sortingMethods[i].type == m_sortingMethod) button->setTextBrightColor(0xff00ff00);
        }
    }
    m_contextMenu->end(false, false);
    m_contextMenu->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onSortChange));
}

void SongBrowser::onSortChange(UString text, int id) { onSortChangeInt(text, true); }

void SongBrowser::onSortChangeInt(UString text, bool autoScroll) {
    SORTING_METHOD *sortingMethod = &m_sortingMethods[3];
    for(size_t i = 0; i < m_sortingMethods.size(); i++) {
        if(m_sortingMethods[i].name == text) {
            sortingMethod = &m_sortingMethods[i];
            break;
        }
    }

    m_sortingMethod = sortingMethod->type;
    m_sortingComparator = sortingMethod->comparator;
    m_sortButton->setText(sortingMethod->name);

    cv_songbrowser_sortingtype.setValue(sortingMethod->name);  // NOTE: remember persistently

    // resort primitive master button array (all songbuttons, No Grouping)
    std::sort(m_songButtons.begin(), m_songButtons.end(), m_sortingComparator);

    // resort Collection buttons (one button for each collection)
    // these are always sorted alphabetically by name
    std::sort(m_collectionButtons.begin(), m_collectionButtons.end(), [](CollectionButton *a, CollectionButton *b) {
        int res = strcasecmp(a->getCollectionName().c_str(), b->getCollectionName().c_str());
        if(res == 0) return a->getSortHack() < b->getSortHack();
        return res < 0;
    });

    // resort Collection button array (each group of songbuttons inside each Collection)
    for(size_t i = 0; i < m_collectionButtons.size(); i++) {
        auto &children = m_collectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), m_sortingComparator);
        m_collectionButtons[i]->setChildren(children);
    }

    // etc.
    for(size_t i = 0; i < m_artistCollectionButtons.size(); i++) {
        auto &children = m_artistCollectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), m_sortingComparator);
        m_artistCollectionButtons[i]->setChildren(children);
    }
    for(size_t i = 0; i < m_bpmCollectionButtons.size(); i++) {
        auto &children = m_bpmCollectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), m_sortingComparator);
        m_bpmCollectionButtons[i]->setChildren(children);
    }
    for(size_t i = 0; i < m_difficultyCollectionButtons.size(); i++) {
        auto &children = m_difficultyCollectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), m_sortingComparator);
        m_difficultyCollectionButtons[i]->setChildren(children);
    }
    for(size_t i = 0; i < m_bpmCollectionButtons.size(); i++) {
        auto &children = m_bpmCollectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), m_sortingComparator);
        m_bpmCollectionButtons[i]->setChildren(children);
    }
    for(size_t i = 0; i < m_creatorCollectionButtons.size(); i++) {
        auto &children = m_creatorCollectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), m_sortingComparator);
        m_creatorCollectionButtons[i]->setChildren(children);
    }
    for(size_t i = 0; i < m_dateaddedCollectionButtons.size(); i++) {
        auto &children = m_dateaddedCollectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), m_sortingComparator);
        m_dateaddedCollectionButtons[i]->setChildren(children);
    }
    for(size_t i = 0; i < m_lengthCollectionButtons.size(); i++) {
        auto &children = m_lengthCollectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), m_sortingComparator);
        m_lengthCollectionButtons[i]->setChildren(children);
    }
    for(size_t i = 0; i < m_titleCollectionButtons.size(); i++) {
        auto &children = m_titleCollectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), m_sortingComparator);
        m_titleCollectionButtons[i]->setChildren(children);
    }

    // we only need to update the visible buttons array if we are in No Grouping (because Collections always get sorted
    // by the collection name on the first level)
    if(m_group == GROUP::GROUP_NO_GROUPING) {
        m_visibleSongButtons.clear();
        m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_songButtons.begin(), m_songButtons.end());
    }

    rebuildSongButtons();
    onAfterSortingOrGroupChange(autoScroll);
}

void SongBrowser::onGroupNoGrouping() {
    m_group = GROUP::GROUP_NO_GROUPING;

    m_visibleSongButtons.clear();
    m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_songButtons.begin(), m_songButtons.end());

    rebuildSongButtons();
    onAfterSortingOrGroupChange();
}

void SongBrowser::onGroupCollections(bool autoScroll) {
    m_group = GROUP::GROUP_COLLECTIONS;

    m_visibleSongButtons.clear();
    m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_collectionButtons.begin(), m_collectionButtons.end());

    rebuildSongButtons();
    onAfterSortingOrGroupChange(autoScroll);
}

void SongBrowser::onGroupArtist() {
    m_group = GROUP::GROUP_ARTIST;

    m_visibleSongButtons.clear();
    m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_artistCollectionButtons.begin(),
                                m_artistCollectionButtons.end());

    rebuildSongButtons();
    onAfterSortingOrGroupChange();
}

void SongBrowser::onGroupDifficulty() {
    m_group = GROUP::GROUP_DIFFICULTY;

    m_visibleSongButtons.clear();
    m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_difficultyCollectionButtons.begin(),
                                m_difficultyCollectionButtons.end());

    rebuildSongButtons();
    onAfterSortingOrGroupChange();
}

void SongBrowser::onGroupBPM() {
    m_group = GROUP::GROUP_BPM;

    m_visibleSongButtons.clear();
    m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_bpmCollectionButtons.begin(),
                                m_bpmCollectionButtons.end());

    rebuildSongButtons();
    onAfterSortingOrGroupChange();
}

void SongBrowser::onGroupCreator() {
    m_group = GROUP::GROUP_CREATOR;

    m_visibleSongButtons.clear();
    m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_creatorCollectionButtons.begin(),
                                m_creatorCollectionButtons.end());

    rebuildSongButtons();
    onAfterSortingOrGroupChange();
}

void SongBrowser::onGroupDateadded() {
    m_group = GROUP::GROUP_DATEADDED;

    m_visibleSongButtons.clear();
    m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_dateaddedCollectionButtons.begin(),
                                m_dateaddedCollectionButtons.end());

    rebuildSongButtons();
    onAfterSortingOrGroupChange();
}

void SongBrowser::onGroupLength() {
    m_group = GROUP::GROUP_LENGTH;

    m_visibleSongButtons.clear();
    m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_lengthCollectionButtons.begin(),
                                m_lengthCollectionButtons.end());

    rebuildSongButtons();
    onAfterSortingOrGroupChange();
}

void SongBrowser::onGroupTitle() {
    m_group = GROUP::GROUP_TITLE;

    m_visibleSongButtons.clear();
    m_visibleSongButtons.insert(m_visibleSongButtons.end(), m_titleCollectionButtons.begin(),
                                m_titleCollectionButtons.end());

    rebuildSongButtons();
    onAfterSortingOrGroupChange();
}

void SongBrowser::onAfterSortingOrGroupChange(bool autoScroll) {
    // keep search state consistent between tab changes
    if(m_bInSearch) onSearchUpdate();

    // (can't call it right here because we maybe have async)
    m_scheduled_scroll_to_selected_button = autoScroll;
}

void SongBrowser::onSelectionMode() {
    m_contextMenu->setPos(m_bottombarNavButtons[0]->getPos());
    m_contextMenu->setRelPos(m_bottombarNavButtons[0]->getRelPos());
    m_contextMenu->begin(0, true);
    {
        UIContextMenuButton *standardButton = m_contextMenu->addButton("Standard", 0);
        standardButton->setTextLeft(false);
        standardButton->setTooltipText("Standard 2D circle clicking.");
        UIContextMenuButton *fposuButton = m_contextMenu->addButton("FPoSu", 2);
        fposuButton->setTextLeft(false);
        fposuButton->setTooltipText(
            "The real 3D FPS mod.\nPlay from a first person shooter perspective in a 3D environment.\nThis is only "
            "intended for mouse! (Enable \"Tablet/Absolute Mode\" for tablets.)");

        CBaseUIButton *activeButton = NULL;
        {
            if(cv_mod_fposu.getBool())
                activeButton = fposuButton;
            else
                activeButton = standardButton;
        }
        if(activeButton != NULL) activeButton->setTextBrightColor(0xff00ff00);
    }
    m_contextMenu->setPos(m_contextMenu->getPos() -
                          Vector2((m_contextMenu->getSize().x - m_bottombarNavButtons[0]->getSize().x) / 2.0f,
                                  m_contextMenu->getSize().y));
    m_contextMenu->setRelPos(m_contextMenu->getRelPos() -
                             Vector2((m_contextMenu->getSize().x - m_bottombarNavButtons[0]->getSize().x) / 2.0f,
                                     m_contextMenu->getSize().y));
    m_contextMenu->end(true, false);
    m_contextMenu->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onModeChange2));
}

void SongBrowser::onSelectionMods() {
    engine->getSound()->play(osu->getSkin()->m_expand);
    osu->toggleModSelection(m_bF1Pressed);
}

void SongBrowser::onSelectionRandom() {
    engine->getSound()->play(osu->getSkin()->m_clickButton);
    if(m_bShiftPressed)
        m_bPreviousRandomBeatmapScheduled = true;
    else
        m_bRandomBeatmapScheduled = true;
}

void SongBrowser::onSelectionOptions() {
    engine->getSound()->play(osu->getSkin()->m_clickButton);

    if(m_selectedButton != NULL) {
        scrollToSongButton(m_selectedButton);

        const Vector2 heuristicSongButtonPositionAfterSmoothScrollFinishes =
            (m_songBrowser->getPos() + m_songBrowser->getSize() / 2);

        SongButton *songButtonPointer = dynamic_cast<SongButton *>(m_selectedButton);
        CollectionButton *collectionButtonPointer = dynamic_cast<CollectionButton *>(m_selectedButton);
        if(songButtonPointer != NULL)
            songButtonPointer->triggerContextMenu(heuristicSongButtonPositionAfterSmoothScrollFinishes);
        else if(collectionButtonPointer != NULL)
            collectionButtonPointer->triggerContextMenu(heuristicSongButtonPositionAfterSmoothScrollFinishes);
    }
}

void SongBrowser::onModeChange(UString text) { onModeChange2(text); }

void SongBrowser::onModeChange2(UString text, int id) { cv_mod_fposu.setValue(id == 2 || text == UString("fposu")); }

void SongBrowser::onUserCardChange(UString new_username) {
    // NOTE: force update options textbox to avoid shutdown inconsistency
    osu->getOptionsMenu()->setUsername(new_username);
    m_userButton->setID(bancho.user_id);
}

void SongBrowser::onScoreClicked(CBaseUIButton *button) {
    ScoreButton *scoreButton = (ScoreButton *)button;

    // NOTE: the order of these two calls matters
    osu->getRankingScreen()->setScore(scoreButton->getScore());
    osu->getRankingScreen()->setBeatmapInfo(m_selectedBeatmap, m_selectedBeatmap->getSelectedDifficulty2());

    osu->getSongBrowser()->setVisible(false);
    osu->getRankingScreen()->setVisible(true);

    engine->getSound()->play(osu->getSkin()->m_menuHit);
}

void SongBrowser::onScoreContextMenu(ScoreButton *scoreButton, int id) {
    // NOTE: see ScoreButton::onContextMenu()

    if(id == 2) {
        m_db->deleteScore(scoreButton->map_hash, scoreButton->getScoreUnixTimestamp());

        rebuildScoreButtons();
        m_userButton->updateUserStats();
    }
}

void SongBrowser::onSongButtonContextMenu(SongButton *songButton, UString text, int id) {
    // debugLog("SongBrowser::onSongButtonContextMenu(%p, %s, %i)\n", songButton, text.toUtf8(), id);

    struct CollectionManagementHelper {
        static std::vector<MD5Hash> getBeatmapSetHashesForSongButton(SongButton *songButton, Database *db) {
            std::vector<MD5Hash> beatmapSetHashes;
            {
                const auto &songButtonChildren = songButton->getChildren();
                if(songButtonChildren.size() > 0) {
                    for(size_t i = 0; i < songButtonChildren.size(); i++) {
                        beatmapSetHashes.push_back(songButtonChildren[i]->getDatabaseBeatmap()->getMD5Hash());
                    }
                } else {
                    const DatabaseBeatmap *beatmap =
                        db->getBeatmapDifficulty(songButton->getDatabaseBeatmap()->getMD5Hash());
                    if(beatmap != NULL) {
                        const std::vector<DatabaseBeatmap *> &diffs = beatmap->getDifficulties();
                        for(size_t i = 0; i < diffs.size(); i++) {
                            beatmapSetHashes.push_back(diffs[i]->getMD5Hash());
                        }
                    }
                }
            }
            return beatmapSetHashes;
        }
    };

    bool updateUIScheduled = false;
    {
        if(id == 1) {
            // add diff to collection
            std::string name = text.toUtf8();
            auto collection = get_or_create_collection(name);
            collection->add_map(songButton->getDatabaseBeatmap()->getMD5Hash());
            save_collections();
            updateUIScheduled = true;
        } else if(id == 2) {
            // add set to collection
            std::string name = text.toUtf8();
            auto collection = get_or_create_collection(name);
            const std::vector<MD5Hash> beatmapSetHashes =
                CollectionManagementHelper::getBeatmapSetHashesForSongButton(songButton, m_db);
            for(auto hash : beatmapSetHashes) {
                collection->add_map(hash);
            }
            save_collections();
            updateUIScheduled = true;
        } else if(id == 3) {
            // remove diff from collection

            // get collection name by selection
            std::string collectionName;
            {
                for(size_t i = 0; i < m_collectionButtons.size(); i++) {
                    if(m_collectionButtons[i]->isSelected()) {
                        collectionName = m_collectionButtons[i]->getCollectionName();
                        break;
                    }
                }
            }

            auto collection = get_or_create_collection(collectionName);
            collection->remove_map(songButton->getDatabaseBeatmap()->getMD5Hash());
            save_collections();
            updateUIScheduled = true;
        } else if(id == 4) {
            // remove entire set from collection

            // get collection name by selection
            std::string collectionName;
            {
                for(size_t i = 0; i < m_collectionButtons.size(); i++) {
                    if(m_collectionButtons[i]->isSelected()) {
                        collectionName = m_collectionButtons[i]->getCollectionName();
                        break;
                    }
                }
            }

            auto collection = get_or_create_collection(collectionName);
            const std::vector<MD5Hash> beatmapSetHashes =
                CollectionManagementHelper::getBeatmapSetHashesForSongButton(songButton, m_db);
            for(auto hash : beatmapSetHashes) {
                collection->remove_map(hash);
            }
            save_collections();
            updateUIScheduled = true;
        } else if(id == -2 || id == -4) {
            // add beatmap(set) to new collection
            std::string name = text.toUtf8();
            auto collection = get_or_create_collection(name);

            if(id == -2) {
                // id == -2 means beatmap
                collection->add_map(songButton->getDatabaseBeatmap()->getMD5Hash());
                updateUIScheduled = true;
            } else if(id == -4) {
                // id == -4 means beatmapset
                const std::vector<MD5Hash> beatmapSetHashes =
                    CollectionManagementHelper::getBeatmapSetHashesForSongButton(songButton, m_db);
                for(size_t i = 0; i < beatmapSetHashes.size(); i++) {
                    collection->add_map(beatmapSetHashes[i]);
                }
                updateUIScheduled = true;
            }

            save_collections();
        }
    }

    if(updateUIScheduled) {
        const float prevScrollPosY = m_songBrowser->getRelPosY();  // usability
        const auto previouslySelectedCollectionName =
            (m_selectionPreviousCollectionButton != NULL ? m_selectionPreviousCollectionButton->getCollectionName()
                                                         : "");  // usability
        {
            recreateCollectionsButtons();
            rebuildSongButtonsAndVisibleSongButtonsWithSearchMatchSupport(
                false, false);  // (last false = skipping rebuildSongButtons() here)
            onSortChangeInt(cv_songbrowser_sortingtype.getString(),
                            false);  // (because this does the rebuildSongButtons())
        }
        if(previouslySelectedCollectionName.length() > 0) {
            for(size_t i = 0; i < m_collectionButtons.size(); i++) {
                if(m_collectionButtons[i]->getCollectionName() == previouslySelectedCollectionName) {
                    m_collectionButtons[i]->select();
                    m_songBrowser->scrollToY(prevScrollPosY, false);
                    break;
                }
            }
        }
    }
}

void SongBrowser::onCollectionButtonContextMenu(CollectionButton *collectionButton, UString text, int id) {
    std::string collection_name = text.toUtf8();

    if(id == 2) {  // delete collection
        for(size_t i = 0; i < m_collectionButtons.size(); i++) {
            if(m_collectionButtons[i]->getCollectionName() == collection_name) {
                // delete UI
                delete m_collectionButtons[i];
                m_collectionButtons.erase(m_collectionButtons.begin() + i);

                // reset UI state
                m_selectionPreviousCollectionButton = NULL;

                auto collection = get_or_create_collection(collection_name);
                collection->delete_collection();
                save_collections();

                // update UI
                onGroupCollections(false);

                break;
            }
        }
    } else if(id == 3) {  // collection has been renamed
        // update UI
        onSortChangeInt(cv_songbrowser_sortingtype.getString(), false);
    }
}

void SongBrowser::highlightScore(u64 unixTimestamp) {
    for(size_t i = 0; i < m_scoreButtonCache.size(); i++) {
        if(m_scoreButtonCache[i]->getScore().unixTimestamp == unixTimestamp) {
            m_scoreBrowser->scrollToElement(m_scoreButtonCache[i], 0, 10);
            m_scoreButtonCache[i]->highlight();
            break;
        }
    }
}

void SongBrowser::recalculateStarsForSelectedBeatmap(bool force) {
    if(m_selectedBeatmap->getSelectedDifficulty2() == NULL) return;

    // TODO @kiwec
}

void SongBrowser::selectSongButton(Button *songButton) {
    if(songButton != NULL && !songButton->isSelected()) {
        m_contextMenu->setVisible2(false);
        songButton->select();
    }
}

void SongBrowser::selectRandomBeatmap() {
    // filter songbuttons or independent diffs
    const std::vector<CBaseUIElement *> &elements = m_songBrowser->getContainer()->getElements();
    std::vector<SongButton *> songButtons;
    for(size_t i = 0; i < elements.size(); i++) {
        SongButton *songButtonPointer = dynamic_cast<SongButton *>(elements[i]);
        SongDifficultyButton *songDifficultyButtonPointer = dynamic_cast<SongDifficultyButton *>(elements[i]);

        if(songButtonPointer != NULL &&
           (songDifficultyButtonPointer == NULL ||
            songDifficultyButtonPointer->isIndependentDiffButton()))  // only allow songbuttons or independent diffs
            songButtons.push_back(songButtonPointer);
    }

    if(songButtons.size() < 1) return;

    // remember previous
    if(m_previousRandomBeatmaps.size() == 0 && m_selectedBeatmap != NULL &&
       m_selectedBeatmap->getSelectedDifficulty2() != NULL)
        m_previousRandomBeatmaps.push_back(m_selectedBeatmap->getSelectedDifficulty2());

    size_t rng;
#ifdef _WIN32
    HCRYPTPROV hCryptProv;
    CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    CryptGenRandom(hCryptProv, sizeof(rng), (BYTE *)&rng);
    CryptReleaseContext(hCryptProv, 0);
#else
    getrandom(&rng, sizeof(rng), 0);
#endif
    size_t randomindex = rng % songButtons.size();

    SongButton *songButton = dynamic_cast<SongButton *>(songButtons[randomindex]);
    selectSongButton(songButton);
}

void SongBrowser::selectPreviousRandomBeatmap() {
    if(m_previousRandomBeatmaps.size() > 0) {
        DatabaseBeatmap *currentRandomBeatmap = m_previousRandomBeatmaps.back();
        if(m_previousRandomBeatmaps.size() > 1 && m_selectedBeatmap != NULL &&
           m_previousRandomBeatmaps[m_previousRandomBeatmaps.size() - 1] == m_selectedBeatmap->getSelectedDifficulty2())
            m_previousRandomBeatmaps.pop_back();  // deletes the current beatmap which may also be at the top (so we
                                                  // don't switch to ourself)

        // filter songbuttons
        const std::vector<CBaseUIElement *> &elements = m_songBrowser->getContainer()->getElements();
        std::vector<SongButton *> songButtons;
        for(size_t i = 0; i < elements.size(); i++) {
            SongButton *songButtonPointer = dynamic_cast<SongButton *>(elements[i]);

            if(songButtonPointer != NULL)  // allow ALL songbuttons
                songButtons.push_back(songButtonPointer);
        }

        // select it, if we can find it (and remove it from memory)
        bool foundIt = false;
        const DatabaseBeatmap *previousRandomBeatmap = m_previousRandomBeatmaps.back();
        for(size_t i = 0; i < songButtons.size(); i++) {
            if(songButtons[i]->getDatabaseBeatmap() != NULL &&
               songButtons[i]->getDatabaseBeatmap() == previousRandomBeatmap) {
                m_previousRandomBeatmaps.pop_back();
                selectSongButton(songButtons[i]);
                foundIt = true;
                break;
            }

            const auto &children = songButtons[i]->getChildren();
            for(size_t c = 0; c < children.size(); c++) {
                if(children[c]->getDatabaseBeatmap() == previousRandomBeatmap) {
                    m_previousRandomBeatmaps.pop_back();
                    selectSongButton(children[c]);
                    foundIt = true;
                    break;
                }
            }

            if(foundIt) break;
        }

        // if we didn't find it then restore the current random beatmap, which got pop_back()'d above (shit logic)
        if(!foundIt) m_previousRandomBeatmaps.push_back(currentRandomBeatmap);
    }
}

void SongBrowser::playSelectedDifficulty() {
    const std::vector<CBaseUIElement *> &elements = m_songBrowser->getContainer()->getElements();
    for(size_t i = 0; i < elements.size(); i++) {
        SongDifficultyButton *songDifficultyButton = dynamic_cast<SongDifficultyButton *>(elements[i]);
        if(songDifficultyButton != NULL && songDifficultyButton->isSelected()) {
            songDifficultyButton->select();
            break;
        }
    }
}

void SongBrowser::recreateCollectionsButtons() {
    // reset
    {
        m_selectionPreviousCollectionButton = NULL;
        for(size_t i = 0; i < m_collectionButtons.size(); i++) {
            delete m_collectionButtons[i];
        }
        m_collectionButtons.clear();

        // sanity
        if(m_group == GROUP::GROUP_COLLECTIONS) {
            m_songBrowser->getContainer()->empty();
            m_visibleSongButtons.clear();
        }
    }

    Timer t;
    t.start();

    for(auto collection : collections) {
        if(collection->maps.empty()) continue;

        std::vector<SongButton *> folder;
        std::vector<u32> matched_sets;

        for(auto &map : collection->maps) {
            auto it = hashToSongButton.find(map);
            if(it == hashToSongButton.end()) continue;

            u32 set_id = 0;
            auto song_button = it->second;
            const auto &songButtonChildren = song_button->getChildren();
            std::vector<SongButton *> matching_diffs;
            for(SongButton *sbc : songButtonChildren) {
                for(auto &map2 : collection->maps) {
                    if(sbc->getDatabaseBeatmap()->getMD5Hash() == map2) {
                        matching_diffs.push_back(sbc);
                        set_id = sbc->getDatabaseBeatmap()->getSetID();
                    }
                }
            }

            auto set = std::find(matched_sets.begin(), matched_sets.end(), set_id);
            if(set == matched_sets.end()) {
                // Mark set as processed so we don't add the diffs from the same set twice
                matched_sets.push_back(set_id);
            } else {
                // We already added the maps from this set to the collection!
                continue;
            }

            if(songButtonChildren.size() == matching_diffs.size()) {
                // all diffs match: add the set button (user added all diffs of beatmap into collection)
                folder.push_back(song_button);
            } else {
                // only add matched diff buttons
                folder.insert(folder.end(), matching_diffs.begin(), matching_diffs.end());
            }
        }

        if(!folder.empty()) {
            UString uname = collection->name.c_str();
            m_collectionButtons.push_back(new CollectionButton(
                this, m_songBrowser, m_contextMenu, 250, 250 + m_beatmaps.size() * 50, 200, 50, "", uname, folder));
        }
    }

    t.update();
    debugLog("recreateCollectionsButtons(): %f seconds\n", t.getElapsedTime());
}
