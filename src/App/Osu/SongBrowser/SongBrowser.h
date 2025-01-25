#pragma once
#include "MouseListener.h"
#include "ScreenBackable.h"

class Beatmap;
class Database;
class DatabaseBeatmap;
typedef DatabaseBeatmap BeatmapDifficulty;
typedef DatabaseBeatmap BeatmapSet;
class SkinImage;

class UIContextMenu;
class UISearchOverlay;
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

typedef bool (*SORTING_COMPARATOR)(const SongButton *a, const SongButton *b);
bool sort_by_artist(const SongButton *a, const SongButton *b);
bool sort_by_bpm(const SongButton *a, const SongButton *b);
bool sort_by_creator(const SongButton *a, const SongButton *b);
bool sort_by_date_added(const SongButton *a, const SongButton *b);
bool sort_by_difficulty(const SongButton *a, const SongButton *b);
bool sort_by_length(const SongButton *a, const SongButton *b);
bool sort_by_title(const SongButton *a, const SongButton *b);

class SongBrowser : public ScreenBackable {
   public:
    static void drawSelectedBeatmapBackgroundImage(Graphics *g, float alpha = 1.0f);

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

    bool selectBeatmapset(i32 set_id);
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
        this->selectRandomBeatmap();
        this->playSelectedDifficulty();
    }
    void recalculateStarsForSelectedBeatmap(bool force = false);

    void refreshBeatmaps();
    void addBeatmapSet(BeatmapSet *beatmap);
    void addSongButtonToAlphanumericGroup(SongButton *btn, std::vector<CollectionButton *> &group,
                                          const std::string &name);

    void requestNextScrollToSongButtonJumpFix(SongDifficultyButton *diffButton);
    bool isButtonVisible(Button *songButton);
    void scrollToBestButton();
    void scrollToSongButton(Button *songButton, bool alignOnTop = false);
    void rebuildSongButtons();
    void recreateCollectionsButtons();
    void rebuildScoreButtons();
    void updateSongButtonLayout();

    inline const std::vector<CollectionButton *> &getCollectionButtons() const { return this->collectionButtons; }

    inline bool hasSelectedAndIsPlaying() const { return this->bHasSelectedAndIsPlaying; }
    inline bool isInSearch() const { return this->bInSearch; }
    inline bool isRightClickScrolling() const { return this->bSongBrowserRightClickScrolling; }

    inline Beatmap *getSelectedBeatmap() const { return this->beatmap; }

    inline InfoLabel *getInfoLabel() { return this->songInfo; }

    inline GROUP getGroupingMode() const { return this->group; }

    enum class SORT {
        SORT_ARTIST,
        SORT_BPM,
        SORT_CREATOR,
        SORT_DATEADDED,
        SORT_DIFFICULTY,
        SORT_LENGTH,
        SORT_TITLE,
        SORT_RANKACHIEVED,
    };

    struct SORTING_METHOD {
        SORT type;
        UString name;
        SORTING_COMPARATOR comparator;
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

    CBaseUIButton *addTopBarLeftTabButton(UString text);
    CBaseUIButton *addTopBarLeftButton(UString text);

    void onDatabaseLoadingFinished();

    void onSearchUpdate();
    void rebuildSongButtonsAndVisibleSongButtonsWithSearchMatchSupport(bool scrollToTop,
                                                                       bool doRebuildSongButtons = true);

    void onSortScoresClicked(CBaseUIButton *button);
    void onSortScoresChange(UString text, int id = -1);
    void onWebClicked(CBaseUIButton *button);

    void onQuickGroupClicked(CBaseUIButton *button);
    void onGroupClicked(CBaseUIButton *button);
    void onGroupChange(UString text, int id = -1);

    void onSortClicked(CBaseUIButton *button);
    void onSortChange(UString text, int id = -1);
    void onSortChangeInt(UString text, bool autoScroll);

    void onGroupNoGrouping();
    void onGroupCollections(bool autoScroll = true);
    void onGroupArtist();
    void onGroupDifficulty();
    void onGroupBPM();
    void onGroupCreator();
    void onGroupDateadded();
    void onGroupLength();
    void onGroupTitle();

    void onAfterSortingOrGroupChange();

    void onSelectionMode();
    void onSelectionMods();
    void onSelectionRandom();
    void onSelectionOptions();

    void onModeChange(UString text);
    void onModeChange2(UString text, int id = -1);

    void onScoreClicked(CBaseUIButton *button);

    void selectSongButton(Button *songButton);
    void selectPreviousRandomBeatmap();
    void playSelectedDifficulty();

    GROUP group;
    std::vector<GROUPING> groupings;

    SORTING_COMPARATOR sortingComparator;
    SORT sortingMethod;
    std::vector<SORTING_METHOD> sortingMethods;

    // top bar left
    CBaseUIContainer *topbarLeft;
    InfoLabel *songInfo;
    std::vector<CBaseUIButton *> topbarLeftTabButtons;
    std::vector<CBaseUIButton *> topbarLeftButtons;
    CBaseUIButton *scoreSortButton;
    CBaseUIButton *webButton;

    // top bar right
    CBaseUIContainer *topbarRight;
    CBaseUILabel *groupLabel;
    CBaseUIButton *groupButton;
    CBaseUILabel *sortLabel;
    CBaseUIButton *sortButton;
    UIContextMenu *contextMenu;

    CBaseUIButton *groupByCollectionBtn;
    CBaseUIButton *groupByArtistBtn;
    CBaseUIButton *groupByDifficultyBtn;
    CBaseUIButton *groupByNothingBtn;

    // score browser
    std::vector<ScoreButton *> scoreButtonCache;
    CBaseUIScrollView *scoreBrowser;
    CBaseUIElement *scoreBrowserScoresStillLoadingElement;
    CBaseUIElement *scoreBrowserNoRecordsYetElement;
    CBaseUIContainer *localBestContainer;
    CBaseUILabel *localBestLabel;
    ScoreButton *localBestButton = NULL;
    bool score_resort_scheduled = false;

    // song carousel
    CBaseUIScrollView *carousel;
    Button *selectedButton = NULL;
    bool bSongBrowserRightClickScrollCheck;
    bool bSongBrowserRightClickScrolling;
    bool bNextScrollToSongButtonJumpFixScheduled;
    bool bNextScrollToSongButtonJumpFixUseScrollSizeDelta;
    bool scheduled_scroll_to_selected_button = false;
    float fNextScrollToSongButtonJumpFixOldRelPosY;
    float fNextScrollToSongButtonJumpFixOldScrollSizeY;

    // song browser selection state logic
    SongButton *selectionPreviousSongButton;
    SongDifficultyButton *selectionPreviousSongDiffButton;
    CollectionButton *selectionPreviousCollectionButton;

    // beatmap database
    std::vector<DatabaseBeatmap *> beatmaps;
    std::vector<SongButton *> songButtons;
    std::vector<Button *> visibleSongButtons;
    std::vector<CollectionButton *> collectionButtons;
    std::vector<CollectionButton *> artistCollectionButtons;
    std::vector<CollectionButton *> difficultyCollectionButtons;
    std::vector<CollectionButton *> bpmCollectionButtons;
    std::vector<CollectionButton *> creatorCollectionButtons;
    std::vector<CollectionButton *> dateaddedCollectionButtons;
    std::vector<CollectionButton *> lengthCollectionButtons;
    std::vector<CollectionButton *> titleCollectionButtons;
    std::unordered_map<MD5Hash, SongButton *> hashToSongButton;
    bool bBeatmapRefreshScheduled;
    UString sLastOsuFolder;
    MD5Hash beatmap_to_reselect_after_db_load;

    // keys
    bool bF1Pressed;
    bool bF2Pressed;
    bool bF3Pressed;
    bool bShiftPressed;
    bool bLeft;
    bool bRight;
    bool bRandomBeatmapScheduled;
    bool bPreviousRandomBeatmapScheduled;

    // behaviour
    DatabaseBeatmap *lastSelectedBeatmap = NULL;
    Beatmap *beatmap;
    bool bHasSelectedAndIsPlaying;
    float fPulseAnimation;
    float fBackgroundFadeInTime;
    std::vector<DatabaseBeatmap *> previousRandomBeatmaps;

    // map auto-download
    i32 map_autodl = 0;
    i32 set_autodl = 0;

    // search
    UISearchOverlay *search;
    UString sSearchString;
    UString sPrevSearchString;
    UString sPrevHardcodedSearchString;
    float fSearchWaitTime;
    bool bInSearch;
    GROUP searchPrevGroup;
    SongBrowserBackgroundSearchMatcher *backgroundSearchMatcher;
};
