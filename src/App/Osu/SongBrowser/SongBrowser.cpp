#include "BaseEnvironment.h"

#include "AnimationHandler.h"
#include "BackgroundImageHandler.h"
#include "Bancho.h"
#include "BanchoLeaderboard.h"
#include "BanchoNetworking.h"
#include "Beatmap.h"
#include "BottomBar.h"
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
#include "Timing.h"
#include "UIBackButton.h"
#include "UIContextMenu.h"
#include "UISearchOverlay.h"
#include "UserCard.h"
#include "VertexArrayObject.h"

#include <chrono>
#include <utility>

const Color highlightColor = argb(255, 0, 255, 0);
const Color defaultColor = argb(255, 255, 255, 255);

class SongBrowserBackgroundSearchMatcher : public Resource {
   public:
    SongBrowserBackgroundSearchMatcher() : Resource() {
        this->bDead = true;  // NOTE: start dead! need to revive() before use
    }

    [[nodiscard]] bool isDead() const { return this->bDead.load(); }
    void kill() { this->bDead = true; }
    void revive() { this->bDead = false; }

    void setSongButtonsAndSearchString(const std::vector<SongButton *> &songButtons, const UString &searchString,
                                       const UString &hardcodedSearchString) {
        this->songButtons = songButtons;

        this->sSearchString.clear();
        if(hardcodedSearchString.length() > 0) {
            this->sSearchString.append(hardcodedSearchString);
            this->sSearchString.append(" ");
        }
        this->sSearchString.append(searchString);
        this->sHardcodedSearchString = hardcodedSearchString;
    }

    [[nodiscard]] Type getResType() const override { return APPDEFINED; }  // TODO: handle this better?

   protected:
    void init() override { this->bReady = true; }

    void initAsync() override {
        if(this->bDead.load()) {
            this->bAsyncReady = true;
            return;
        }

        // flag matches across entire database
        const std::vector<UString> searchStringTokens = this->sSearchString.split(" ");
        for(size_t i = 0; i < this->songButtons.size(); i++) {
            const auto &children = this->songButtons[i]->getChildren();
            if(children.size() > 0) {
                for(size_t c = 0; c < children.size(); c++) {
                    children[c]->setIsSearchMatch(
                        SongBrowser::searchMatcher(children[c]->getDatabaseBeatmap(), searchStringTokens));
                }
            } else
                this->songButtons[i]->setIsSearchMatch(
                    SongBrowser::searchMatcher(this->songButtons[i]->getDatabaseBeatmap(), searchStringTokens));

            // cancellation point
            if(this->bDead.load()) break;
        }

        this->bAsyncReady = true;
    }

    void destroy() override { ; }

   private:
    std::atomic<bool> bDead;

    UString sSearchString;
    UString sHardcodedSearchString;
    std::vector<SongButton *> songButtons;
};

class ScoresStillLoadingElement : public CBaseUILabel {
   public:
    ScoresStillLoadingElement(const UString& text) : CBaseUILabel(0, 0, 0, 0, "", text) {
        this->sIconString.insert(0, Icons::GLOBE);
    }

    void drawText() override {
        // draw icon
        const float iconScale = 0.6f;
        McFont *iconFont = osu->getFontIcons();
        int iconWidth = 0;
        g->pushTransform();
        {
            const float scale = (this->vSize.y / iconFont->getHeight()) * iconScale;
            const float paddingLeft = scale * 15;

            iconWidth = paddingLeft + iconFont->getStringWidth(this->sIconString) * scale;

            g->scale(scale, scale);
            g->translate((int)(this->vPos.x + paddingLeft),
                         (int)(this->vPos.y + this->vSize.y / 2 + iconFont->getHeight() * scale / 2));
            g->setColor(0xffffffff);
            g->drawString(iconFont, this->sIconString);
        }
        g->popTransform();

        // draw text
        const float textScale = 0.4f;
        McFont *textFont = osu->getSongBrowserFont();
        g->pushTransform();
        {
            const float stringWidth = textFont->getStringWidth(this->sText);

            const float scale = ((this->vSize.x - iconWidth) / stringWidth) * textScale;

            g->scale(scale, scale);
            g->translate((int)(this->vPos.x + iconWidth + (this->vSize.x - iconWidth) / 2 - stringWidth * scale / 2),
                         (int)(this->vPos.y + this->vSize.y / 2 + textFont->getHeight() * scale / 2));
            g->setColor(0xff02c3e5);
            g->drawString(textFont, this->sText);
        }
        g->popTransform();
    }

   private:
    UString sIconString;
};

class NoRecordsSetElement : public CBaseUILabel {
   public:
    NoRecordsSetElement(const UString& text) : CBaseUILabel(0, 0, 0, 0, "", text) {
        this->sIconString.insert(0, Icons::TROPHY);
    }

    void drawText() override {
        // draw icon
        const float iconScale = 0.6f;
        McFont *iconFont = osu->getFontIcons();
        int iconWidth = 0;
        g->pushTransform();
        {
            const float scale = (this->vSize.y / iconFont->getHeight()) * iconScale;
            const float paddingLeft = scale * 15;

            iconWidth = paddingLeft + iconFont->getStringWidth(this->sIconString) * scale;

            g->scale(scale, scale);
            g->translate((int)(this->vPos.x + paddingLeft),
                         (int)(this->vPos.y + this->vSize.y / 2 + iconFont->getHeight() * scale / 2));
            g->setColor(0xffffffff);
            g->drawString(iconFont, this->sIconString);
        }
        g->popTransform();

        // draw text
        const float textScale = 0.6f;
        McFont *textFont = osu->getSongBrowserFont();
        g->pushTransform();
        {
            const float stringWidth = textFont->getStringWidth(this->sText);

            const float scale = ((this->vSize.x - iconWidth) / stringWidth) * textScale;

            g->scale(scale, scale);
            g->translate((int)(this->vPos.x + iconWidth + (this->vSize.x - iconWidth) / 2 - stringWidth * scale / 2),
                         (int)(this->vPos.y + this->vSize.y / 2 + textFont->getHeight() * scale / 2));
            g->setColor(0xff02c3e5);
            g->drawString(textFont, this->sText);
        }
        g->popTransform();
    }

   private:
    UString sIconString;
};

bool sort_by_artist(SongButton const *a, SongButton const *b) {
    if(a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL) return a < b;

    UString artistA{a->getDatabaseBeatmap()->getCreator()};
    UString artistB{b->getDatabaseBeatmap()->getCreator()};

    return artistA.lessThanIgnoreCaseStrict(artistB);
}

bool sort_by_bpm(SongButton const *a, SongButton const *b) {
    if(a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL) return a < b;

    int bpm1 = a->getDatabaseBeatmap()->getMostCommonBPM();
    int bpm2 = b->getDatabaseBeatmap()->getMostCommonBPM();
    if(bpm1 == bpm2) return a < b;
    return bpm1 < bpm2;
}

bool sort_by_creator(SongButton const *a, SongButton const *b) {
    if(a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL) return a < b;

    UString creatorA{a->getDatabaseBeatmap()->getCreator()};
    UString creatorB{b->getDatabaseBeatmap()->getCreator()};

    return creatorA.lessThanIgnoreCaseStrict(creatorB);
}

bool sort_by_date_added(SongButton const *a, SongButton const *b) {
    if(a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL) return a < b;

    long long time1 = a->getDatabaseBeatmap()->last_modification_time;
    long long time2 = b->getDatabaseBeatmap()->last_modification_time;
    if(time1 == time2) return a > b;
    return time1 > time2;
}

bool sort_by_difficulty(SongButton const *a, SongButton const *b) {
    if(a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL) return a < b;

    float stars1 = a->getDatabaseBeatmap()->getStarsNomod();
    float stars2 = b->getDatabaseBeatmap()->getStarsNomod();
    if(stars1 != stars2) return stars1 < stars2;

    float diff1 = (a->getDatabaseBeatmap()->getAR() + 1) * (a->getDatabaseBeatmap()->getCS() + 1) *
                  (a->getDatabaseBeatmap()->getHP() + 1) * (a->getDatabaseBeatmap()->getOD() + 1) *
                  (std::max(a->getDatabaseBeatmap()->getMostCommonBPM(), 1));
    float diff2 = (b->getDatabaseBeatmap()->getAR() + 1) * (b->getDatabaseBeatmap()->getCS() + 1) *
                  (b->getDatabaseBeatmap()->getHP() + 1) * (b->getDatabaseBeatmap()->getOD() + 1) *
                  (std::max(b->getDatabaseBeatmap()->getMostCommonBPM(), 1));
    if(diff1 != diff2) return diff1 < diff2;

    return a < b;
}

bool sort_by_length(SongButton const *a, SongButton const *b) {
    if(a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL) return a < b;

    unsigned long length1 = a->getDatabaseBeatmap()->getLengthMS();
    unsigned long length2 = b->getDatabaseBeatmap()->getLengthMS();
    if(length1 == length2) return a < b;
    return length1 < length2;
}

bool sort_by_title(SongButton const *a, SongButton const *b) {
    if(a->getDatabaseBeatmap() == NULL || b->getDatabaseBeatmap() == NULL) return a < b;

    UString titleA{a->getDatabaseBeatmap()->getCreator()};
    UString titleB{b->getDatabaseBeatmap()->getCreator()};

    return titleA.lessThanIgnoreCaseStrict(titleB);
}

bool sort_by_grade(SongButton const *a, SongButton const *b) {
    if(a->grade == b->grade) return a < b;
    return a->grade < b->grade;
}

SongBrowser::SongBrowser() : ScreenBackable() {
    // random selection algorithm init
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    this->rngalg = std::mt19937(static_cast<std::mt19937::result_type>(seed));

    // sorting/grouping + methods
    this->group = GROUP::GROUP_NO_GROUPING;
    {
        this->groupings.push_back({GROUP::GROUP_NO_GROUPING, "No Grouping", 0});
        this->groupings.push_back({GROUP::GROUP_ARTIST, "By Artist", 1});
        this->groupings.push_back({GROUP::GROUP_BPM, "By BPM", 2});
        this->groupings.push_back({GROUP::GROUP_CREATOR, "By Creator", 3});
        /// m_groupings.push_back({GROUP::GROUP_DATEADDED, "By Date Added", 4}); // not yet possible
        this->groupings.push_back({GROUP::GROUP_DIFFICULTY, "By Difficulty", 5});
        this->groupings.push_back({GROUP::GROUP_LENGTH, "By Length", 6});
        this->groupings.push_back({GROUP::GROUP_TITLE, "By Title", 7});
        this->groupings.push_back({GROUP::GROUP_COLLECTIONS, "Collections", 8});
    }

    this->sortingMethod = SORT::SORT_ARTIST;
    this->sortingComparator = sort_by_artist;

    this->sortingMethods.push_back({SORT::SORT_ARTIST, "By Artist", sort_by_artist});
    this->sortingMethods.push_back({SORT::SORT_BPM, "By BPM", sort_by_bpm});
    this->sortingMethods.push_back({SORT::SORT_CREATOR, "By Creator", sort_by_creator});
    this->sortingMethods.push_back({SORT::SORT_DATEADDED, "By Date Added", sort_by_date_added});
    this->sortingMethods.push_back({SORT::SORT_DIFFICULTY, "By Difficulty", sort_by_difficulty});
    this->sortingMethods.push_back({SORT::SORT_LENGTH, "By Length", sort_by_length});
    this->sortingMethods.push_back({SORT::SORT_TITLE, "By Title", sort_by_title});
    this->sortingMethods.push_back({SORT::SORT_RANKACHIEVED, "By Rank Achieved", sort_by_grade});

    // vars
    this->bSongBrowserRightClickScrollCheck = false;
    this->bSongBrowserRightClickScrolling = false;
    this->bNextScrollToSongButtonJumpFixScheduled = false;
    this->bNextScrollToSongButtonJumpFixUseScrollSizeDelta = false;
    this->fNextScrollToSongButtonJumpFixOldRelPosY = 0.0f;

    this->selectionPreviousSongButton = NULL;
    this->selectionPreviousSongDiffButton = NULL;
    this->selectionPreviousCollectionButton = NULL;

    this->bF1Pressed = false;
    this->bF2Pressed = false;
    this->bF3Pressed = false;
    this->bShiftPressed = false;
    this->bLeft = false;
    this->bRight = false;

    this->bRandomBeatmapScheduled = false;
    this->bPreviousRandomBeatmapScheduled = false;

    // build topbar left
    this->topbarLeft = new CBaseUIContainer(0, 0, 0, 0, "");
    this->songInfo = new InfoLabel(0, 0, 0, 0, "");
    this->topbarLeft->addBaseUIElement(this->songInfo);

    this->scoreSortButton = new CBaseUIButton(0, 0, 0, 0, "", "Sort by score");
    this->scoreSortButton->setDrawBackground(false);
    this->scoreSortButton->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onSortScoresClicked));
    this->topbarLeft->addBaseUIElement(this->scoreSortButton);

    this->webButton = new CBaseUIButton(0, 0, 0, 0, "", "Web");
    this->webButton->setDrawBackground(false);
    this->webButton->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onWebClicked));
    this->topbarLeft->addBaseUIElement(this->webButton);

    // build topbar right
    this->topbarRight = new CBaseUIContainer(0, 0, 0, 0, "");
    {
        this->groupLabel = new CBaseUILabel(0, 0, 0, 0, "", "Group:");
        this->groupLabel->setSizeToContent(3);
        this->groupLabel->setDrawFrame(false);
        this->groupLabel->setDrawBackground(false);
        this->groupLabel->grabs_clicks = true;
        this->topbarRight->addBaseUIElement(this->groupLabel);

        this->groupButton = new CBaseUIButton(0, 0, 0, 0, "", "No Grouping");
        this->groupButton->setDrawBackground(false);
        this->groupButton->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onGroupClicked));
        this->groupButton->grabs_clicks = true;
        this->topbarRight->addBaseUIElement(this->groupButton);

        this->sortLabel = new CBaseUILabel(0, 0, 0, 0, "", "Sort:");
        this->sortLabel->setSizeToContent(3);
        this->sortLabel->setDrawFrame(false);
        this->sortLabel->setDrawBackground(false);
        this->sortLabel->grabs_clicks = true;
        this->topbarRight->addBaseUIElement(this->sortLabel);

        this->sortButton = new CBaseUIButton(0, 0, 0, 0, "", "By Date Added");
        this->sortButton->setDrawBackground(false);
        this->sortButton->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onSortClicked));
        this->sortButton->grabs_clicks = true;
        this->topbarRight->addBaseUIElement(this->sortButton);

        // "hardcoded" grouping tabs
        this->groupByCollectionBtn = new CBaseUIButton(0, 0, 0, 0, "", "Collections");
        this->groupByCollectionBtn->setDrawBackground(false);
        this->groupByCollectionBtn->setClickCallback(
            fastdelegate::MakeDelegate(this, &SongBrowser::onQuickGroupClicked));
        this->groupByCollectionBtn->grabs_clicks = true;
        this->topbarRight->addBaseUIElement(this->groupByCollectionBtn);
        this->groupByArtistBtn = new CBaseUIButton(0, 0, 0, 0, "", "By Artist");
        this->groupByArtistBtn->setDrawBackground(false);
        this->groupByArtistBtn->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onQuickGroupClicked));
        this->groupByArtistBtn->grabs_clicks = true;
        this->topbarRight->addBaseUIElement(this->groupByArtistBtn);
        this->groupByDifficultyBtn = new CBaseUIButton(0, 0, 0, 0, "", "By Difficulty");
        this->groupByDifficultyBtn->setDrawBackground(false);
        this->groupByDifficultyBtn->setClickCallback(
            fastdelegate::MakeDelegate(this, &SongBrowser::onQuickGroupClicked));
        this->groupByDifficultyBtn->grabs_clicks = true;
        this->topbarRight->addBaseUIElement(this->groupByDifficultyBtn);
        this->groupByNothingBtn = new CBaseUIButton(0, 0, 0, 0, "", "No Grouping");
        this->groupByNothingBtn->setDrawBackground(false);
        this->groupByNothingBtn->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onQuickGroupClicked));
        this->groupByNothingBtn->grabs_clicks = true;
        this->topbarRight->addBaseUIElement(this->groupByNothingBtn);
    }

    // context menu
    this->contextMenu = new UIContextMenu(50, 50, 150, 0, "");
    this->contextMenu->setVisible(true);

    // build scorebrowser
    this->scoreBrowser = new CBaseUIScrollView(0, 0, 0, 0, "");
    this->scoreBrowser->bScrollbarOnLeft = true;
    this->scoreBrowser->setDrawBackground(false);
    this->scoreBrowser->setDrawFrame(false);
    this->scoreBrowser->setHorizontalScrolling(false);
    this->scoreBrowser->setScrollbarSizeMultiplier(0.25f);
    this->scoreBrowser->setScrollResistance(15);
    this->scoreBrowser->bHorizontalClipping = false;
    this->scoreBrowserScoresStillLoadingElement = new ScoresStillLoadingElement("Loading...");
    this->scoreBrowserNoRecordsYetElement = new NoRecordsSetElement("No records set!");
    this->scoreBrowser->getContainer()->addBaseUIElement(this->scoreBrowserNoRecordsYetElement);

    // NOTE: we don't add localBestContainer to the screen; we draw and update it manually so that
    //       it can be drawn under skins which overlay the scores list.
    this->localBestContainer = new CBaseUIContainer(0, 0, 0, 0, "");
    this->localBestContainer->setVisible(false);
    this->localBestLabel = new CBaseUILabel(0, 0, 0, 0, "", "Personal Best (from local scores)");
    this->localBestLabel->setDrawBackground(false);
    this->localBestLabel->setDrawFrame(false);
    this->localBestLabel->setTextJustification(CBaseUILabel::TEXT_JUSTIFICATION::TEXT_JUSTIFICATION_CENTERED);

    // build carousel
    this->carousel = new CBaseUIScrollView(0, 0, 0, 0, "");
    this->carousel->setDrawBackground(false);
    this->carousel->setDrawFrame(false);
    this->carousel->setHorizontalScrolling(false);
    this->carousel->setScrollResistance(15);
    this->thumbnailYRatio = cv::draw_songbrowser_thumbnails.getBool() ? 1.333333f : 0.f;

    // beatmap database
    db = new Database();
    this->bBeatmapRefreshScheduled = true;

    // behaviour
    this->bHasSelectedAndIsPlaying = false;
    this->fPulseAnimation = 0.0f;
    this->fBackgroundFadeInTime = 0.0f;

    // search
    this->search = new UISearchOverlay(0, 0, 0, 0, "");
    this->search->setOffsetRight(10);
    this->fSearchWaitTime = 0.0f;
    this->bInSearch = (cv::songbrowser_search_hardcoded_filter.getString().length() > 0);
    this->searchPrevGroup = GROUP::GROUP_NO_GROUPING;
    this->backgroundSearchMatcher = new SongBrowserBackgroundSearchMatcher();

    this->beatmap = new Beatmap();

    this->updateLayout();
}

