#pragma once
#include "App.h"
#include "ConVar.h"
#include "ModSelector.h"
#include "MouseListener.h"
#include "Replay.h"
#include "score.h"

class CWindowManager;

class VolumeOverlay;
class UserCard;
class Chat;
class Lobby;
class RoomScreen;
class PromptScreen;
class UIUserContextMenuScreen;
class MainMenu;
class PauseMenu;
class OptionsMenu;
class SongBrowser;
class SpectatorScreen;
class BackgroundImageHandler;
class RankingScreen;
class UpdateHandler;
class NotificationOverlay;
class TooltipOverlay;
class Beatmap;
class OsuScreen;
class Skin;
class HUD;
class Changelog;
class ModFPoSu;

class ConVar;
class Image;
class McFont;
class RenderTarget;

class Osu : public App, public MouseListener {
   public:
    static bool autoUpdater;

    static Vector2 osuBaseResolution;

    static float getImageScaleToFitResolution(Image *img, Vector2 resolution);
    static float getImageScaleToFitResolution(Vector2 size, Vector2 resolution);
    static float getImageScaleToFillResolution(Vector2 size, Vector2 resolution);
    static float getImageScaleToFillResolution(Image *img, Vector2 resolution);
    static float getImageScale(Vector2 size, float osuSize);
    static float getImageScale(Image *img, float osuSize);
    static float getUIScale(float osuResolutionRatio);
    static float getUIScale();  // NOTE: includes premultiplied dpi scale!

    Osu();
    ~Osu() override;

    void draw() override;
    void update() override;
	bool isInCriticalInteractiveSession() override;

    void onKeyDown(KeyboardEvent &e) override;
    void onKeyUp(KeyboardEvent &e) override;
    void onChar(KeyboardEvent &e) override;
    void stealFocus() override;

    void onLeftChange(bool down) override;
    void onMiddleChange(bool down) override { (void)down; }
    void onRightChange(bool down) override;

    void onWheelVertical(int delta) override { (void)delta; }
    void onWheelHorizontal(int delta) override { (void)delta; }

    void onResolutionChanged(Vector2 newResolution) override;
    void onDPIChanged() override;

    void onFocusGained() override;
    void onFocusLost() override;
    void onMinimized() override;
    bool onShutdown() override;

    void onPlayEnd(FinishedScore score, bool quit = true, bool aborted = false);

    void toggleModSelection(bool waitForF1KeyUp = false);
    void toggleSongBrowser();
    void toggleOptionsMenu();
    void toggleChangelog();
    void toggleEditor();

    void saveScreenshot();

    void reloadSkin() { this->onSkinReload(); }

    [[nodiscard]] inline Vector2 getScreenSize() const { return g_vInternalResolution; }
    [[nodiscard]] inline int getScreenWidth() const { return (int)g_vInternalResolution.x; }
    [[nodiscard]] inline int getScreenHeight() const { return (int)g_vInternalResolution.y; }

    Beatmap *getSelectedBeatmap();

    [[nodiscard]] inline OptionsMenu *getOptionsMenu() const { return this->optionsMenu; }
    [[nodiscard]] inline SongBrowser *getSongBrowser() const { return this->songBrowser2; }
    [[nodiscard]] inline BackgroundImageHandler *getBackgroundImageHandler() const { return this->backgroundImageHandler; }
    [[nodiscard]] inline Skin *getSkin() const { return this->skin; }
    [[nodiscard]] inline HUD *getHUD() const { return this->hud; }
    [[nodiscard]] inline NotificationOverlay *getNotificationOverlay() const { return this->notificationOverlay; }
    [[nodiscard]] inline TooltipOverlay *getTooltipOverlay() const { return this->tooltipOverlay; }
    [[nodiscard]] inline ModSelector *getModSelector() const { return this->modSelector; }
    [[nodiscard]] inline ModFPoSu *getFPoSu() const { return this->fposu; }
    [[nodiscard]] inline PauseMenu *getPauseMenu() const { return this->pauseMenu; }
    [[nodiscard]] inline MainMenu *getMainMenu() const { return this->mainMenu; }
    [[nodiscard]] inline RankingScreen *getRankingScreen() const { return this->rankingScreen; }
    [[nodiscard]] inline LiveScore *getScore() const { return this->score; }
    [[nodiscard]] inline UpdateHandler *getUpdateHandler() const { return this->updateHandler; }

