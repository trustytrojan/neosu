#pragma once
#include "MouseListener.h"
#include "ScreenBackable.h"

class Beatmap;
class Database;
class DatabaseBeatmap;
class SkinImage;

class UIContextMenu;
class UISearchOverlay;
class UISelectionButton;
class InfoLabel;
class UserCard;
class ScoreButton;
class Button;
class SongButton;
class SongDifficultyButton;
class CollectionButton;

class CBaseUIContainer;
class CBaseUIImageButton;
class CBaseUIScrollView;
class CBaseUIButton;
class CBaseUILabel;

class McFont;
class ConVar;

class SongBrowserBackgroundSearchMatcher;

class SongBrowser : public ScreenBackable {
   public:
    static void drawSelectedBeatmapBackgroundImage(Graphics *g, float alpha = 1.0f);

    struct SORTING_COMPARATOR {
        virtual ~SORTING_COMPARATOR() { ; }
        virtual bool operator()(Button const *a, Button const *b) const = 0;
    };

    struct SortByArtist : public SORTING_COMPARATOR {
        virtual ~SortByArtist() { ; }
        virtual bool operator()(Button const *a, Button const *b) const;
    };

    struct SortByBPM : public SORTING_COMPARATOR {
        virtual ~SortByBPM() { ; }
        virtual bool operator()(Button const *a, Button const *b) const;
    };

    struct SortByCreator : public SORTING_COMPARATOR {
        virtual ~SortByCreator() { ; }
        virtual bool operator()(Button const *a, Button const *b) const;
    };

    struct SortByDateAdded : public SORTING_COMPARATOR {
        virtual ~SortByDateAdded() { ; }
        virtual bool operator()(Button const *a, Button const *b) const;
    };

    struct SortByDifficulty : public SORTING_COMPARATOR {
        virtual ~SortByDifficulty() { ; }
        virtual bool operator()(Button const *a, Button const *b) const;
    };

    struct SortByLength : public SORTING_COMPARATOR {
        virtual ~SortByLength() { ; }
        bool operator()(Button const *a, Button const *b) const;
    };

    struct SortByTitle : public SORTING_COMPARATOR {
        virtual ~SortByTitle() { ; }
        bool operator()(Button const *a, Button const *b) const;
    };

    enum class GROUP {
        GROUP_NO_GROUPING,
        GROUP_ARTIST,
        GROUP_BPM,
        GROUP_CREATOR,
        GROUP_DATEADDED,
        GROUP_DIFFICULTY,
        GROUP_LENGTH,
        GROUP_TITLE,
        GROUP_COLLECTIONS
    };

    friend class SongBrowserBackgroundSearchMatcher;

    SongBrowser();
    virtual ~SongBrowser();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    virtual void onKeyDown(KeyboardEvent &e);
    virtual void onKeyUp(KeyboardEvent &e);
    virtual void onChar(KeyboardEvent &e);

    virtual void onResolutionChange(Vector2 newResolution);

    virtual CBaseUIContainer *setVisible(bool visible);

    void selectSelectedBeatmapSongButton();
    void onPlayEnd(bool quit = true);  // called when a beatmap is finished playing (or the player quit)

    void onSelectionChange(Button *button, bool rebuild);
    void onDifficultySelected(DatabaseBeatmap *diff2, bool play = false);

    void onScoreContextMenu(ScoreButton *scoreButton, int id);
    void onSongButtonContextMenu(SongButton *songButton, UString text, int id);
    void onCollectionButtonContextMenu(CollectionButton *collectionButton, UString text, int id);

    void highlightScore(u64 unixTimestamp);
    void selectRandomBeatmap();
    void playNextRandomBeatmap() {
        selectRandomBeatmap();
        playSelectedDifficulty();
    }
    void recalculateStarsForSelectedBeatmap(bool force = false);

    void refreshBeatmaps();
    void addBeatmap(DatabaseBeatmap *beatmap);
    void readdBeatmap(DatabaseBeatmap *diff2);

    void requestNextScrollToSongButtonJumpFix(SongDifficultyButton *diffButton);
    void scrollToSongButton(Button *songButton, bool alignOnTop = false);
    void scrollToSelectedSongButton();
    void rebuildSongButtons();
    void recreateCollectionsButtons();
    void rebuildScoreButtons();
    void updateSongButtonLayout();
    void updateSongButtonSorting();

    Button *findCurrentlySelectedSongButton() const;
    inline const std::vector<CollectionButton *> &getCollectionButtons() const { return m_collectionButtons; }

    inline bool hasSelectedAndIsPlaying() const { return m_bHasSelectedAndIsPlaying; }
    inline bool isInSearch() const { return m_bInSearch; }
    inline bool isRightClickScrolling() const { return m_bSongBrowserRightClickScrolling; }