SongBrowser::~SongBrowser() {
    sct_abort();
    lct_set_map(NULL);
    VolNormalization::abort();
    MapCalcThread::abort();
    this->checkHandleKillBackgroundSearchMatcher();

    resourceManager->destroyResource(this->backgroundSearchMatcher);

    this->carousel->getContainer()->empty();

    for(size_t i = 0; i < this->songButtons.size(); i++) {
        delete this->songButtons[i];
    }
    for(size_t i = 0; i < this->collectionButtons.size(); i++) {
        delete this->collectionButtons[i];
    }
    for(size_t i = 0; i < this->artistCollectionButtons.size(); i++) {
        delete this->artistCollectionButtons[i];
    }
    for(size_t i = 0; i < this->bpmCollectionButtons.size(); i++) {
        delete this->bpmCollectionButtons[i];
    }
    for(size_t i = 0; i < this->difficultyCollectionButtons.size(); i++) {
        delete this->difficultyCollectionButtons[i];
    }
    for(size_t i = 0; i < this->creatorCollectionButtons.size(); i++) {
        delete this->creatorCollectionButtons[i];
    }
    for(size_t i = 0; i < this->dateaddedCollectionButtons.size(); i++) {
        delete this->dateaddedCollectionButtons[i];
    }
    for(size_t i = 0; i < this->lengthCollectionButtons.size(); i++) {
        delete this->lengthCollectionButtons[i];
    }
    for(size_t i = 0; i < this->titleCollectionButtons.size(); i++) {
        delete this->titleCollectionButtons[i];
    }

    this->scoreBrowser->getContainer()->empty();
    for(ScoreButton* button : this->scoreButtonCache) {
        SAFE_DELETE(button);
    }
    this->scoreButtonCache.clear();

    SAFE_DELETE(this->localBestButton);
    SAFE_DELETE(this->scoreBrowserScoresStillLoadingElement);
    SAFE_DELETE(this->scoreBrowserNoRecordsYetElement);

    SAFE_DELETE(this->beatmap);
    SAFE_DELETE(this->search);
    SAFE_DELETE(this->topbarLeft);
    SAFE_DELETE(this->topbarRight);
    SAFE_DELETE(this->scoreBrowser);
    SAFE_DELETE(this->carousel);
    SAFE_DELETE(db);

    // Memory leak on shutdown, maybe
    this->empty();
}