    [[nodiscard]] inline RenderTarget *getPlayfieldBuffer() const { return this->playfieldBuffer; }
    [[nodiscard]] inline RenderTarget *getSliderFrameBuffer() const { return this->sliderFrameBuffer; }
    [[nodiscard]] inline RenderTarget *getFrameBuffer() const { return this->frameBuffer; }
    [[nodiscard]] inline RenderTarget *getFrameBuffer2() const { return this->frameBuffer2; }
    [[nodiscard]] inline McFont *getTitleFont() const { return this->titleFont; }
    [[nodiscard]] inline McFont *getSubTitleFont() const { return this->subTitleFont; }
    [[nodiscard]] inline McFont *getSongBrowserFont() const { return this->songBrowserFont; }
    [[nodiscard]] inline McFont *getSongBrowserFontBold() const { return this->songBrowserFontBold; }
    [[nodiscard]] inline McFont *getFontIcons() const { return this->fontIcons; }

    float getDifficultyMultiplier();
    float getCSDifficultyMultiplier();
    float getScoreMultiplier();
    float getAnimationSpeedMultiplier();

    [[nodiscard]] inline bool getModAuto() const { return cv::mod_autoplay.getBool(); }
    [[nodiscard]] inline bool getModAutopilot() const { return cv::mod_autopilot.getBool(); }
    [[nodiscard]] inline bool getModRelax() const { return cv::mod_relax.getBool(); }
    [[nodiscard]] inline bool getModSpunout() const { return cv::mod_spunout.getBool(); }
    [[nodiscard]] inline bool getModTarget() const { return cv::mod_target.getBool(); }
    [[nodiscard]] inline bool getModScorev2() const { return cv::mod_scorev2.getBool(); }
    [[nodiscard]] inline bool getModFlashlight() const { return cv::mod_flashlight.getBool(); }
    [[nodiscard]] inline bool getModNF() const { return cv::mod_nofail.getBool(); }
    [[nodiscard]] inline bool getModHD() const { return cv::mod_hidden.getBool(); }
    [[nodiscard]] inline bool getModHR() const { return cv::mod_hardrock.getBool(); }
    [[nodiscard]] inline bool getModEZ() const { return cv::mod_easy.getBool(); }
    [[nodiscard]] inline bool getModSD() const { return cv::mod_suddendeath.getBool(); }
    [[nodiscard]] inline bool getModSS() const { return cv::mod_perfect.getBool(); }
    [[nodiscard]] inline bool getModNightmare() const { return cv::mod_nightmare.getBool(); }
    [[nodiscard]] inline bool getModTD() const { return cv::mod_touchdevice.getBool() || cv::mod_touchdevice_always.getBool(); }

    [[nodiscard]] inline std::vector<ConVar *> getExperimentalMods() const { return this->experimentalMods; }

    bool isInPlayMode();
    [[nodiscard]] inline bool isSkinLoading() const { return this->bSkinLoadScheduled; }

    [[nodiscard]] inline bool isSkipScheduled() const { return this->bSkipScheduled; }
    [[nodiscard]] inline bool isSeeking() const { return this->bSeeking; }
    [[nodiscard]] inline float getQuickSaveTime() const { return this->fQuickSaveTime; }

    bool shouldFallBackToLegacySliderRenderer();  // certain mods or actions require Sliders to render dynamically
                                                  // (e.g. wobble or the CS override slider)

    void useMods(FinishedScore *score);
    void updateMods();
    void updateCursorVisibility();
    void updateConfineCursor();
    void updateMouseSettings();
    void updateWindowsKeyDisable();

    static Vector2 g_vInternalResolution;

    void updateModsForConVarTemplate(const UString &oldValue, const UString &newValue) {
        (void)oldValue;
        (void)newValue;
        this->updateMods();
    }

    void rebuildRenderTargets();
    void reloadFonts();
    void fireResolutionChanged();

    // callbacks
    void onWindowedResolutionChanged(const UString& oldValue, const UString& args);
    void onInternalResolutionChanged(const UString& oldValue, const UString& args);
    void onSensitivityChange(const UString &oldValue, const UString &newValue);
    void onRawInputChange(const UString &oldValue, const UString &newValue);

    void onSkinReload();
    void onSkinChange(const UString &newValue);
    void onAnimationSpeedChange();
    void updateAnimationSpeed();

    void onSpeedChange(const UString &newValue);
    void onDTPresetChange();
    void onHTPresetChange();
    void onThumbnailsToggle();