    inline Database *getDatabase() const { return m_db; }
    inline Beatmap *getSelectedBeatmap() const { return m_selectedBeatmap; }

    inline InfoLabel *getInfoLabel() { return m_songInfo; }

    inline GROUP getGroupingMode() const { return m_group; }

    enum class SORT {
        SORT_ARTIST,
        SORT_BPM,
        SORT_CREATOR,
        SORT_DATEADDED,
        SORT_DIFFICULTY,
        SORT_LENGTH,
        SORT_RANKACHIEVED,
        SORT_TITLE
    };

    struct SORTING_METHOD {
        SORT type;
        UString name;
        SORTING_COMPARATOR *comparator;
    };

    struct GROUPING {
        GROUP type;
        UString name;
        int id;
    };

    static bool searchMatcher(const DatabaseBeatmap *databaseBeatmap, const std::vector<UString> &searchStringTokens);
    static bool findSubstringInDifficulty(const DatabaseBeatmap *diff, const UString &searchString);

    virtual void updateLayout();
    virtual void onBack();

    void updateScoreBrowserLayout();

    void scheduleSearchUpdate(bool immediately = false);

    void checkHandleKillBackgroundSearchMatcher();

    UISelectionButton *addBottombarNavButton(std::function<SkinImage *()> getImageFunc,
                                             std::function<SkinImage *()> getImageOverFunc);
    CBaseUIButton *addTopBarRightTabButton(UString text);
    CBaseUIButton *addTopBarRightGroupButton(UString text);
    CBaseUIButton *addTopBarRightSortButton(UString text);
    CBaseUIButton *addTopBarLeftTabButton(UString text);
    CBaseUIButton *addTopBarLeftButton(UString text);

    void onDatabaseLoadingFinished();

    void onSearchUpdate();
    void rebuildSongButtonsAndVisibleSongButtonsWithSearchMatchSupport(bool scrollToTop,
                                                                       bool doRebuildSongButtons = true);

    void onSortScoresClicked(CBaseUIButton *button);
    void onSortScoresChange(UString text, int id = -1);
    void onWebClicked(CBaseUIButton *button);

    void onGroupClicked(CBaseUIButton *button);
    void onGroupChange(UString text, int id = -1);

    void onSortClicked(CBaseUIButton *button);
    void onSortChange(UString text, int id = -1);
    void onSortChangeInt(UString text, bool autoScroll);

    void onGroupTabButtonClicked(CBaseUIButton *groupTabButton);
    void onGroupNoGrouping();
    void onGroupCollections(bool autoScroll = true);
    void onGroupArtist();
    void onGroupDifficulty();
    void onGroupBPM();
    void onGroupCreator();
    void onGroupDateadded();
    void onGroupLength();
    void onGroupTitle();

    void onAfterSortingOrGroupChange(bool autoScroll = true);
    void onAfterSortingOrGroupChangeUpdateInt(bool autoScroll);

    void onSelectionMode();
    void onSelectionMods();
    void onSelectionRandom();
    void onSelectionOptions();

    void onModeChange(UString text);
    void onModeChange2(UString text, int id = -1);

    void onUserCardChange(UString new_username);

    void onScoreClicked(CBaseUIButton *button);

    void selectSongButton(Button *songButton);
    void selectPreviousRandomBeatmap();
    void playSelectedDifficulty();

    ConVar *m_fps_max_ref;
    ConVar *m_osu_scores_enabled;
    ConVar *m_name_ref;

    ConVar *m_osu_draw_scrubbing_timeline_strain_graph_ref;
    ConVar *m_osu_hud_scrubbing_timeline_strains_height_ref;
    ConVar *m_osu_hud_scrubbing_timeline_strains_alpha_ref;
    ConVar *m_osu_hud_scrubbing_timeline_strains_aim_color_r_ref;
    ConVar *m_osu_hud_scrubbing_timeline_strains_aim_color_g_ref;
    ConVar *m_osu_hud_scrubbing_timeline_strains_aim_color_b_ref;
    ConVar *m_osu_hud_scrubbing_timeline_strains_speed_color_r_ref;
    ConVar *m_osu_hud_scrubbing_timeline_strains_speed_color_g_ref;
    ConVar *m_osu_hud_scrubbing_timeline_strains_speed_color_b_ref;

    ConVar *m_osu_draw_statistics_perfectpp_ref;
    ConVar *m_osu_draw_statistics_totalstars_ref;

    ConVar *m_osu_mod_fposu_ref;

    GROUP m_group;
    std::vector<GROUPING> m_groupings;
    SORT m_sortingMethod;
    std::vector<SORTING_METHOD> m_sortingMethods;