void SongBrowser::draw() {
    if(!this->bVisible) return;

    // draw background
    g->setColor(0xff000000);
    g->fillRect(0, 0, osu->getScreenWidth(), osu->getScreenHeight());

    // refreshing (blocks every other call in draw() below it!)
    if(this->bBeatmapRefreshScheduled) {
        UString loadingMessage = UString::format("Loading beatmaps ... (%i %%)", (int)(db->getProgress() * 100.0f));

        g->setColor(0xffffffff);
        g->pushTransform();
        {
            g->translate((int)(osu->getScreenWidth() / 2 - osu->getSubTitleFont()->getStringWidth(loadingMessage) / 2),
                         osu->getScreenHeight() - 15);
            g->drawString(osu->getSubTitleFont(), loadingMessage);
        }
        g->popTransform();

        osu->getHUD()->drawBeatmapImportSpinner();
        return;
    }

    // draw background image
    if(cv::draw_songbrowser_background_image.getBool()) {
        float alpha = 1.0f;
        if(cv::songbrowser_background_fade_in_duration.getFloat() > 0.0f) {
            // handle fadein trigger after handler is finished loading
            const bool ready = osu->getSelectedBeatmap()->getSelectedDifficulty2() != NULL &&
                               osu->getBackgroundImageHandler()->getLoadBackgroundImage(
                                   osu->getSelectedBeatmap()->getSelectedDifficulty2()) != NULL &&
                               osu->getBackgroundImageHandler()
                                   ->getLoadBackgroundImage(osu->getSelectedBeatmap()->getSelectedDifficulty2())
                                   ->isReady();

            if(!ready)
                this->fBackgroundFadeInTime = engine->getTime();
            else if(this->fBackgroundFadeInTime > 0.0f && engine->getTime() > this->fBackgroundFadeInTime) {
                alpha = std::clamp<float>((engine->getTime() - this->fBackgroundFadeInTime) /
                                              cv::songbrowser_background_fade_in_duration.getFloat(),
                                          0.0f, 1.0f);
                alpha = 1.0f - (1.0f - alpha) * (1.0f - alpha);
            }
        }

        drawSelectedBeatmapBackgroundImage(alpha);
    } else if(cv::draw_songbrowser_menu_background_image.getBool()) {
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

    {
        auto screen = osu->getScreenSize();
        bool is_widescreen = ((i32)(std::max(0, (i32)((screen.x - (screen.y * 4.f / 3.f)) / 2.f))) > 0);
        f32 global_scale = screen.x / (is_widescreen ? 1366.f : 1024.f);
        f32 mode_osu_scale = global_scale * (osu->getSkin()->mode_osu->is_2x ? 0.5f : 1.f);

        g->setColor(0xffffffff);
        if(cv::avoid_flashes.getBool()) {
            g->setAlpha(0.1f);
        } else {
            // XXX: Flash based on song BPM
            g->setAlpha(0.1f);
        }

        g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ADDITIVE);
        osu->getSkin()->mode_osu->drawRaw(Vector2(osu->getScreenWidth() / 2, osu->getScreenHeight() / 2),
                                          mode_osu_scale, AnchorPoint::CENTER);
        g->setBlendMode(Graphics::BLEND_MODE::BLEND_MODE_ALPHA);
    }

    // draw score browser
    this->scoreBrowser->draw();
    this->localBestContainer->draw();

    if(cv::debug.getBool()) {
        this->scoreBrowser->getContainer()->draw_debug();
    }

    // draw strain graph of currently selected beatmap
    if(cv::draw_songbrowser_strain_graph.getBool()) {
        const std::vector<double> &aimStrains = this->getSelectedBeatmap()->aimStrains;
        const std::vector<double> &speedStrains = this->getSelectedBeatmap()->speedStrains;
        const float speedMultiplier = this->getSelectedBeatmap()->getSpeedMultiplier();

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

                const float graphWidth = this->scoreBrowser->getSize().x;

                const float msPerPixel = (float)lengthMS / graphWidth;
                const float strainWidth = strainStepMS / msPerPixel;
                const float strainHeightMultiplier = cv::hud_scrubbing_timeline_strains_height.getFloat() * dpiScale;

                McRect graphRect(0, osu->getScreenHeight() - (get_bottombar_height() + strainHeightMultiplier),
                                 graphWidth, strainHeightMultiplier);

                const float alpha =
                    (graphRect.contains(mouse->getPos()) ? 1.0f : cv::hud_scrubbing_timeline_strains_alpha.getFloat());

                const Color aimStrainColor =
                    argb(alpha, cv::hud_scrubbing_timeline_strains_aim_color_r.getInt() / 255.0f,
                           cv::hud_scrubbing_timeline_strains_aim_color_g.getInt() / 255.0f,
                           cv::hud_scrubbing_timeline_strains_aim_color_b.getInt() / 255.0f);
                const Color speedStrainColor =
                    argb(alpha, cv::hud_scrubbing_timeline_strains_speed_color_r.getInt() / 255.0f,
                           cv::hud_scrubbing_timeline_strains_speed_color_g.getInt() / 255.0f,
                           cv::hud_scrubbing_timeline_strains_speed_color_b.getInt() / 255.0f);

                g->setDepthBuffer(true);
                for(int i = 0; i < aimStrains.size(); i++) {
                    const double aimStrain = (aimStrains[i]) / highestStrain;
                    const double speedStrain = (speedStrains[i]) / highestStrain;
                    // const double strain = (aimStrains[i] + speedStrains[i]) / highestStrain;

                    const double aimStrainHeight = aimStrain * strainHeightMultiplier;
                    const double speedStrainHeight = speedStrain * strainHeightMultiplier;
                    // const double strainHeight = strain * strainHeightMultiplier;

                    if(!keyboard->isShiftDown()) {
                        g->setColor(aimStrainColor);
                        g->fillRect(i * strainWidth,
                                    osu->getScreenHeight() - (get_bottombar_height() + aimStrainHeight),
                                    std::max(1.0f, std::round(strainWidth + 0.5f)), aimStrainHeight);
                    }

                    if(!keyboard->isControlDown()) {
                        g->setColor(speedStrainColor);
                        g->fillRect(i * strainWidth,
                                    osu->getScreenHeight() -
                                        (get_bottombar_height() +
                                         ((keyboard->isShiftDown() ? 0 : aimStrainHeight) - speedStrainHeight)),
                                    std::max(1.0f, std::round(strainWidth + 0.5f)), speedStrainHeight + 1);
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

                    Vector2 topLeftCenter = Vector2(
                        highestStrainIndex * strainWidth + strainWidth / 2.0f,
                        osu->getScreenHeight() - (get_bottombar_height() + aimStrainHeight + speedStrainHeight));

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
    this->carousel->draw();

    // draw search
    this->search->setSearchString(this->sSearchString, cv::songbrowser_search_hardcoded_filter.getString().c_str());
    this->search->setDrawNumResults(this->bInSearch);
    this->search->setNumFoundResults(this->visibleSongButtons.size());
    this->search->setSearching(!this->backgroundSearchMatcher->isDead());
    this->search->draw();

    // draw topbar background
    g->setColor(0xffffffff);
    g->pushTransform();
    {
        auto screen = osu->getScreenSize();
        bool is_widescreen = ((i32)(std::max(0, (i32)((screen.x - (screen.y * 4.f / 3.f)) / 2.f))) > 0);

        Image *topbar = osu->getSkin()->songSelectTop;
        f32 scale = (f32)osu->getScreenWidth() / (f32)topbar->getWidth();
        if(!is_widescreen) scale /= 0.75;

        g->scale(scale, scale);
        g->drawImage(topbar, AnchorPoint::TOP_LEFT);
    }
    g->popTransform();

    // draw bottom bar
    draw_bottombar();

    // draw top bar
    this->topbarLeft->draw();
    if(cv::debug.getBool()) this->topbarLeft->draw_debug();
    this->topbarRight->draw();
    if(cv::debug.getBool()) this->topbarRight->draw_debug();

    // NOTE: Intentionally not calling ScreenBackable::draw() here, since we're already drawing
    //       the back button in draw_bottombar().
    OsuScreen::draw();

    // no beatmaps found (osu folder is probably invalid)
    if(this->beatmaps.size() == 0 && !this->bBeatmapRefreshScheduled) {
        UString errorMessage1 = "Invalid osu! folder (or no beatmaps found): ";
        errorMessage1.append(this->sLastOsuFolder);
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
    this->contextMenu->draw();

    // click pulse animation overlay
    if(this->fPulseAnimation > 0.0f) {
        Color topColor = 0x00ffffff;
        Color bottomColor = argb((int)(25 * this->fPulseAnimation), 255, 255, 255);

        g->fillGradient(0, 0, osu->getScreenWidth(), osu->getScreenHeight(), topColor, topColor, bottomColor,
                        bottomColor);
    }
}

void SongBrowser::drawSelectedBeatmapBackgroundImage(float alpha) {
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
        osu->getNotificationOverlay()->addToast("Beatmapset has no difficulties :/");
        return false;
    } else {
        this->onSelectionChange(this->hashToSongButton[best_diff->getMD5Hash()], false);
        this->onDifficultySelected(best_diff, false);
        this->selectSelectedBeatmapSongButton();
        return true;
    }
}

void SongBrowser::mouse_update(bool *propagate_clicks) {
    if(!this->bVisible) return;

    this->localBestContainer->mouse_update(propagate_clicks);
    ScreenBackable::mouse_update(propagate_clicks);

    // NOTE: This is placed before update_bottombar(), otherwise the context menu would close
    //       on a bottombar selector click (yeah, a bit hacky)
    this->contextMenu->mouse_update(propagate_clicks);

    update_bottombar(propagate_clicks);

    // refresh logic (blocks every other call in the update() function below it!)
    if(this->bBeatmapRefreshScheduled) {
        // check if we are finished loading
        if(db->isFinished()) {
            this->bBeatmapRefreshScheduled = false;
            this->onDatabaseLoadingFinished();
        }
        return;
    }

    // map star/bpm/other calc
    if(MapCalcThread::is_finished()) {
        MapCalcThread::abort();  // join thread

        auto &maps = db->maps_to_recalc;

        const auto &results = MapCalcThread::get_results();

        db->peppy_overrides_mtx.lock();
        for(int i = 0; i < results.size(); i++) {
            auto diff = maps[i];
            auto res = results[i];
            diff->iNumCircles = res.nb_circles;
            diff->iNumSliders = res.nb_sliders;
            diff->iNumSpinners = res.nb_spinners;
            diff->fStarsNomod = res.star_rating;
            diff->iMinBPM = res.min_bpm;
            diff->iMaxBPM = res.max_bpm;
            diff->iMostCommonBPM = res.avg_bpm;
            db->peppy_overrides[diff->sMD5Hash] = diff->get_overrides();
        }
        db->peppy_overrides_mtx.unlock();

        maps.clear();
    }

    // selected mods pp calc
    auto diff2 = this->beatmap->getSelectedDifficulty2();
    lct_set_map(diff2);
    if(diff2 != NULL && diff2->pp.pp == -1.0) {
        auto mods = osu->getScore()->mods;

        pp_calc_request request;
        request.mods_legacy = mods.to_legacy();
        request.speed = mods.speed;
        request.AR = mods.get_naive_ar(diff2);
        request.OD = mods.get_naive_od(diff2);
        request.CS = diff2->getCS();
        if(mods.cs_override != -1.f) request.CS = mods.cs_override;
        request.rx = mods.flags & Replay::ModFlags::Relax;
        request.td = mods.flags & Replay::ModFlags::TouchDevice;
        request.comboMax = -1;
        request.numMisses = 0;
        request.num300s = diff2->getNumObjects();
        request.num100s = 0;
        request.num50s = 0;

        diff2->pp = lct_get_pp(request);
    }

    // auto-download
    if(this->map_autodl) {
        float progress = -1.f;
        auto beatmap = Downloader::download_beatmap(this->map_autodl, this->set_autodl, &progress);
        if(progress == -1.f) {
            auto error_str = UString::format("Failed to download Beatmap #%d :(", this->map_autodl);
            osu->getNotificationOverlay()->addToast(error_str);
            this->map_autodl = 0;
            this->set_autodl = 0;
        } else if(progress < 1.f) {
            // TODO @kiwec: this notification format is jank & laggy
            auto text = UString::format("Downloading... %.2f%%", progress * 100.f);
            osu->getNotificationOverlay()->addNotification(text);
        } else if(beatmap != NULL) {
            osu->songBrowser2->onDifficultySelected(beatmap, false);
            osu->songBrowser2->selectSelectedBeatmapSongButton();
            this->map_autodl = 0;
            this->set_autodl = 0;
        }
    } else if(this->set_autodl) {
        if(this->selectBeatmapset(this->set_autodl)) {
            this->map_autodl = 0;
            this->set_autodl = 0;
        } else {
            float progress = -1.f;
            Downloader::download_beatmapset(this->set_autodl, &progress);
            if(progress == -1.f) {
                auto error_str = UString::format("Failed to download Beatmapset #%d :(", this->set_autodl);
                osu->getNotificationOverlay()->addToast(error_str);
                this->map_autodl = 0;
                this->set_autodl = 0;
            } else if(progress < 1.f) {
                // TODO @kiwec: this notification format is jank & laggy
                auto text = UString::format("Downloading... %.2f%%", progress * 100.f);
                osu->getNotificationOverlay()->addNotification(text);
            } else {
                this->selectBeatmapset(this->set_autodl);

                this->map_autodl = 0;
                this->set_autodl = 0;
            }
        }
    }

    if(this->score_resort_scheduled) {
        this->rebuildScoreButtons();
        this->score_resort_scheduled = false;
    }

    // update and focus handling
    this->topbarRight->mouse_update(propagate_clicks);
    this->carousel->mouse_update(propagate_clicks);
    this->carousel->getContainer()->update_pos();  // necessary due to constant animations
    if(this->localBestButton) this->localBestButton->mouse_update(propagate_clicks);
    this->scoreBrowser->mouse_update(propagate_clicks);
    this->topbarLeft->mouse_update(propagate_clicks);

    // handle right click absolute scrolling
    {
        if(mouse->isRightDown() && !this->contextMenu->isMouseInside()) {
            if(!this->bSongBrowserRightClickScrollCheck) {
                this->bSongBrowserRightClickScrollCheck = true;

                bool isMouseInsideAnySongButton = false;
                {
                    const std::vector<CBaseUIElement *> &elements = this->carousel->getContainer()->getElements();
                    for(CBaseUIElement *songButton : elements) {
                        if(songButton->isMouseInside()) {
                            isMouseInsideAnySongButton = true;
                            break;
                        }
                    }
                }

                if(this->carousel->isMouseInside() && !osu->getOptionsMenu()->isMouseInside() &&
                   !isMouseInsideAnySongButton)
                    this->bSongBrowserRightClickScrolling = true;
                else
                    this->bSongBrowserRightClickScrolling = false;
            }
        } else {
            this->bSongBrowserRightClickScrollCheck = false;
            this->bSongBrowserRightClickScrolling = false;
        }

        if(this->bSongBrowserRightClickScrolling) {
            this->carousel->scrollToY(
                -((mouse->getPos().y - 2 - this->carousel->getPos().y) / this->carousel->getSize().y) *
                this->carousel->getScrollSize().y);
        }
    }

    // handle async random beatmap selection
    if(this->bRandomBeatmapScheduled) {
        this->bRandomBeatmapScheduled = false;
        this->selectRandomBeatmap();
    }
    if(this->bPreviousRandomBeatmapScheduled) {
        this->bPreviousRandomBeatmapScheduled = false;
        this->selectPreviousRandomBeatmap();
    }

    // if cursor is to the left edge of the screen, force center currently selected beatmap/diff
    // but only if the context menu is currently not visible (since we don't want move things while e.g. managing
    // collections etc.)
    if(mouse->getPos().x < osu->getScreenWidth() * 0.1f && !this->contextMenu->isVisible()) {
        this->scheduled_scroll_to_selected_button = true;
    }

    // handle searching
    if(this->fSearchWaitTime != 0.0f && engine->getTime() > this->fSearchWaitTime) {
        this->fSearchWaitTime = 0.0f;
        this->onSearchUpdate();
    }

    // handle background search matcher
    {
        if(!this->backgroundSearchMatcher->isDead() && this->backgroundSearchMatcher->isAsyncReady()) {
            // we have the results, now update the UI
            this->rebuildSongButtonsAndVisibleSongButtonsWithSearchMatchSupport(true);
            this->backgroundSearchMatcher->kill();
        }
        if(this->backgroundSearchMatcher->isDead()) {
            if(this->scheduled_scroll_to_selected_button) {
                this->scheduled_scroll_to_selected_button = false;
                this->scrollToBestButton();
            }
        }
    }
}

void SongBrowser::onKeyDown(KeyboardEvent &key) {
    OsuScreen::onKeyDown(key);  // only used for options menu
    if(!this->bVisible || key.isConsumed()) return;

    if(this->bVisible && this->bBeatmapRefreshScheduled &&
       (key == KEY_ESCAPE || key == (KEYCODE)cv::GAME_PAUSE.getInt())) {
        db->cancel();
        key.consume();
        return;
    }

    if(this->bBeatmapRefreshScheduled) return;

    // context menu
    this->contextMenu->onKeyDown(key);
    if(key.isConsumed()) return;

    // searching text delete & escape key handling
    if(this->sSearchString.length() > 0) {
        switch(key.getKeyCode()) {
            case KEY_DELETE:
            case KEY_BACKSPACE:
                key.consume();
                if(this->sSearchString.length() > 0) {
                    if(keyboard->isControlDown()) {
                        // delete everything from the current caret position to the left, until after the first
                        // non-space character (but including it)
                        bool foundNonSpaceChar = false;
                        while(this->sSearchString.length() > 0) {
                            UString curChar = this->sSearchString.substr(this->sSearchString.length() - 1, 1);

                            if(foundNonSpaceChar && curChar.isWhitespaceOnly()) break;

                            if(!curChar.isWhitespaceOnly()) foundNonSpaceChar = true;

                            this->sSearchString.erase(this->sSearchString.length() - 1, 1);
                        }
                    } else
                        this->sSearchString = this->sSearchString.substr(0, this->sSearchString.length() - 1);

                    this->scheduleSearchUpdate(this->sSearchString.length() == 0);
                }
                break;

            case KEY_ESCAPE:
                key.consume();
                this->sSearchString = "";
                this->scheduleSearchUpdate(true);
                break;
        }
    } else if(!this->contextMenu->isVisible()) {
        if(key == KEY_ESCAPE)  // can't support GAME_PAUSE hotkey here because of text searching
            osu->toggleSongBrowser();
    }

    // paste clipboard support
    if(key == KEY_V) {
        if(keyboard->isControlDown()) {
            const UString clipstring = env->getClipBoardText();
            if(clipstring.length() > 0) {
                this->sSearchString.append(clipstring);
                this->scheduleSearchUpdate(false);
            }
        }
    }

    if(key == KEY_LSHIFT || key == KEY_RSHIFT) this->bShiftPressed = true;

    // function hotkeys
    if((key == KEY_F1 || key == (KEYCODE)cv::TOGGLE_MODSELECT.getInt()) && !this->bF1Pressed) {
        this->bF1Pressed = true;
        press_bottombar_button(1);
    }
    if((key == KEY_F2 || key == (KEYCODE)cv::RANDOM_BEATMAP.getInt()) && !this->bF2Pressed) {
        this->bF2Pressed = true;
        press_bottombar_button(2);
    }
    if(key == KEY_F3 && !this->bF3Pressed) {
        this->bF3Pressed = true;
        press_bottombar_button(3);
    }

    if(key == KEY_F5) this->refreshBeatmaps();

    // selection move
    if(!keyboard->isAltDown() && key == KEY_DOWN) {
        const std::vector<CBaseUIElement *> &elements = this->carousel->getContainer()->getElements();

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

    if(!keyboard->isAltDown() && key == KEY_UP) {
        const std::vector<CBaseUIElement *> &elements = this->carousel->getContainer()->getElements();

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

    if(key == KEY_LEFT && !this->bLeft) {
        this->bLeft = true;

        const bool jumpToNextGroup = keyboard->isShiftDown();

        const std::vector<CBaseUIElement *> &elements = this->carousel->getContainer()->getElements();

        bool foundSelected = false;
        for(int i = elements.size() - 1; i >= 0; i--) {
            const SongDifficultyButton *diffButtonPointer = dynamic_cast<SongDifficultyButton *>(elements[i]);
            const CollectionButton *collectionButtonPointer = dynamic_cast<CollectionButton *>(elements[i]);

            Button *button = dynamic_cast<Button *>(elements[i]);
            const bool isSongDifficultyButtonAndNotIndependent =
                (diffButtonPointer != NULL && !diffButtonPointer->isIndependentDiffButton());

            if(foundSelected && button != NULL && !button->isSelected() && !isSongDifficultyButtonAndNotIndependent &&
               (!jumpToNextGroup || collectionButtonPointer != NULL)) {
                this->bNextScrollToSongButtonJumpFixUseScrollSizeDelta = true;
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
                this->bNextScrollToSongButtonJumpFixUseScrollSizeDelta = false;

                break;
            }

            if(button != NULL && button->isSelected()) foundSelected = true;
        }
    }

    if(key == KEY_RIGHT && !this->bRight) {
        this->bRight = true;

        const bool jumpToNextGroup = keyboard->isShiftDown();

        const std::vector<CBaseUIElement *> &elements = this->carousel->getContainer()->getElements();

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

    if(key == KEY_PAGEUP) this->carousel->scrollY(this->carousel->getSize().y);
    if(key == KEY_PAGEDOWN) this->carousel->scrollY(-this->carousel->getSize().y);

    // group open/close
    // NOTE: only closing works atm (no "focus" state on buttons yet)
    if((key == KEY_ENTER || key == KEY_NUMPAD_ENTER) && keyboard->isShiftDown()) {
        const std::vector<CBaseUIElement *> &elements = this->carousel->getContainer()->getElements();

        for(int i = 0; i < elements.size(); i++) {
            const CollectionButton *collectionButtonPointer = dynamic_cast<CollectionButton *>(elements[i]);

            Button *button = dynamic_cast<Button *>(elements[i]);

            if(collectionButtonPointer != NULL && button != NULL && button->isSelected()) {
                button->select();  // deselect
                this->scrollToSongButton(button);
                break;
            }
        }
    }

    // selection select
    if((key == KEY_ENTER || key == KEY_NUMPAD_ENTER) && !keyboard->isShiftDown()) this->playSelectedDifficulty();

    // toggle auto
    if(key == KEY_A && keyboard->isControlDown()) osu->getModSelector()->toggleAuto();

    key.consume();
}

void SongBrowser::onKeyUp(KeyboardEvent &key) {
    // context menu
    this->contextMenu->onKeyUp(key);
    if(key.isConsumed()) return;

    if(key == KEY_LSHIFT || key == KEY_RSHIFT) this->bShiftPressed = false;
    if(key == KEY_LEFT) this->bLeft = false;
    if(key == KEY_RIGHT) this->bRight = false;

    if(key == KEY_F1 || key == (KEYCODE)cv::TOGGLE_MODSELECT.getInt()) this->bF1Pressed = false;
    if(key == KEY_F2 || key == (KEYCODE)cv::RANDOM_BEATMAP.getInt()) this->bF2Pressed = false;
    if(key == KEY_F3) this->bF3Pressed = false;
}

void SongBrowser::onChar(KeyboardEvent &e) {
    // context menu
    this->contextMenu->onChar(e);
    if(e.isConsumed()) return;

    if(e.getCharCode() < 32 || !this->bVisible || this->bBeatmapRefreshScheduled ||
       (keyboard->isControlDown() && !keyboard->isAltDown()))
        return;
    if(this->bF1Pressed || this->bF2Pressed || this->bF3Pressed) return;

    // handle searching
    KEYCODE charCode = e.getCharCode();
    UString stringChar = "";
    stringChar.insert(0, charCode);
    this->sSearchString.append(stringChar);

    this->scheduleSearchUpdate();
}

void SongBrowser::onResolutionChange(Vector2 newResolution) { ScreenBackable::onResolutionChange(newResolution); }

CBaseUIContainer *SongBrowser::setVisible(bool visible) {
    if(bancho->spectating && visible) return this;  // don't allow song browser to be visible while spectating
    if(visible == this->bVisible) return this;

    this->bVisible = visible;
    this->bShiftPressed = false;  // seems to get stuck sometimes otherwise

    if(this->bVisible) {
        soundEngine->play(osu->getSkin()->expand);
        RichPresence::onSongBrowser();

        this->updateLayout();

        // we have to re-select the current beatmap to start playing music again
        if(this->beatmap != NULL) this->beatmap->select();

        this->bHasSelectedAndIsPlaying = false;  // sanity

        // try another refresh, maybe the osu!folder has changed
        if(this->beatmaps.size() == 0) {
            this->refreshBeatmaps();
        }

        // update user name/stats
        osu->onUserCardChange(cv::name.getString().c_str());

        // HACKHACK: workaround for BaseUI framework deficiency (missing mouse events. if a mouse button is being held,
        // and then suddenly a BaseUIElement gets put under it and set visible, and then the mouse button is released,
        // that "incorrectly" fires onMouseUpInside/onClicked/etc.)
        mouse->onLeftChange(false);
        mouse->onRightChange(false);

        if(this->beatmap != NULL) {
            // For multiplayer: if the host exits song selection without selecting a song, we want to be able to revert
            // to that previous song.
            this->lastSelectedBeatmap = this->beatmap->getSelectedDifficulty2();

            // Select button matching current song preview
            this->selectSelectedBeatmapSongButton();
        }
    } else {
        this->contextMenu->setVisible2(false);
    }

    osu->chat->updateVisibility();
    return this;
}

void SongBrowser::selectSelectedBeatmapSongButton() {
    if(this->beatmap == NULL) return;

    auto diff = this->beatmap->getSelectedDifficulty2();
    if(diff == NULL) return;

    auto it = this->hashToSongButton.find(diff->getMD5Hash());
    if(it == this->hashToSongButton.end()) {
        debugLog("No song button found for currently selected beatmap...\n");
        return;
    }

    auto btn = it->second;
    for(auto sub_btn : btn->getChildren()) {
        // hashToSongButton points to the *beatmap* song button.
        // We want to select the *difficulty* song button.
        if(sub_btn->getDatabaseBeatmap() == diff) {
            btn = sub_btn;
            break;
        }
    }

    if(btn->getDatabaseBeatmap() != diff) {
        debugLog("Found matching beatmap, but not matching difficulty.\n");
        return;
    }

    btn->deselect();  // if we select() it when already selected, it would start playing!
    btn->select();
}

void SongBrowser::onPlayEnd(bool quit) {
    this->bHasSelectedAndIsPlaying = false;

    // update score displays
    if(!quit) {
        this->rebuildScoreButtons();

        SongDifficultyButton *selectedSongDiffButton = dynamic_cast<SongDifficultyButton *>(this->selectedButton);
        if(selectedSongDiffButton != NULL) selectedSongDiffButton->updateGrade();
    }

    // update song info
    if(this->beatmap != NULL && this->beatmap->getSelectedDifficulty2() != NULL) {
        this->songInfo->setFromBeatmap(this->beatmap, this->beatmap->getSelectedDifficulty2());
    }
}

void SongBrowser::onSelectionChange(Button *button, bool rebuild) {
    this->selectedButton = button;
    if(button == NULL) return;

    this->contextMenu->setVisible2(false);

    // keep track and update all selection states
    // I'm still not happy with this, but at least all state update logic is localized in this function instead of
    // spread across all buttons

    SongButton *songButtonPointer = dynamic_cast<SongButton *>(button);
    SongDifficultyButton *songDiffButtonPointer = dynamic_cast<SongDifficultyButton *>(button);
    CollectionButton *collectionButtonPointer = dynamic_cast<CollectionButton *>(button);

    if(songDiffButtonPointer != NULL) {
        if(this->selectionPreviousSongDiffButton != NULL &&
           this->selectionPreviousSongDiffButton != songDiffButtonPointer)
            this->selectionPreviousSongDiffButton->deselect();

        // support individual diffs independent from their parent song button container
        {
            // if the new diff has a parent song button, then update its selection state (select it to stay consistent)
            if(songDiffButtonPointer->getParentSongButton() != NULL &&
               !songDiffButtonPointer->getParentSongButton()->isSelected()) {
                songDiffButtonPointer->getParentSongButton()
                    ->sortChildren();  // NOTE: workaround for disabled callback firing in select()
                songDiffButtonPointer->getParentSongButton()->select(false);
                this->onSelectionChange(songDiffButtonPointer->getParentSongButton(), false);  // NOTE: recursive call
            }

            // if the new diff does not have a parent song button, but the previous diff had, then update the previous
            // diff parent song button selection state (to deselect it)
            if(songDiffButtonPointer->getParentSongButton() == NULL) {
                if(this->selectionPreviousSongDiffButton != NULL &&
                   this->selectionPreviousSongDiffButton->getParentSongButton() != NULL)
                    this->selectionPreviousSongDiffButton->getParentSongButton()->deselect();
            }
        }

        this->selectionPreviousSongDiffButton = songDiffButtonPointer;
    } else if(songButtonPointer != NULL) {
        if(this->selectionPreviousSongButton != NULL && this->selectionPreviousSongButton != songButtonPointer)
            this->selectionPreviousSongButton->deselect();
        if(this->selectionPreviousSongDiffButton != NULL) this->selectionPreviousSongDiffButton->deselect();

        this->selectionPreviousSongButton = songButtonPointer;
    } else if(collectionButtonPointer != NULL) {
        // TODO: maybe expand this logic with per-group-type last-open-collection memory

        // logic for allowing collections to be deselected by clicking on the same button (contrary to how beatmaps
        // work)
        const bool isTogglingCollection = (this->selectionPreviousCollectionButton != NULL &&
                                           this->selectionPreviousCollectionButton == collectionButtonPointer);

        if(this->selectionPreviousCollectionButton != NULL) this->selectionPreviousCollectionButton->deselect();

        this->selectionPreviousCollectionButton = collectionButtonPointer;

        if(isTogglingCollection) this->selectionPreviousCollectionButton = NULL;
    }

    if(rebuild) this->rebuildSongButtons();
}

void SongBrowser::onDifficultySelected(DatabaseBeatmap *diff2, bool play) {
    // deselect = unload
    auto prev_diff2 = this->beatmap->getSelectedDifficulty2();
    this->beatmap->deselect();
    if(diff2 != prev_diff2 && !diff2->do_not_store) {
        this->previousRandomBeatmaps.push_back(diff2);
    }

    // select = play preview music
    this->beatmap->selectDifficulty2(diff2);

    // update song info
    this->songInfo->setFromBeatmap(this->beatmap, diff2);

    // start playing
    if(play) {
        if(bancho->is_in_a_multi_room()) {
            bancho->room.map_name = UString::format("%s - %s [%s]", diff2->getArtist().c_str(),
                                                   diff2->getTitle().c_str(), diff2->getDifficultyName().c_str());
            bancho->room.map_md5 = diff2->getMD5Hash();
            bancho->room.map_id = diff2->getID();

            Packet packet;
            packet.id = MATCH_CHANGE_SETTINGS;
            bancho->room.pack(&packet);
            BANCHO::Net::send_packet(packet);

            osu->room->on_map_change();

            this->setVisible(false);
        } else {
            // CTRL + click = auto
            if(keyboard->isControlDown()) {
                osu->bModAutoTemp = true;
                osu->getModSelector()->enableAuto();
            }

            if(this->beatmap->play()) {
                this->bHasSelectedAndIsPlaying = true;
                this->setVisible(false);
            }
        }
    }

    // animate
    this->fPulseAnimation = 1.0f;
    anim->moveLinear(&this->fPulseAnimation, 0.0f, 0.55f, true);

    // update score display
    this->rebuildScoreButtons();

    // update web button
    this->webButton->setVisible(this->songInfo->getBeatmapID() > 0);

    // trigger dynamic star calc (including current mods etc.)
    this->recalculateStarsForSelectedBeatmap();
}

void SongBrowser::refreshBeatmaps() {
    if(!this->bVisible || this->bHasSelectedAndIsPlaying) return;

    // reset
    this->checkHandleKillBackgroundSearchMatcher();

    // don't pause the music the first time we load the song database
    static bool first_refresh = true;
    if(first_refresh) {
        this->beatmap->music = NULL;
        first_refresh = false;
    }

    auto diff2 = this->beatmap->getSelectedDifficulty2();
    if(diff2) {
        this->beatmap_to_reselect_after_db_load = diff2->getMD5Hash();
    }

    this->beatmap->pausePreviewMusic();
    this->beatmap->deselect();
    SAFE_DELETE(this->beatmap);
    this->beatmap = new Beatmap();

    this->selectionPreviousSongButton = NULL;
    this->selectionPreviousSongDiffButton = NULL;
    this->selectionPreviousCollectionButton = NULL;

    // delete local database and UI
    this->carousel->getContainer()->empty();

    for(size_t i = 0; i < this->songButtons.size(); i++) {
        delete this->songButtons[i];
    }
    this->songButtons.clear();
    this->hashToSongButton.clear();
    for(size_t i = 0; i < this->collectionButtons.size(); i++) {
        delete this->collectionButtons[i];
    }
    this->collectionButtons.clear();
    for(size_t i = 0; i < this->artistCollectionButtons.size(); i++) {
        delete this->artistCollectionButtons[i];
    }
    this->artistCollectionButtons.clear();
    for(size_t i = 0; i < this->bpmCollectionButtons.size(); i++) {
        delete this->bpmCollectionButtons[i];
    }
    this->bpmCollectionButtons.clear();
    for(size_t i = 0; i < this->difficultyCollectionButtons.size(); i++) {
        delete this->difficultyCollectionButtons[i];
    }
    this->difficultyCollectionButtons.clear();
    for(size_t i = 0; i < this->creatorCollectionButtons.size(); i++) {
        delete this->creatorCollectionButtons[i];
    }
    this->creatorCollectionButtons.clear();
    for(size_t i = 0; i < this->dateaddedCollectionButtons.size(); i++) {
        delete this->dateaddedCollectionButtons[i];
    }
    this->dateaddedCollectionButtons.clear();
    for(size_t i = 0; i < this->lengthCollectionButtons.size(); i++) {
        delete this->lengthCollectionButtons[i];
    }
    this->lengthCollectionButtons.clear();
    for(size_t i = 0; i < this->titleCollectionButtons.size(); i++) {
        delete this->titleCollectionButtons[i];
    }
    this->titleCollectionButtons.clear();

    this->visibleSongButtons.clear();
    this->beatmaps.clear();
    this->previousRandomBeatmaps.clear();

    this->contextMenu->setVisible2(false);

    // clear potentially active search
    this->bInSearch = false;
    this->sSearchString = "";
    this->sPrevSearchString = "";
    this->fSearchWaitTime = 0.0f;
    this->searchPrevGroup = GROUP::GROUP_NO_GROUPING;

    // force no grouping
    if(this->group != GROUP::GROUP_NO_GROUPING) this->onGroupChange("", 0);

    // start loading
    this->bBeatmapRefreshScheduled = true;
    db->load();
}

void SongBrowser::addBeatmapSet(BeatmapSet *mapset) {
    if(mapset->getDifficulties().size() < 1) return;

    SongButton *songButton;
    if(mapset->getDifficulties().size() > 1) {
        songButton = new SongButton(this, this->carousel, this->contextMenu, 250, 250 + this->beatmaps.size() * 50, 200,
                                    50, "", mapset);
    } else {
        songButton =
            new SongDifficultyButton(this, this->carousel, this->contextMenu, 250, 250 + this->beatmaps.size() * 50,
                                     200, 50, "", mapset->getDifficulties()[0], NULL);
    }

    this->songButtons.push_back(songButton);
    for(auto diff : mapset->getDifficulties()) {
        this->hashToSongButton[diff->getMD5Hash()] = songButton;
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
        this->addSongButtonToAlphanumericGroup(songButton, this->artistCollectionButtons, mapset->getArtist());
        this->addSongButtonToAlphanumericGroup(songButton, this->creatorCollectionButtons, mapset->getCreator());
        this->addSongButtonToAlphanumericGroup(songButton, this->titleCollectionButtons, mapset->getTitle());

        // difficulty
        if(this->difficultyCollectionButtons.size() == 12) {
            for(SongButton *diff_btn : tempChildrenForGroups) {
                const auto &stars_tmp = diff_btn->getDatabaseBeatmap()->getStarsNomod();
                const int index = std::clamp<int>(
                    (std::isfinite(stars_tmp) && stars_tmp >= static_cast<float>(std::numeric_limits<int>::min()) &&
                     stars_tmp <= static_cast<float>(std::numeric_limits<int>::max()))
                        ? static_cast<int>(stars_tmp)
                        : 0,
                    0, 11);
                auto &children = this->difficultyCollectionButtons[index]->getChildren();
                auto it = std::ranges::lower_bound(children, diff_btn, sort_by_difficulty);
                children.insert(it, diff_btn);
            }
        }

        // bpm
        if(this->bpmCollectionButtons.size() == 6) {
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
                auto &children = this->bpmCollectionButtons[index]->getChildren();
                auto it = std::lower_bound(children.begin(), children.end(), diff_btn, sort_by_bpm);
                children.insert(it, diff_btn);
            }
        }

        // dateadded
        {
            // TODO: extremely annoying
        }

        // length
        if(this->lengthCollectionButtons.size() == 7) {
            for(auto diff_btn : tempChildrenForGroups) {
                const u32 lengthMS = diff_btn->getDatabaseBeatmap()->getLengthMS();

                std::vector<SongButton *> *children = NULL;
                if(lengthMS <= 1000 * 60) {
                    children = &this->lengthCollectionButtons[0]->getChildren();
                } else if(lengthMS <= 1000 * 60 * 2) {
                    children = &this->lengthCollectionButtons[1]->getChildren();
                } else if(lengthMS <= 1000 * 60 * 3) {
                    children = &this->lengthCollectionButtons[2]->getChildren();
                } else if(lengthMS <= 1000 * 60 * 4) {
                    children = &this->lengthCollectionButtons[3]->getChildren();
                } else if(lengthMS <= 1000 * 60 * 5) {
                    children = &this->lengthCollectionButtons[4]->getChildren();
                } else if(lengthMS <= 1000 * 60 * 10) {
                    children = &this->lengthCollectionButtons[5]->getChildren();
                } else {
                    children = &this->lengthCollectionButtons[6]->getChildren();
                }

                auto it = std::lower_bound(children->begin(), children->end(), diff_btn, sort_by_length);
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

    auto it = std::lower_bound(children->begin(), children->end(), btn, this->sortingComparator);
    if(cv::debug.getBool()) debugLog("Inserting %s at index %d\n", name.c_str(), it - children->begin());
    children->insert(it, btn);
}

void SongBrowser::requestNextScrollToSongButtonJumpFix(SongDifficultyButton *diffButton) {
    if(diffButton == NULL) return;

    this->bNextScrollToSongButtonJumpFixScheduled = true;
    this->fNextScrollToSongButtonJumpFixOldRelPosY =
        (diffButton->getParentSongButton() != NULL ? diffButton->getParentSongButton()->getRelPos().y
                                                   : diffButton->getRelPos().y);
    this->fNextScrollToSongButtonJumpFixOldScrollSizeY = this->carousel->getScrollSize().y;
}

bool SongBrowser::isButtonVisible(Button *songButton) {
    for(auto &btn : this->visibleSongButtons) {
        if(btn == songButton) {
            return true;
        }
        for(auto &child : btn->getChildren()) {
            if(child == songButton) {
                return true;
            }

            for(auto &grandchild : child->getChildren()) {
                if(grandchild == songButton) {
                    return true;
                }
            }
        }
    }

    return false;
}

void SongBrowser::scrollToBestButton() {
    for(auto &collection : this->visibleSongButtons) {
        for(auto &mapset : collection->getChildren()) {
            for(auto &diff : mapset->getChildren()) {
                if(diff->isSelected()) {
                    this->scrollToSongButton(diff);
                    return;
                }
            }

            if(mapset->isSelected()) {
                this->scrollToSongButton(mapset);
                return;
            }
        }

        if(collection->isSelected()) {
            this->scrollToSongButton(collection);
            return;
        }
    }

    this->carousel->scrollToTop();
}

void SongBrowser::scrollToSongButton(Button *songButton, bool alignOnTop) {
    if(songButton == NULL || !this->isButtonVisible(songButton)) {
        return;
    }

    // NOTE: compensate potential scroll jump due to added/removed elements (feels a lot better this way, also easier on
    // the eyes)
    if(this->bNextScrollToSongButtonJumpFixScheduled) {
        this->bNextScrollToSongButtonJumpFixScheduled = false;

        float delta = 0.0f;
        {
            if(!this->bNextScrollToSongButtonJumpFixUseScrollSizeDelta)
                delta = (songButton->getRelPos().y - this->fNextScrollToSongButtonJumpFixOldRelPosY);  // (default case)
            else
                delta = this->carousel->getScrollSize().y -
                        this->fNextScrollToSongButtonJumpFixOldScrollSizeY;  // technically not correct but feels a
                                                                             // lot better for KEY_LEFT navigation
        }
        this->carousel->scrollToY(this->carousel->getRelPosY() - delta, false);
    }

    this->carousel->scrollToY(-songButton->getRelPos().y +
                              (alignOnTop ? (0) : (this->carousel->getSize().y / 2 - songButton->getSize().y / 2)));
}

void SongBrowser::rebuildSongButtons() {
    this->carousel->getContainer()->empty();

    // NOTE: currently supports 3 depth layers (collection > beatmap > diffs)
    for(size_t i = 0; i < this->visibleSongButtons.size(); i++) {
        Button *button = this->visibleSongButtons[i];
        button->resetAnimations();

        if(!(button->isSelected() && button->isHiddenIfSelected()))
            this->carousel->getContainer()->addBaseUIElement(button);

        // children
        if(button->isSelected()) {
            const auto &children = this->visibleSongButtons[i]->getChildren();
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

                if(this->bInSearch && !isButton2SearchMatch) continue;

                button2->resetAnimations();

                if(!(button2->isSelected() && button2->isHiddenIfSelected()))
                    this->carousel->getContainer()->addBaseUIElement(button2);

                // child children
                if(button2->isSelected()) {
                    const auto &children2 = button2->getChildren();
                    for(size_t c2 = 0; c2 < children2.size(); c2++) {
                        Button *button3 = children2[c2];

                        if(this->bInSearch && !button3->isSearchMatch()) continue;

                        button3->resetAnimations();

                        if(!(button3->isSelected() && button3->isHiddenIfSelected()))
                            this->carousel->getContainer()->addBaseUIElement(button3);
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

    this->updateSongButtonLayout();
}

void SongBrowser::updateSongButtonLayout() {
    // this rebuilds the entire songButton layout (songButtons in relation to others)
    // only the y axis is set, because the x axis is constantly animated and handled within the button classes
    // themselves
    const std::vector<CBaseUIElement *> &elements = this->carousel->getContainer()->getElements();

    int yCounter = this->carousel->getSize().y / 4;
    if(elements.size() <= 1) yCounter = this->carousel->getSize().y / 2;

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
    this->carousel->setScrollSizeToContent(this->carousel->getSize().y / 2);
}

bool SongBrowser::searchMatcher(const DatabaseBeatmap *databaseBeatmap,
                                const std::vector<UString> &searchStringTokens) {
    if(databaseBeatmap == NULL) return false;

    const std::vector<DatabaseBeatmap *> &diffs = databaseBeatmap->getDifficulties();
    const bool isContainer = (diffs.size() > 0);
    const int numDiffs = (isContainer ? diffs.size() : 1);
    auto speed = osu->getSelectedBeatmap()->getSpeedMultiplier();

    // TODO: optimize this dumpster fire. can at least cache the parsed tokens and literal strings array instead of
    // parsing every single damn time

    // intelligent search parser
    // all strings which are not expressions get appended with spaces between, then checked with one call to
    // findSubstringInDifficulty() the rest is interpreted NOTE: this code is quite shitty. the order of the operators
    // array does matter, because find() is used to detect their presence (and '=' would then break '<=' etc.)
    enum operatorId : uint8_t { EQ, LT, GT, LE, GE, NE };
    static const std::vector<std::pair<UString, operatorId>> operators = {
        std::pair<UString, operatorId>("<=", LE), std::pair<UString, operatorId>(">=", GE),
        std::pair<UString, operatorId>("<", LT),  std::pair<UString, operatorId>(">", GT),
        std::pair<UString, operatorId>("!=", NE), std::pair<UString, operatorId>("==", EQ),
        std::pair<UString, operatorId>("=", EQ),
    };

    enum keywordId : uint8_t {
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
                                            speed;
                                        break;
                                    case CPM:
                                        compareValue =
                                            (diff->getLengthMS() > 0 ? ((float)diff->getNumCircles() /
                                                                        (float)(diff->getLengthMS() / 1000.0f / 60.0f))
                                                                     : 0.0f) *
                                            speed;
                                        break;
                                    case SPM:
                                        compareValue =
                                            (diff->getLengthMS() > 0 ? ((float)diff->getNumSliders() /
                                                                        (float)(diff->getLengthMS() / 1000.0f / 60.0f))
                                                                     : 0.0f) *
                                            speed;
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
    if(!diff->getTitle().empty()) {
        if(UString{diff->getTitle()}.findIgnoreCase(searchString) != -1) return true;
    }

    if(!diff->getArtist().empty()) {
        if(UString{diff->getArtist()}.findIgnoreCase(searchString) != -1) return true;
    }

    if(!diff->getCreator().empty()) {
        if(UString{diff->getCreator()}.findIgnoreCase(searchString) != -1) return true;
    }

    if(!diff->getDifficultyName().empty()) {
        if(UString{diff->getDifficultyName()}.findIgnoreCase(searchString) != -1) return true;
    }

    if(!diff->getSource().empty()) {
        if(UString{diff->getSource()}.findIgnoreCase(searchString) != -1) return true;
    }

    if(!diff->getTags().empty()) {
        if(UString{diff->getTags()}.findIgnoreCase(searchString) != -1) return true;
    }

    if(diff->getID() > 0) {
        if(UString::fmt("{:d}", diff->getID()).findIgnoreCase(searchString) != -1) return true;
    }

    if(diff->getSetID() > 0) {
        if(UString::fmt("{:d}", diff->getSetID()).findIgnoreCase(searchString) != -1) return true;
    }

    return false;
}

void SongBrowser::updateLayout() {
    ScreenBackable::updateLayout();

    auto screen = osu->getScreenSize();
    bool is_widescreen = ((i32)(std::max(0, (i32)((screen.x - (screen.y * 4.f / 3.f)) / 2.f))) > 0);
    f32 global_scale = screen.x / (is_widescreen ? 1366.f : 1024.f);

    /* TODO: use?
     * const float uiScale = cv::ui_scale.getFloat();
     */

    const float dpiScale = Osu::getUIScale();

    const int margin = 5 * dpiScale;

    // topbar left
    // TODO @kiwec: move score sorting method to settings menu. instead make the dropdown
    //              select between Local/Country/Friends/Global
    // TODO @kiwec: have checkbox in settings to toggle displaying non-vanilla scores
    // TODO @kiwec: have checkbox in settings to toggle only showing selected mods (?)

    this->topbarLeft->setSize(global_scale * 390.f, global_scale * 145.f);
    this->songInfo->setRelPos(margin, margin);
    this->songInfo->setSize(
        this->topbarLeft->getSize().x - margin,
        std::max(this->topbarLeft->getSize().y * 0.75f, this->songInfo->getMinimumHeight() + margin));

    const int topbarLeftButtonMargin = 5 * dpiScale;
    const int topbarLeftButtonHeight = 30 * dpiScale;
    const int topbarLeftButtonWidth = 55 * dpiScale;
    this->webButton->onResized();  // HACKHACK: framework bug (should update string metrics on setSize())
    this->webButton->setSize(topbarLeftButtonWidth, topbarLeftButtonHeight);
    this->webButton->setRelPos(this->topbarLeft->getSize().x - (topbarLeftButtonMargin + topbarLeftButtonWidth),
                               this->topbarLeft->getSize().y - this->webButton->getSize().y);

    this->scoreSortButton->onResized();  // HACKHACK: framework bug (should update string metrics on setSize())
    this->scoreSortButton->setSize(
        this->topbarLeft->getSize().x - 3 * topbarLeftButtonMargin - (topbarLeftButtonWidth + topbarLeftButtonMargin),
        topbarLeftButtonHeight);
    this->scoreSortButton->setRelPos(topbarLeftButtonMargin,
                                     this->topbarLeft->getSize().y - this->scoreSortButton->getSize().y);

    this->topbarLeft->update_pos();

    // topbar right
    this->topbarRight->setPosX(osu->getScreenWidth() / 2);
    this->topbarRight->setSize(osu->getScreenWidth() - this->topbarRight->getPos().x, 80.f * global_scale);

    float btn_margin = 10.f * dpiScale;
    this->sortButton->setSize(200.f * dpiScale, 30.f * dpiScale);
    this->sortButton->setRelPos(this->topbarRight->getSize().x - (this->sortButton->getSize().x + btn_margin),
                                btn_margin);

    this->sortLabel->onResized();  // HACKHACK: framework bug (should update string metrics on setSizeToContent())
    this->sortLabel->setSizeToContent(3 * dpiScale);
    this->sortLabel->setRelPos(this->sortButton->getRelPos().x - (this->sortLabel->getSize().x + btn_margin),
                               (this->sortLabel->getSize().y + btn_margin) / 2.f);

    this->groupButton->setSize(this->sortButton->getSize());
    this->groupButton->setRelPos(
        this->sortLabel->getRelPos().x - (this->sortButton->getSize().x + 30.f * dpiScale + btn_margin), btn_margin);

    this->groupLabel->onResized();  // HACKHACK: framework bug (should update string metrics on setSizeToContent())
    this->groupLabel->setSizeToContent(3 * dpiScale);
    this->groupLabel->setRelPos(this->groupButton->getRelPos().x - (this->groupLabel->getSize().x + btn_margin),
                                (this->groupLabel->getSize().y + btn_margin) / 2.f);

    // "hardcoded" group buttons
    const i32 group_btn_width =
        std::clamp<i32>((this->topbarRight->getSize().x - 2 * btn_margin) / 4, 0, 200 * dpiScale);
    this->groupByCollectionBtn->setSize(group_btn_width, 30 * dpiScale);
    this->groupByCollectionBtn->setRelPos(this->topbarRight->getSize().x - (btn_margin + (4 * group_btn_width)),
                                          this->topbarRight->getSize().y - 30 * dpiScale);
    this->groupByArtistBtn->setSize(group_btn_width, 30 * dpiScale);
    this->groupByArtistBtn->setRelPos(this->topbarRight->getSize().x - (btn_margin + (3 * group_btn_width)),
                                      this->topbarRight->getSize().y - 30 * dpiScale);
    this->groupByDifficultyBtn->setSize(group_btn_width, 30 * dpiScale);
    this->groupByDifficultyBtn->setRelPos(this->topbarRight->getSize().x - (btn_margin + (2 * group_btn_width)),
                                          this->topbarRight->getSize().y - 30 * dpiScale);
    this->groupByNothingBtn->setSize(group_btn_width, 30 * dpiScale);
    this->groupByNothingBtn->setRelPos(this->topbarRight->getSize().x - (btn_margin + (1 * group_btn_width)),
                                       this->topbarRight->getSize().y - 30 * dpiScale);

    this->topbarRight->update_pos();

    // score browser
    this->updateScoreBrowserLayout();

    // song browser
    this->carousel->setPos(this->topbarLeft->getPos().x + this->topbarLeft->getSize().x + 1, 0);
    this->carousel->setSize(osu->getScreenWidth() - (this->topbarLeft->getPos().x + this->topbarLeft->getSize().x),
                            osu->getScreenHeight());
    this->updateSongButtonLayout();

    this->search->setPos(osu->getScreenWidth() / 2, this->topbarRight->getSize().y + 8 * dpiScale);
    this->search->setSize(osu->getScreenWidth() / 2, 20 * dpiScale);
}

void SongBrowser::onBack() { osu->toggleSongBrowser(); }

void SongBrowser::updateScoreBrowserLayout() {
    const float dpiScale = Osu::getUIScale();

    auto screen = osu->getScreenSize();
    bool is_widescreen = ((i32)(std::max(0, (i32)((screen.x - (screen.y * 4.f / 3.f)) / 2.f))) > 0);
    f32 global_scale = screen.x / (is_widescreen ? 1366.f : 1024.f);

    const bool shouldScoreBrowserBeVisible =
        (cv::scores_enabled.getBool() && cv::songbrowser_scorebrowser_enabled.getBool());
    if(shouldScoreBrowserBeVisible != this->scoreBrowser->isVisible())
        this->scoreBrowser->setVisible(shouldScoreBrowserBeVisible);

    const int scoreButtonWidthMax = this->topbarLeft->getSize().x;

    /* TODO: use?
     * f32 back_btn_height = osu->getSkin()->getMenuBack2()->getSize().y;
     */
    f32 browserHeight = osu->getScreenHeight() -
                        (get_bottombar_height() + (this->topbarLeft->getPos().y + this->topbarLeft->getSize().y)) +
                        2 * dpiScale;
    this->scoreBrowser->setPos(this->topbarLeft->getPos().x + 2 * dpiScale,
                               this->topbarLeft->getPos().y + this->topbarLeft->getSize().y + 4 * dpiScale);
    this->scoreBrowser->setSize(scoreButtonWidthMax, browserHeight);
    const i32 scoreHeight = 53.f * global_scale;

    // In stable, even when looking at local scores, there is space where the "local best" would be.
    f32 local_best_size = scoreHeight + 61 * global_scale;
    browserHeight -= local_best_size;
    this->scoreBrowser->setSize(this->scoreBrowser->getSize().x, browserHeight);
    this->scoreBrowser->setScrollSizeToContent();

    if(this->localBestContainer->isVisible()) {
        this->localBestContainer->setPos(this->scoreBrowser->getPos().x,
                                         this->scoreBrowser->getPos().y + this->scoreBrowser->getSize().y);
        this->localBestContainer->setSize(this->scoreBrowser->getPos().x, local_best_size);
        this->localBestLabel->setRelPos(this->scoreBrowser->getPos().x, 0);
        this->localBestLabel->setSize(this->scoreBrowser->getSize().x, 40);
        if(this->localBestButton) {
            this->localBestButton->setRelPos(this->scoreBrowser->getPos().x, 40);
            this->localBestButton->setSize(this->scoreBrowser->getSize().x, scoreHeight);
        }
    }

    const std::vector<CBaseUIElement *> &elements = this->scoreBrowser->getContainer()->getElements();
    for(size_t i = 0; i < elements.size(); i++) {
        CBaseUIElement *scoreButton = elements[i];
        scoreButton->setSize(this->scoreBrowser->getSize().x, scoreHeight);
        scoreButton->setRelPos(0, i * scoreButton->getSize().y);
    }
    this->scoreBrowserScoresStillLoadingElement->setSize(this->scoreBrowser->getSize().x * 0.9f, scoreHeight * 0.75f);
    this->scoreBrowserScoresStillLoadingElement->setRelPos(
        this->scoreBrowser->getSize().x / 2 - this->scoreBrowserScoresStillLoadingElement->getSize().x / 2,
        (browserHeight / 2) * 0.65f - this->scoreBrowserScoresStillLoadingElement->getSize().y / 2);
    this->scoreBrowserNoRecordsYetElement->setSize(this->scoreBrowser->getSize().x * 0.9f, scoreHeight * 0.75f);
    if(elements[0] == this->scoreBrowserNoRecordsYetElement) {
        this->scoreBrowserNoRecordsYetElement->setRelPos(
            this->scoreBrowser->getSize().x / 2 - this->scoreBrowserScoresStillLoadingElement->getSize().x / 2,
            (browserHeight / 2) * 0.65f - this->scoreBrowserScoresStillLoadingElement->getSize().y / 2);
    } else {
        this->scoreBrowserNoRecordsYetElement->setRelPos(
            this->scoreBrowser->getSize().x / 2 - this->scoreBrowserScoresStillLoadingElement->getSize().x / 2, 45);
    }
    this->localBestContainer->update_pos();
    this->scoreBrowser->getContainer()->update_pos();
    this->scoreBrowser->setScrollSizeToContent();
}

void SongBrowser::rebuildScoreButtons() {
    if(!this->isVisible()) return;

    // XXX: When online, it would be nice to scroll to the current user's highscore

    // reset
    this->scoreBrowser->getContainer()->empty();
    this->localBestContainer->empty();
    this->localBestContainer->setVisible(false);

    const bool validBeatmap = (this->beatmap != NULL && this->beatmap->getSelectedDifficulty2() != NULL);
    bool is_online = cv::songbrowser_scores_sortingtype.getString() == "Online Leaderboard";

    std::vector<FinishedScore> scores;
    if(validBeatmap) {
        std::scoped_lock lock(db->scores_mtx);
        auto diff2 = this->beatmap->getSelectedDifficulty2();
        auto local_scores = db->scores[diff2->getMD5Hash()];
        auto local_best = max_element(local_scores.begin(), local_scores.end(),
                                      [](FinishedScore const &a, FinishedScore const &b) { return a.score < b.score; });

        if(is_online) {
            auto search = db->online_scores.find(diff2->getMD5Hash());
            if(search != db->online_scores.end()) {
                scores = search->second;

                if(local_best == local_scores.end()) {
                    if(!scores.empty()) {
                        // We only want to display "No scores" if there are online scores present
                        // Otherwise, it would be displayed twice
                        SAFE_DELETE(this->localBestButton);
                        this->localBestContainer->addBaseUIElement(this->localBestLabel);
                        this->localBestContainer->addBaseUIElement(this->scoreBrowserNoRecordsYetElement);
                        this->localBestContainer->setVisible(true);
                    }
                } else {
                    SAFE_DELETE(this->localBestButton);
                    this->localBestButton = new ScoreButton(this->contextMenu, 0, 0, 0, 0);
                    this->localBestButton->setClickCallback(
                        fastdelegate::MakeDelegate(this, &SongBrowser::onScoreClicked));
                    this->localBestButton->map_hash = diff2->getMD5Hash();
                    this->localBestButton->setScore(*local_best, diff2);
                    this->localBestButton->resetHighlight();
                    this->localBestButton->grabs_clicks = true;
                    this->localBestContainer->addBaseUIElement(this->localBestLabel);
                    this->localBestContainer->addBaseUIElement(this->localBestButton);
                    this->localBestContainer->setVisible(true);
                }

                // We have already fetched the scores so there's no point in showing "Loading...".
                // When there are no online scores for this map, let's treat it as if we are
                // offline in order to show "No records set!" instead.
                is_online = false;
            } else {
                // We haven't fetched the scores yet, do so now
                BANCHO::Leaderboard::fetch_online_scores(diff2);

                // Display local best while scores are loading
                if(local_best != local_scores.end()) {
                    SAFE_DELETE(this->localBestButton);
                    this->localBestButton = new ScoreButton(this->contextMenu, 0, 0, 0, 0);
                    this->localBestButton->setClickCallback(
                        fastdelegate::MakeDelegate(this, &SongBrowser::onScoreClicked));
                    this->localBestButton->map_hash = diff2->getMD5Hash();
                    this->localBestButton->setScore(*local_best, diff2);
                    this->localBestButton->resetHighlight();
                    this->localBestButton->grabs_clicks = true;
                    this->localBestContainer->addBaseUIElement(this->localBestLabel);
                    this->localBestContainer->addBaseUIElement(this->localBestButton);
                    this->localBestContainer->setVisible(true);
                }
            }
        } else {
            scores = local_scores;
        }
    }

    const int numScores = scores.size();

    // top up cache as necessary
    if(numScores > this->scoreButtonCache.size()) {
        const int numNewButtons = numScores - this->scoreButtonCache.size();
        for(size_t i = 0; i < numNewButtons; i++) {
            ScoreButton *scoreButton = new ScoreButton(this->contextMenu, 0, 0, 0, 0);
            scoreButton->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onScoreClicked));
            this->scoreButtonCache.push_back(scoreButton);
        }
    }

    // and build the ui
    if(numScores < 1) {
        if(validBeatmap && is_online) {
            this->scoreBrowser->getContainer()->addBaseUIElement(
                this->scoreBrowserScoresStillLoadingElement, this->scoreBrowserScoresStillLoadingElement->getRelPos().x,
                this->scoreBrowserScoresStillLoadingElement->getRelPos().y);
        } else {
            this->scoreBrowser->getContainer()->addBaseUIElement(
                this->scoreBrowserNoRecordsYetElement, this->scoreBrowserScoresStillLoadingElement->getRelPos().x,
                this->scoreBrowserScoresStillLoadingElement->getRelPos().y);
        }
    } else {
        // sort
        db->sortScoresInPlace(scores);

        // build
        std::vector<ScoreButton *> scoreButtons;
        for(size_t i = 0; i < numScores; i++) {
            ScoreButton *button = this->scoreButtonCache[i];
            button->map_hash = this->beatmap->getSelectedDifficulty2()->getMD5Hash();
            button->setScore(scores[i], this->beatmap->getSelectedDifficulty2(), i + 1);
            scoreButtons.push_back(button);
        }

        // add
        for(size_t i = 0; i < numScores; i++) {
            scoreButtons[i]->setIndex(i + 1);
            this->scoreBrowser->getContainer()->addBaseUIElement(scoreButtons[i]);
        }

        // reset
        for(size_t i = 0; i < scoreButtons.size(); i++) {
            scoreButtons[i]->resetHighlight();
        }
    }

    // layout
    this->updateScoreBrowserLayout();
}

void SongBrowser::scheduleSearchUpdate(bool immediately) {
    this->fSearchWaitTime = engine->getTime() + (immediately ? 0.0f : cv::songbrowser_search_delay.getFloat());
}

void SongBrowser::checkHandleKillBackgroundSearchMatcher() {
    if(!this->backgroundSearchMatcher->isDead()) {
        this->backgroundSearchMatcher->kill();

        const double startTime = engine->getTimeReal();
        while(!this->backgroundSearchMatcher->isAsyncReady()) {
            if(engine->getTimeReal() - startTime > 2) {
                debugLog("WARNING: Ignoring stuck SearchMatcher thread!\n");
                break;
            }
        }
    }
}

void SongBrowser::onDatabaseLoadingFinished() {
    // having a copy of the vector in here is actually completely unnecessary
    this->beatmaps = std::vector<DatabaseBeatmap *>(db->getDatabaseBeatmaps());

    debugLog("SongBrowser::onDatabaseLoadingFinished() : %i beatmapsets.\n", this->beatmaps.size());

    // initialize all collection (grouped) buttons
    {
        // artist
        {
            // 0-9
            {
                CollectionButton *b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50,
                                                           "", "0-9", std::vector<SongButton *>());
                this->artistCollectionButtons.push_back(b);
            }

            // A-Z
            for(size_t i = 0; i < 26; i++) {
                UString artistCollectionName = UString::format("%c", 'A' + i);

                CollectionButton *b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50,
                                                           "", artistCollectionName, std::vector<SongButton *>());
                this->artistCollectionButtons.push_back(b);
            }

            // Other
            {
                CollectionButton *b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50,
                                                           "", "Other", std::vector<SongButton *>());
                this->artistCollectionButtons.push_back(b);
            }
        }

        // difficulty
        for(size_t i = 0; i < 12; i++) {
            UString difficultyCollectionName = UString::format(i == 1 ? "%i star" : "%i stars", i);
            if(i < 1) difficultyCollectionName = "Below 1 star";
            if(i > 10) difficultyCollectionName = "Above 10 stars";

            std::vector<SongButton *> children;

            CollectionButton *b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50, "",
                                                       difficultyCollectionName, children);
            this->difficultyCollectionButtons.push_back(b);
        }

        // bpm
        {
            CollectionButton *b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50, "",
                                                       "Under 60 BPM", std::vector<SongButton *>());
            this->bpmCollectionButtons.push_back(b);
            b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50, "", "Under 120 BPM",
                                     std::vector<SongButton *>());
            this->bpmCollectionButtons.push_back(b);
            b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50, "", "Under 180 BPM",
                                     std::vector<SongButton *>());
            this->bpmCollectionButtons.push_back(b);
            b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50, "", "Under 240 BPM",
                                     std::vector<SongButton *>());
            this->bpmCollectionButtons.push_back(b);
            b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50, "", "Under 300 BPM",
                                     std::vector<SongButton *>());
            this->bpmCollectionButtons.push_back(b);
            b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50, "", "Over 300 BPM",
                                     std::vector<SongButton *>());
            this->bpmCollectionButtons.push_back(b);
        }

        // creator
        {
            // 0-9
            {
                CollectionButton *b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50,
                                                           "", "0-9", std::vector<SongButton *>());
                this->creatorCollectionButtons.push_back(b);
            }

            // A-Z
            for(size_t i = 0; i < 26; i++) {
                UString artistCollectionName = UString::format("%c", 'A' + i);

                CollectionButton *b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50,
                                                           "", artistCollectionName, std::vector<SongButton *>());
                this->creatorCollectionButtons.push_back(b);
            }

            // Other
            {
                CollectionButton *b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50,
                                                           "", "Other", std::vector<SongButton *>());
                this->creatorCollectionButtons.push_back(b);
            }
        }

        // dateadded
        {
            // TODO: annoying
        }

        // length
        {
            CollectionButton *b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50, "",
                                                       "1 minute or less", std::vector<SongButton *>());
            this->lengthCollectionButtons.push_back(b);
            b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50, "",
                                     "2 minutes or less", std::vector<SongButton *>());
            this->lengthCollectionButtons.push_back(b);
            b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50, "",
                                     "3 minutes or less", std::vector<SongButton *>());
            this->lengthCollectionButtons.push_back(b);
            b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50, "",
                                     "4 minutes or less", std::vector<SongButton *>());
            this->lengthCollectionButtons.push_back(b);
            b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50, "",
                                     "5 minutes or less", std::vector<SongButton *>());
            this->lengthCollectionButtons.push_back(b);
            b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50, "",
                                     "10 minutes or less", std::vector<SongButton *>());
            this->lengthCollectionButtons.push_back(b);
            b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50, "", "Over 10 minutes",
                                     std::vector<SongButton *>());
            this->lengthCollectionButtons.push_back(b);
        }

        // title
        {
            // 0-9
            {
                CollectionButton *b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50,
                                                           "", "0-9", std::vector<SongButton *>());
                this->titleCollectionButtons.push_back(b);
            }

            // A-Z
            for(size_t i = 0; i < 26; i++) {
                UString artistCollectionName = UString::format("%c", 'A' + i);

                CollectionButton *b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50,
                                                           "", artistCollectionName, std::vector<SongButton *>());
                this->titleCollectionButtons.push_back(b);
            }

            // Other
            {
                CollectionButton *b = new CollectionButton(this, this->carousel, this->contextMenu, 250, 250, 200, 50,
                                                           "", "Other", std::vector<SongButton *>());
                this->titleCollectionButtons.push_back(b);
            }
        }
    }

    // add all beatmaps (build buttons)
    for(size_t i = 0; i < this->beatmaps.size(); i++) {
        this->addBeatmapSet(this->beatmaps[i]);
    }

    // build collections
    this->recreateCollectionsButtons();

    this->onSortChange(cv::songbrowser_sortingtype.getString().c_str());
    this->onSortScoresChange(cv::songbrowser_scores_sortingtype.getString().c_str());

    // update rich presence (discord total pp)
    RichPresence::onSongBrowser();

    // update user name/stats
    osu->onUserCardChange(cv::name.getString().c_str());

    if(cv::songbrowser_search_hardcoded_filter.getString().length() > 0) this->onSearchUpdate();

    if(this->beatmap_to_reselect_after_db_load.hash[0] != 0) {
        auto beatmap = db->getBeatmapDifficulty(this->beatmap_to_reselect_after_db_load);
        if(beatmap) {
            this->onDifficultySelected(beatmap, false);
            this->selectSelectedBeatmapSongButton();
        }

        SAFE_DELETE(osu->mainMenu->preloaded_beatmapset);
    }

    // ok, if we still haven't selected a song, do so now
    if(this->beatmap->getSelectedDifficulty2() == NULL) {
        this->selectRandomBeatmap();
    }
}