    void onPlayfieldChange();

    void onUIScaleChange(const UString &oldValue, const UString &newValue);
    void onUIScaleToDPIChange(const UString &oldValue, const UString &newValue);
    void onLetterboxingChange(const UString &oldValue, const UString &newValue);

    void onKey1Change(bool pressed, bool mouse);
    void onKey2Change(bool pressed, bool mouse);

    void onModMafhamChange();
    void onModFPoSuChange();
    void onModFPoSu3DChange();
    void onModFPoSu3DSpheresChange();
    void onModFPoSu3DSpheresAAChange();

    void onLetterboxingOffsetChange();

    void onUserCardChange(const UString& new_username);

    void setupSoloud();

    // interfaces
    VolumeOverlay *volumeOverlay = NULL;
    MainMenu *mainMenu = NULL;
    OptionsMenu *optionsMenu = NULL;
    Chat *chat = NULL;
    Lobby *lobby = NULL;
    RoomScreen *room = NULL;
    PromptScreen *prompt = NULL;
    UIUserContextMenuScreen *user_actions = NULL;
    SongBrowser *songBrowser2 = NULL;
    BackgroundImageHandler *backgroundImageHandler = NULL;
    ModSelector *modSelector = NULL;
    RankingScreen *rankingScreen = NULL;
    PauseMenu *pauseMenu = NULL;
    Skin *skin = NULL;
    HUD *hud = NULL;
    TooltipOverlay *tooltipOverlay = NULL;
    NotificationOverlay *notificationOverlay = NULL;
    LiveScore *score = NULL;
    Changelog *changelog = NULL;
    UpdateHandler *updateHandler = NULL;
    ModFPoSu *fposu = NULL;
    SpectatorScreen *spectatorScreen = NULL;

    UserCard *userButton = NULL;

    std::vector<OsuScreen *> screens;

    // rendering
    RenderTarget *backBuffer;
    RenderTarget *playfieldBuffer;
    RenderTarget *sliderFrameBuffer;
    RenderTarget *frameBuffer;
    RenderTarget *frameBuffer2;
    Vector2 vInternalResolution;
    Vector2 flashlight_position;

    // mods
    Replay::Mods previous_mods;
    bool bModAutoTemp = false;  // when ctrl+clicking a map, the auto mod should disable itself after the map finishes

    std::vector<ConVar *> experimentalMods;

    // keys
    bool bF1;
    bool bUIToggleCheck;
    bool bScoreboardToggleCheck;
    bool bEscape;
    bool bKeyboardKey1Down;
    bool bKeyboardKey12Down;
    bool bKeyboardKey2Down;
    bool bKeyboardKey22Down;
    bool bMouseKey1Down;
    bool bMouseKey2Down;
    bool bSkipScheduled;
    bool bQuickRetryDown;
    float fQuickRetryTime;
    bool bSeekKey;
    bool bSeeking;
    bool bClickedSkipButton = false;
    float fPrevSeekMousePosX;
    float fQuickSaveTime;

    // async toggles
    // TODO: this way of doing things is bullshit
    bool bToggleModSelectionScheduled;
    bool bToggleSongBrowserScheduled;
    bool bToggleOptionsMenuScheduled;
    bool bOptionsMenuFullscreen;
    bool bToggleChangelogScheduled;
    bool bToggleEditorScheduled;

    // cursor
    bool bShouldCursorBeVisible;

    // global resources
    std::vector<McFont *> fonts;
    McFont *titleFont;
    McFont *subTitleFont;
    McFont *songBrowserFont;
    McFont *songBrowserFontBold;
    McFont *fontIcons;

    // debugging
    CWindowManager *windowManager;

    // replay
    UString watched_user_name;
    i32 watched_user_id = 0;

    // custom
    bool music_unpause_scheduled = false;
    bool bScheduleEndlessModNextBeatmap;
    int iMultiplayerClientNumEscPresses;
    bool bWasBossKeyPaused;
    bool bSkinLoadScheduled;
    bool bSkinLoadWasReload;
    Skin *skinScheduledToLoad;
    bool bFontReloadScheduled;
    bool bFireResolutionChangedScheduled;
    bool bFireDelayedFontReloadAndResolutionChangeToFixDesyncedUIScaleScheduled;
    std::atomic<bool> should_pause_background_threads = false;
};

extern Osu *osu;