    // top bar
    float m_fSongSelectTopScale;

    // top bar left
    CBaseUIContainer *m_topbarLeft;
    InfoLabel *m_songInfo;
    std::vector<CBaseUIButton *> m_topbarLeftTabButtons;
    std::vector<CBaseUIButton *> m_topbarLeftButtons;
    CBaseUIButton *m_scoreSortButton;
    CBaseUIButton *m_webButton;

    // top bar right
    CBaseUIContainer *m_topbarRight;
    std::vector<CBaseUIButton *> m_topbarRightTabButtons;
    std::vector<CBaseUIButton *> m_topbarRightGroupButtons;
    std::vector<CBaseUIButton *> m_topbarRightSortButtons;
    CBaseUILabel *m_groupLabel;
    CBaseUIButton *m_groupButton;
    CBaseUIButton *m_noGroupingButton;
    CBaseUIButton *m_collectionsButton;
    CBaseUIButton *m_artistButton;
    CBaseUIButton *m_difficultiesButton;
    CBaseUILabel *m_sortLabel;
    CBaseUIButton *m_sortButton;
    UIContextMenu *m_contextMenu;

    // bottom bar
    CBaseUIContainer *m_bottombar;
    std::vector<UISelectionButton *> m_bottombarNavButtons;
    UserCard *m_userButton = NULL;

    // score browser
    std::vector<ScoreButton *> m_scoreButtonCache;
    CBaseUIScrollView *m_scoreBrowser;
    CBaseUIElement *m_scoreBrowserScoresStillLoadingElement;
    CBaseUIElement *m_scoreBrowserNoRecordsYetElement;
    CBaseUIContainer *m_localBestContainer;
    CBaseUILabel *m_localBestLabel;
    ScoreButton *m_localBestButton = NULL;

    // song browser
    CBaseUIScrollView *m_songBrowser;
    bool m_bSongBrowserRightClickScrollCheck;
    bool m_bSongBrowserRightClickScrolling;
    bool m_bNextScrollToSongButtonJumpFixScheduled;
    bool m_bNextScrollToSongButtonJumpFixUseScrollSizeDelta;
    float m_fNextScrollToSongButtonJumpFixOldRelPosY;
    float m_fNextScrollToSongButtonJumpFixOldScrollSizeY;

    // song browser selection state logic
    SongButton *m_selectionPreviousSongButton;
    SongDifficultyButton *m_selectionPreviousSongDiffButton;
    CollectionButton *m_selectionPreviousCollectionButton;

    // beatmap database
    Database *m_db;
    std::vector<DatabaseBeatmap *> m_beatmaps;
    std::vector<SongButton *> m_songButtons;
    std::vector<Button *> m_visibleSongButtons;
    std::vector<CollectionButton *> m_collectionButtons;
    std::vector<CollectionButton *> m_artistCollectionButtons;
    std::vector<CollectionButton *> m_difficultyCollectionButtons;
    std::vector<CollectionButton *> m_bpmCollectionButtons;
    std::vector<CollectionButton *> m_creatorCollectionButtons;
    std::vector<CollectionButton *> m_dateaddedCollectionButtons;
    std::vector<CollectionButton *> m_lengthCollectionButtons;
    std::vector<CollectionButton *> m_titleCollectionButtons;
    std::unordered_map<MD5Hash, SongButton *> hashToSongButton;
    bool m_bBeatmapRefreshScheduled;
    UString m_sLastOsuFolder;

    // keys
    bool m_bF1Pressed;
    bool m_bF2Pressed;
    bool m_bF3Pressed;
    bool m_bShiftPressed;
    bool m_bLeft;
    bool m_bRight;
    bool m_bRandomBeatmapScheduled;
    bool m_bPreviousRandomBeatmapScheduled;

    // behaviour
    DatabaseBeatmap *m_lastSelectedBeatmap = NULL;
    Beatmap *m_selectedBeatmap;
    bool m_bHasSelectedAndIsPlaying;
    float m_fPulseAnimation;
    float m_fBackgroundFadeInTime;
    std::vector<DatabaseBeatmap *> m_previousRandomBeatmaps;

    // search
    UISearchOverlay *m_search;
    UString m_sSearchString;
    UString m_sPrevSearchString;
    UString m_sPrevHardcodedSearchString;
    float m_fSearchWaitTime;
    bool m_bInSearch;
    GROUP m_searchPrevGroup;
    SongBrowserBackgroundSearchMatcher *m_backgroundSearchMatcher;
    bool m_bOnAfterSortingOrGroupChangeUpdateScheduled;
    bool m_bOnAfterSortingOrGroupChangeUpdateScheduledAutoScroll;
};