void SongBrowser::onSearchUpdate() {
    const bool hasHardcodedSearchStringChanged =
        (this->sPrevHardcodedSearchString != cv::songbrowser_search_hardcoded_filter.getString().c_str());
    const bool hasSearchStringChanged = (this->sPrevSearchString != this->sSearchString);

    const bool prevInSearch = this->bInSearch;
    this->bInSearch =
        (this->sSearchString.length() > 0 || cv::songbrowser_search_hardcoded_filter.getString().length() > 0);
    const bool hasInSearchChanged = (prevInSearch != this->bInSearch);

    if(this->bInSearch) {
        this->searchPrevGroup = this->group;

        // flag all search matches across entire database
        if(hasSearchStringChanged || hasHardcodedSearchStringChanged || hasInSearchChanged) {
            // stop potentially running async search
            this->checkHandleKillBackgroundSearchMatcher();

            this->backgroundSearchMatcher->revive();
            this->backgroundSearchMatcher->release();
            this->backgroundSearchMatcher->setSongButtonsAndSearchString(
                this->songButtons, this->sSearchString, cv::songbrowser_search_hardcoded_filter.getString().c_str());

            resourceManager->requestNextLoadAsync();
            resourceManager->loadResource(this->backgroundSearchMatcher);
        } else
            this->rebuildSongButtonsAndVisibleSongButtonsWithSearchMatchSupport(true);

        // (results are handled in update() once available)
    } else  // exit search
    {
        // exiting the search does not need any async work, so we can just directly do it in here

        // stop potentially running async search
        this->checkHandleKillBackgroundSearchMatcher();

        // reset container and visible buttons list
        this->carousel->getContainer()->empty();
        this->visibleSongButtons.clear();

        // reset all search flags
        for(size_t i = 0; i < this->songButtons.size(); i++) {
            const auto &children = this->songButtons[i]->getChildren();
            if(children.size() > 0) {
                for(size_t c = 0; c < children.size(); c++) {
                    children[c]->setIsSearchMatch(true);
                }
            } else
                this->songButtons[i]->setIsSearchMatch(true);
        }

        // remember which tab was selected, instead of defaulting back to no grouping
        // this also rebuilds the visible buttons list
        for(size_t i = 0; i < this->groupings.size(); i++) {
            if(this->groupings[i].type == this->searchPrevGroup) this->onGroupChange("", this->groupings[i].id);
        }
    }

    this->sPrevSearchString = this->sSearchString;
    this->sPrevHardcodedSearchString = cv::songbrowser_search_hardcoded_filter.getString().c_str();
}

void SongBrowser::rebuildSongButtonsAndVisibleSongButtonsWithSearchMatchSupport(bool scrollToTop,
                                                                                bool doRebuildSongButtons) {
    // reset container and visible buttons list
    this->carousel->getContainer()->empty();
    this->visibleSongButtons.clear();

    // use flagged search matches to rebuild visible song buttons
    {
        if(this->group == GROUP::GROUP_NO_GROUPING) {
            for(size_t i = 0; i < this->songButtons.size(); i++) {
                const auto &children = this->songButtons[i]->getChildren();
                if(children.size() > 0) {
                    // if all children match, then we still want to display the parent wrapper button (without expanding
                    // all diffs)
                    bool allChildrenMatch = true;
                    for(size_t c = 0; c < children.size(); c++) {
                        if(!children[c]->isSearchMatch()) allChildrenMatch = false;
                    }

                    if(allChildrenMatch)
                        this->visibleSongButtons.push_back(this->songButtons[i]);
                    else {
                        // rip matching children from parent
                        for(size_t c = 0; c < children.size(); c++) {
                            if(children[c]->isSearchMatch()) this->visibleSongButtons.push_back(children[c]);
                        }
                    }
                } else if(this->songButtons[i]->isSearchMatch())
                    this->visibleSongButtons.push_back(this->songButtons[i]);
            }
        } else {
            std::vector<CollectionButton *> *groupButtons = NULL;
            {
                switch(this->group) {
                    case GROUP::GROUP_NO_GROUPING:
                        break;
                    case GROUP::GROUP_ARTIST:
                        groupButtons = &this->artistCollectionButtons;
                        break;
                    case GROUP::GROUP_BPM:
                        groupButtons = &this->bpmCollectionButtons;
                        break;
                    case GROUP::GROUP_CREATOR:
                        groupButtons = &this->creatorCollectionButtons;
                        break;
                    case GROUP::GROUP_DIFFICULTY:
                        groupButtons = &this->difficultyCollectionButtons;
                        break;
                    case GROUP::GROUP_LENGTH:
                        groupButtons = &this->lengthCollectionButtons;
                        break;
                    case GROUP::GROUP_TITLE:
                        groupButtons = &this->titleCollectionButtons;
                        break;
                    case GROUP::GROUP_COLLECTIONS:
                        groupButtons = &this->collectionButtons;
                        break;
                    default:
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

                    if(isAnyMatchInGroup || !this->bInSearch) this->visibleSongButtons.push_back((*groupButtons)[i]);
                }
            }
        }

        if(doRebuildSongButtons) this->rebuildSongButtons();

        // scroll to top search result, or auto select the only result
        if(scrollToTop) {
            if(this->visibleSongButtons.size() > 1) {
                this->scrollToSongButton(this->visibleSongButtons[0]);
            } else if(this->visibleSongButtons.size() > 0) {
                this->selectSongButton(this->visibleSongButtons[0]);
            }
        }
    }
}

void SongBrowser::onSortScoresClicked(CBaseUIButton *button) {
    this->contextMenu->setPos(button->getPos());
    this->contextMenu->setRelPos(button->getRelPos());
    this->contextMenu->begin(button->getSize().x);
    {
        CBaseUIButton *button = this->contextMenu->addButton("Online Leaderboard");
        if(cv::songbrowser_scores_sortingtype.getString() == "Online Leaderboard")
            button->setTextBrightColor(0xff00ff00);

        const std::vector<Database::SCORE_SORTING_METHOD> &scoreSortingMethods = db->getScoreSortingMethods();
        for(size_t i = 0; i < scoreSortingMethods.size(); i++) {
            CBaseUIButton *button = this->contextMenu->addButton(scoreSortingMethods[i].name);
            if(scoreSortingMethods[i].name.utf8View() == cv::songbrowser_scores_sortingtype.getString())
                button->setTextBrightColor(0xff00ff00);
        }
    }
    this->contextMenu->end(false, false);
    this->contextMenu->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onSortScoresChange));
}

void SongBrowser::onSortScoresChange(const UString& text, int  /*id*/) {
    cv::songbrowser_scores_sortingtype.setValue(text);  // NOTE: remember
    this->scoreSortButton->setText(text);
    this->rebuildScoreButtons();
    this->scoreBrowser->scrollToTop();

    // update grades of all visible songdiffbuttons
    // NOTE(kiwec): why here???? wtf lol
    if(this->beatmap != NULL) {
        for(size_t i = 0; i < this->visibleSongButtons.size(); i++) {
            if(this->visibleSongButtons[i]->getDatabaseBeatmap() == this->beatmap->getSelectedDifficulty2()) {
                SongButton *songButtonPointer = dynamic_cast<SongButton *>(this->visibleSongButtons[i]);
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

void SongBrowser::onWebClicked(CBaseUIButton * /*button*/) {
    if(this->songInfo->getBeatmapID() > 0) {
        env->openURLInDefaultBrowser(UString::format("https://osu.ppy.sh/b/%ld", this->songInfo->getBeatmapID()));
        osu->getNotificationOverlay()->addNotification("Opening browser, please wait ...", 0xffffffff, false, 0.75f);
    }
}

void SongBrowser::onQuickGroupClicked(CBaseUIButton *button) {
    for(auto group : this->groupings) {
        if(group.name == button->getText()) {
            this->onGroupChange(group.name, group.id);
            return;
        }
    }
}

void SongBrowser::onGroupClicked(CBaseUIButton *button) {
    this->contextMenu->setPos(button->getPos());
    this->contextMenu->setRelPos(button->getRelPos());
    this->contextMenu->begin(button->getSize().x);
    {
        for(size_t i = 0; i < this->groupings.size(); i++) {
            CBaseUIButton *button = this->contextMenu->addButton(this->groupings[i].name, this->groupings[i].id);
            if(this->groupings[i].type == this->group) button->setTextBrightColor(0xff00ff00);
        }
    }
    this->contextMenu->end(false, false);
    this->contextMenu->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onGroupChange));
}

void SongBrowser::onGroupChange(const UString& text, int id) {
    this->groupByCollectionBtn->setTextBrightColor(defaultColor);
    this->groupByArtistBtn->setTextBrightColor(defaultColor);
    this->groupByDifficultyBtn->setTextBrightColor(defaultColor);
    this->groupByNothingBtn->setTextBrightColor(defaultColor);

    GROUPING *grouping = (this->groupings.size() > 0 ? &this->groupings[0] : NULL);
    for(size_t i = 0; i < this->groupings.size(); i++) {
        if(this->groupings[i].id == id || (text.length() > 1 && this->groupings[i].name == text)) {
            grouping = &this->groupings[i];
            break;
        }
    }
    if(grouping == NULL) return;

    // update group combobox button text
    this->groupButton->setText(grouping->name);

    // and update the actual songbrowser contents
    switch(grouping->type) {
        case GROUP::GROUP_NO_GROUPING:
            this->onGroupNoGrouping();
            break;
        case GROUP::GROUP_ARTIST:
            this->onGroupArtist();
            break;
        case GROUP::GROUP_BPM:
            this->onGroupBPM();
            break;
        case GROUP::GROUP_CREATOR:
            this->onGroupCreator();
            break;
        case GROUP::GROUP_DATEADDED:
            this->onGroupDateadded();
            break;
        case GROUP::GROUP_DIFFICULTY:
            this->onGroupDifficulty();
            break;
        case GROUP::GROUP_LENGTH:
            this->onGroupLength();
            break;
        case GROUP::GROUP_TITLE:
            this->onGroupTitle();
            break;
        case GROUP::GROUP_COLLECTIONS:
            this->onGroupCollections();
            break;
    }
}

void SongBrowser::onSortClicked(CBaseUIButton *button) {
    this->contextMenu->setPos(button->getPos());
    this->contextMenu->setRelPos(button->getRelPos());
    this->contextMenu->begin(button->getSize().x);
    {
        for(size_t i = 0; i < this->sortingMethods.size(); i++) {
            CBaseUIButton *button = this->contextMenu->addButton(this->sortingMethods[i].name);
            if(this->sortingMethods[i].type == this->sortingMethod) button->setTextBrightColor(0xff00ff00);
        }
    }
    this->contextMenu->end(false, false);
    this->contextMenu->setClickCallback(fastdelegate::MakeDelegate(this, &SongBrowser::onSortChange));
}

void SongBrowser::onSortChange(const UString& text, int  /*id*/) { this->onSortChangeInt(text, true); }

void SongBrowser::onSortChangeInt(const UString& text, bool  /*autoScroll*/) {
    SORTING_METHOD *sortingMethod = &this->sortingMethods[3];
    for(size_t i = 0; i < this->sortingMethods.size(); i++) {
        if(this->sortingMethods[i].name == text) {
            sortingMethod = &this->sortingMethods[i];
            break;
        }
    }

    this->sortingMethod = sortingMethod->type;
    this->sortingComparator = sortingMethod->comparator;
    this->sortButton->setText(sortingMethod->name);

    cv::songbrowser_sortingtype.setValue(sortingMethod->name);  // NOTE: remember persistently

    // resort primitive master button array (all songbuttons, No Grouping)
    std::sort(this->songButtons.begin(), this->songButtons.end(), this->sortingComparator);

    // resort Collection buttons (one button for each collection)
    // these are always sorted alphabetically by name
    std::ranges::sort(this->collectionButtons, UString::ncasecomp{},
                      [](const CollectionButton *btn) { return UString{btn->getCollectionName()}; });

    // resort Collection button array (each group of songbuttons inside each Collection)
    for(size_t i = 0; i < this->collectionButtons.size(); i++) {
        auto &children = this->collectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), this->sortingComparator);
        this->collectionButtons[i]->setChildren(children);
    }

    // etc.
    for(size_t i = 0; i < this->artistCollectionButtons.size(); i++) {
        auto &children = this->artistCollectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), this->sortingComparator);
        this->artistCollectionButtons[i]->setChildren(children);
    }
    for(size_t i = 0; i < this->bpmCollectionButtons.size(); i++) {
        auto &children = this->bpmCollectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), this->sortingComparator);
        this->bpmCollectionButtons[i]->setChildren(children);
    }
    for(size_t i = 0; i < this->difficultyCollectionButtons.size(); i++) {
        auto &children = this->difficultyCollectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), this->sortingComparator);
        this->difficultyCollectionButtons[i]->setChildren(children);
    }
    for(size_t i = 0; i < this->bpmCollectionButtons.size(); i++) {
        auto &children = this->bpmCollectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), this->sortingComparator);
        this->bpmCollectionButtons[i]->setChildren(children);
    }
    for(size_t i = 0; i < this->creatorCollectionButtons.size(); i++) {
        auto &children = this->creatorCollectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), this->sortingComparator);
        this->creatorCollectionButtons[i]->setChildren(children);
    }
    for(size_t i = 0; i < this->dateaddedCollectionButtons.size(); i++) {
        auto &children = this->dateaddedCollectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), this->sortingComparator);
        this->dateaddedCollectionButtons[i]->setChildren(children);
    }
    for(size_t i = 0; i < this->lengthCollectionButtons.size(); i++) {
        auto &children = this->lengthCollectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), this->sortingComparator);
        this->lengthCollectionButtons[i]->setChildren(children);
    }
    for(size_t i = 0; i < this->titleCollectionButtons.size(); i++) {
        auto &children = this->titleCollectionButtons[i]->getChildren();
        std::sort(children.begin(), children.end(), this->sortingComparator);
        this->titleCollectionButtons[i]->setChildren(children);
    }

    // we only need to update the visible buttons array if we are in No Grouping (because Collections always get sorted
    // by the collection name on the first level)
    if(this->group == GROUP::GROUP_NO_GROUPING) {
        this->visibleSongButtons.clear();
        this->visibleSongButtons.insert(this->visibleSongButtons.end(), this->songButtons.begin(),
                                        this->songButtons.end());
    }

    this->rebuildSongButtons();
    this->onAfterSortingOrGroupChange();
}

void SongBrowser::onGroupNoGrouping() {
    this->group = GROUP::GROUP_NO_GROUPING;
    this->groupByNothingBtn->setTextBrightColor(highlightColor);

    this->visibleSongButtons.clear();
    this->visibleSongButtons.insert(this->visibleSongButtons.end(), this->songButtons.begin(), this->songButtons.end());

    this->rebuildSongButtons();
    this->onAfterSortingOrGroupChange();
}

void SongBrowser::onGroupCollections(bool  /*autoScroll*/) {
    this->group = GROUP::GROUP_COLLECTIONS;
    this->groupByCollectionBtn->setTextBrightColor(highlightColor);

    this->visibleSongButtons.clear();
    this->visibleSongButtons.insert(this->visibleSongButtons.end(), this->collectionButtons.begin(),
                                    this->collectionButtons.end());

    this->rebuildSongButtons();
    this->onAfterSortingOrGroupChange();
}

void SongBrowser::onGroupArtist() {
    this->group = GROUP::GROUP_ARTIST;
    this->groupByArtistBtn->setTextBrightColor(highlightColor);

    this->visibleSongButtons.clear();
    this->visibleSongButtons.insert(this->visibleSongButtons.end(), this->artistCollectionButtons.begin(),
                                    this->artistCollectionButtons.end());

    this->rebuildSongButtons();
    this->onAfterSortingOrGroupChange();
}

void SongBrowser::onGroupDifficulty() {
    this->group = GROUP::GROUP_DIFFICULTY;
    this->groupByDifficultyBtn->setTextBrightColor(highlightColor);

    this->visibleSongButtons.clear();
    this->visibleSongButtons.insert(this->visibleSongButtons.end(), this->difficultyCollectionButtons.begin(),
                                    this->difficultyCollectionButtons.end());

    this->rebuildSongButtons();
    this->onAfterSortingOrGroupChange();
}

void SongBrowser::onGroupBPM() {
    this->group = GROUP::GROUP_BPM;

    this->visibleSongButtons.clear();
    this->visibleSongButtons.insert(this->visibleSongButtons.end(), this->bpmCollectionButtons.begin(),
                                    this->bpmCollectionButtons.end());

    this->rebuildSongButtons();
    this->onAfterSortingOrGroupChange();
}

void SongBrowser::onGroupCreator() {
    this->group = GROUP::GROUP_CREATOR;

    this->visibleSongButtons.clear();
    this->visibleSongButtons.insert(this->visibleSongButtons.end(), this->creatorCollectionButtons.begin(),
                                    this->creatorCollectionButtons.end());

    this->rebuildSongButtons();
    this->onAfterSortingOrGroupChange();
}

void SongBrowser::onGroupDateadded() {
    this->group = GROUP::GROUP_DATEADDED;

    this->visibleSongButtons.clear();
    this->visibleSongButtons.insert(this->visibleSongButtons.end(), this->dateaddedCollectionButtons.begin(),
                                    this->dateaddedCollectionButtons.end());

    this->rebuildSongButtons();
    this->onAfterSortingOrGroupChange();
}

void SongBrowser::onGroupLength() {
    this->group = GROUP::GROUP_LENGTH;

    this->visibleSongButtons.clear();
    this->visibleSongButtons.insert(this->visibleSongButtons.end(), this->lengthCollectionButtons.begin(),
                                    this->lengthCollectionButtons.end());

    this->rebuildSongButtons();
    this->onAfterSortingOrGroupChange();
}

void SongBrowser::onGroupTitle() {
    this->group = GROUP::GROUP_TITLE;

    this->visibleSongButtons.clear();
    this->visibleSongButtons.insert(this->visibleSongButtons.end(), this->titleCollectionButtons.begin(),
                                    this->titleCollectionButtons.end());

    this->rebuildSongButtons();
    this->onAfterSortingOrGroupChange();
}

void SongBrowser::onAfterSortingOrGroupChange() {
    // keep search state consistent between tab changes
    if(this->bInSearch) this->onSearchUpdate();

    // (can't call it right here because we maybe have async)
    this->scheduled_scroll_to_selected_button = true;
}

void SongBrowser::onSelectionMode() {
    if(cv::mod_fposu.getBool()) {
        cv::mod_fposu.setValue(false);
        osu->getNotificationOverlay()->addToast("Enabled FPoSu mode.", 0xff00ff00);
    } else {
        cv::mod_fposu.setValue(true);
        osu->getNotificationOverlay()->addToast("Disabled FPoSu mode.", 0xff00ff00);
    }
}

void SongBrowser::onSelectionMods() {
    soundEngine->play(osu->getSkin()->expand);
    osu->toggleModSelection(this->bF1Pressed);
}

void SongBrowser::onSelectionRandom() {
    soundEngine->play(osu->getSkin()->clickButton);
    if(this->bShiftPressed)
        this->bPreviousRandomBeatmapScheduled = true;
    else
        this->bRandomBeatmapScheduled = true;
}

void SongBrowser::onSelectionOptions() {
    soundEngine->play(osu->getSkin()->clickButton);

    if(this->selectedButton != NULL) {
        this->scrollToSongButton(this->selectedButton);

        const Vector2 heuristicSongButtonPositionAfterSmoothScrollFinishes =
            (this->carousel->getPos() + this->carousel->getSize() / 2);

        SongButton *songButtonPointer = dynamic_cast<SongButton *>(this->selectedButton);
        CollectionButton *collectionButtonPointer = dynamic_cast<CollectionButton *>(this->selectedButton);
        if(songButtonPointer != NULL) {
            songButtonPointer->triggerContextMenu(heuristicSongButtonPositionAfterSmoothScrollFinishes);
        } else if(collectionButtonPointer != NULL) {
            collectionButtonPointer->triggerContextMenu(heuristicSongButtonPositionAfterSmoothScrollFinishes);
        }
    }
}

void SongBrowser::onModeChange(const UString& text) { this->onModeChange2(text); }

void SongBrowser::onModeChange2(const UString& text, int id) { cv::mod_fposu.setValue(id == 2 || text == UString("fposu")); }

void SongBrowser::onScoreClicked(CBaseUIButton *button) {
    ScoreButton *scoreButton = (ScoreButton *)button;

    // NOTE: the order of these two calls matters
    osu->getRankingScreen()->setScore(scoreButton->getScore());
    osu->getRankingScreen()->setBeatmapInfo(this->beatmap, this->beatmap->getSelectedDifficulty2());

    osu->getSongBrowser()->setVisible(false);
    osu->getRankingScreen()->setVisible(true);

    soundEngine->play(osu->getSkin()->menuHit);
}

void SongBrowser::onScoreContextMenu(ScoreButton *scoreButton, int id) {
    // NOTE: see ScoreButton::onContextMenu()

    if(id == 2) {
        db->deleteScore(scoreButton->map_hash, scoreButton->getScoreUnixTimestamp());

        this->rebuildScoreButtons();
        osu->userButton->updateUserStats();
    }
}

void SongBrowser::onSongButtonContextMenu(SongButton *songButton, const UString& text, int id) {
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
                CollectionManagementHelper::getBeatmapSetHashesForSongButton(songButton, db);
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
                for(size_t i = 0; i < this->collectionButtons.size(); i++) {
                    if(this->collectionButtons[i]->isSelected()) {
                        collectionName = this->collectionButtons[i]->getCollectionName();
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
                for(size_t i = 0; i < this->collectionButtons.size(); i++) {
                    if(this->collectionButtons[i]->isSelected()) {
                        collectionName = this->collectionButtons[i]->getCollectionName();
                        break;
                    }
                }
            }

            auto collection = get_or_create_collection(collectionName);
            const std::vector<MD5Hash> beatmapSetHashes =
                CollectionManagementHelper::getBeatmapSetHashesForSongButton(songButton, db);
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
                    CollectionManagementHelper::getBeatmapSetHashesForSongButton(songButton, db);
                for(size_t i = 0; i < beatmapSetHashes.size(); i++) {
                    collection->add_map(beatmapSetHashes[i]);
                }
                updateUIScheduled = true;
            }

            save_collections();
        }
    }

    if(updateUIScheduled) {
        const float prevScrollPosY = this->carousel->getRelPosY();  // usability
        const auto previouslySelectedCollectionName =
            (this->selectionPreviousCollectionButton != NULL
                 ? this->selectionPreviousCollectionButton->getCollectionName()
                 : "");  // usability
        {
            this->recreateCollectionsButtons();
            this->rebuildSongButtonsAndVisibleSongButtonsWithSearchMatchSupport(
                false, false);  // (last false = skipping rebuildSongButtons() here)
            this->onSortChangeInt(cv::songbrowser_sortingtype.getString().c_str(),
                                  false);  // (because this does the rebuildSongButtons())
        }
        if(previouslySelectedCollectionName.length() > 0) {
            for(size_t i = 0; i < this->collectionButtons.size(); i++) {
                if(this->collectionButtons[i]->getCollectionName() == previouslySelectedCollectionName) {
                    this->collectionButtons[i]->select();
                    this->carousel->scrollToY(prevScrollPosY, false);
                    break;
                }
            }
        }
    }
}

void SongBrowser::onCollectionButtonContextMenu(CollectionButton * /*collectionButton*/, const UString& text, int id) {
    std::string collection_name = text.toUtf8();

    if(id == 2) {  // delete collection
        for(size_t i = 0; i < this->collectionButtons.size(); i++) {
            if(this->collectionButtons[i]->getCollectionName() == collection_name) {
                // delete UI
                delete this->collectionButtons[i];
                this->collectionButtons.erase(this->collectionButtons.begin() + i);

                // reset UI state
                this->selectionPreviousCollectionButton = NULL;

                auto collection = get_or_create_collection(collection_name);
                collection->delete_collection();
                save_collections();

                // update UI
                this->onGroupCollections(false);

                break;
            }
        }
    } else if(id == 3) {  // collection has been renamed
        // update UI
        this->onSortChangeInt(cv::songbrowser_sortingtype.getString().c_str(), false);
    }
}

void SongBrowser::highlightScore(u64 unixTimestamp) {
    for(size_t i = 0; i < this->scoreButtonCache.size(); i++) {
        if(this->scoreButtonCache[i]->getScore().unixTimestamp == unixTimestamp) {
            this->scoreBrowser->scrollToElement(this->scoreButtonCache[i], 0, 10);
            this->scoreButtonCache[i]->highlight();
            break;
        }
    }
}

void SongBrowser::recalculateStarsForSelectedBeatmap(bool  /*force*/) {
    if(this->beatmap->getSelectedDifficulty2() == NULL) return;

    this->beatmap->getSelectedDifficulty2()->pp.pp = -1.0;
}

void SongBrowser::selectSongButton(Button *songButton) {
    if(songButton != NULL && !songButton->isSelected()) {
        this->contextMenu->setVisible2(false);
        songButton->select();
    }
}

void SongBrowser::selectRandomBeatmap() {
    // filter songbuttons or independent diffs
    const std::vector<CBaseUIElement *> &elements = this->carousel->getContainer()->getElements();
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
    if(this->previousRandomBeatmaps.size() == 0 && this->beatmap != NULL &&
       this->beatmap->getSelectedDifficulty2() != NULL) {
        this->previousRandomBeatmaps.push_back(this->beatmap->getSelectedDifficulty2());
    }

    std::uniform_int_distribution<size_t> rng(0, songButtons.size() - 1);
    size_t randomIndex = rng(this->rngalg);

    SongButton *songButton = dynamic_cast<SongButton *>(songButtons[randomIndex]);
    this->selectSongButton(songButton);
}

void SongBrowser::selectPreviousRandomBeatmap() {
    if(this->previousRandomBeatmaps.size() > 0) {
        DatabaseBeatmap *currentRandomBeatmap = this->previousRandomBeatmaps.back();
        if(this->previousRandomBeatmaps.size() > 1 && this->beatmap != NULL &&
           this->previousRandomBeatmaps[this->previousRandomBeatmaps.size() - 1] ==
               this->beatmap->getSelectedDifficulty2())
            this->previousRandomBeatmaps.pop_back();  // deletes the current beatmap which may also be at the top (so
                                                      // we don't switch to ourself)

        // filter songbuttons
        const std::vector<CBaseUIElement *> &elements = this->carousel->getContainer()->getElements();
        std::vector<SongButton *> songButtons;
        for(size_t i = 0; i < elements.size(); i++) {
            SongButton *songButtonPointer = dynamic_cast<SongButton *>(elements[i]);

            if(songButtonPointer != NULL)  // allow ALL songbuttons
                songButtons.push_back(songButtonPointer);
        }

        // select it, if we can find it (and remove it from memory)
        bool foundIt = false;
        const DatabaseBeatmap *previousRandomBeatmap = this->previousRandomBeatmaps.back();
        for(size_t i = 0; i < songButtons.size(); i++) {
            if(songButtons[i]->getDatabaseBeatmap() != NULL &&
               songButtons[i]->getDatabaseBeatmap() == previousRandomBeatmap) {
                this->previousRandomBeatmaps.pop_back();
                this->selectSongButton(songButtons[i]);
                foundIt = true;
                break;
            }

            const auto &children = songButtons[i]->getChildren();
            for(size_t c = 0; c < children.size(); c++) {
                if(children[c]->getDatabaseBeatmap() == previousRandomBeatmap) {
                    this->previousRandomBeatmaps.pop_back();
                    this->selectSongButton(children[c]);
                    foundIt = true;
                    break;
                }
            }

            if(foundIt) break;
        }

        // if we didn't find it then restore the current random beatmap, which got pop_back()'d above (shit logic)
        if(!foundIt) this->previousRandomBeatmaps.push_back(currentRandomBeatmap);
    }
}

void SongBrowser::playSelectedDifficulty() {
    const std::vector<CBaseUIElement *> &elements = this->carousel->getContainer()->getElements();
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
        this->selectionPreviousCollectionButton = NULL;
        for(size_t i = 0; i < this->collectionButtons.size(); i++) {
            delete this->collectionButtons[i];
        }
        this->collectionButtons.clear();

        // sanity
        if(this->group == GROUP::GROUP_COLLECTIONS) {
            this->carousel->getContainer()->empty();
            this->visibleSongButtons.clear();
        }
    }

    Timer t;
    t.start();

    for(auto collection : collections) {
        if(collection->maps.empty()) continue;

        std::vector<SongButton *> folder;
        std::vector<u32> matched_sets;

        for(auto &map : collection->maps) {
            auto it = this->hashToSongButton.find(map);
            if(it == this->hashToSongButton.end()) continue;

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
            this->collectionButtons.push_back(new CollectionButton(this, this->carousel, this->contextMenu, 250,
                                                                   250 + this->beatmaps.size() * 50, 200, 50, "", uname,
                                                                   folder));
        }
    }

    t.update();
    debugLog("recreateCollectionsButtons(): %f seconds\n", t.getElapsedTime());
}
