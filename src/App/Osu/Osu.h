#pragma once
#include "App.h"
#include "BanchoNetworking.h"
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

    static bool findIgnoreCase(const std::string &haystack, const std::string &needle);

    Osu();
    virtual ~Osu();

    virtual void draw(Graphics *g);
    virtual void update();

    virtual void onKeyDown(KeyboardEvent &e);
    virtual void onKeyUp(KeyboardEvent &e);
    virtual void onChar(KeyboardEvent &e);
    virtual void stealFocus();

    void onLeftChange(bool down);
    void onMiddleChange(bool down) { (void)down; }
    void onRightChange(bool down);

    void onWheelVertical(int delta) { (void)delta; }
    void onWheelHorizontal(int delta) { (void)delta; }

    virtual void onResolutionChanged(Vector2 newResolution);
    virtual void onDPIChanged();

    virtual void onFocusGained();
    virtual void onFocusLost();
    virtual void onMinimized();
    virtual bool onShutdown();

    void onPlayEnd(FinishedScore score, bool quit = true, bool aborted = false);

    void toggleModSelection(bool waitForF1KeyUp = false);
    void toggleSongBrowser();
    void toggleOptionsMenu();
    void toggleChangelog();
    void toggleEditor();

    void saveScreenshot();

    void reloadSkin() { this->onSkinReload(); }

    inline Vector2 getScreenSize() const { return g_vInternalResolution; }
    inline int getScreenWidth() const { return (int)g_vInternalResolution.x; }
    inline int getScreenHeight() const { return (int)g_vInternalResolution.y; }

    Beatmap *getSelectedBeatmap();

    inline OptionsMenu *getOptionsMenu() const { return this->optionsMenu; }
    inline SongBrowser *getSongBrowser() const { return this->songBrowser2; }
    inline BackgroundImageHandler *getBackgroundImageHandler() const { return this->backgroundImageHandler; }
    inline Skin *getSkin() const { return this->skin; }
    inline HUD *getHUD() const { return this->hud; }
    inline NotificationOverlay *getNotificationOverlay() const { return this->notificationOverlay; }
    inline TooltipOverlay *getTooltipOverlay() const { return this->tooltipOverlay; }
    inline ModSelector *getModSelector() const { return this->modSelector; }
    inline ModFPoSu *getFPoSu() const { return this->fposu; }
    inline PauseMenu *getPauseMenu() const { return this->pauseMenu; }
    inline MainMenu *getMainMenu() const { return this->mainMenu; }
    inline RankingScreen *getRankingScreen() const { return this->rankingScreen; }
    inline LiveScore *getScore() const { return this->score; }
    inline UpdateHandler *getUpdateHandler() const { return this->updateHandler; }

    inline RenderTarget *getPlayfieldBuffer() const { return this->playfieldBuffer; }
    inline RenderTarget *getSliderFrameBuffer() const { return this->sliderFrameBuffer; }
    inline RenderTarget *getFrameBuffer() const { return this->frameBuffer; }
    inline RenderTarget *getFrameBuffer2() const { return this->frameBuffer2; }
    inline McFont *getTitleFont() const { return this->titleFont; }
    inline McFont *getSubTitleFont() const { return this->subTitleFont; }
    inline McFont *getSongBrowserFont() const { return this->songBrowserFont; }
    inline McFont *getSongBrowserFontBold() const { return this->songBrowserFontBold; }
    inline McFont *getFontIcons() const { return this->fontIcons; }

    float getDifficultyMultiplier();
    float getCSDifficultyMultiplier();
    float getScoreMultiplier();
    float getAnimationSpeedMultiplier();

    inline bool getModAuto() const { return cv_mod_autoplay.getBool(); }
    inline bool getModAutopilot() const { return cv_mod_autopilot.getBool(); }
    inline bool getModRelax() const { return cv_mod_relax.getBool(); }
    inline bool getModSpunout() const { return cv_mod_spunout.getBool(); }
    inline bool getModTarget() const { return cv_mod_target.getBool(); }
    inline bool getModScorev2() const { return cv_mod_scorev2.getBool(); }
    inline bool getModFlashlight() const { return cv_mod_flashlight.getBool(); }
    inline bool getModNF() const { return cv_mod_nofail.getBool(); }
    inline bool getModHD() const { return cv_mod_hidden.getBool(); }
    inline bool getModHR() const { return cv_mod_hardrock.getBool(); }
    inline bool getModEZ() const { return cv_mod_easy.getBool(); }
    inline bool getModSD() const { return cv_mod_suddendeath.getBool(); }
    inline bool getModSS() const { return cv_mod_perfect.getBool(); }
    inline bool getModNightmare() const { return cv_mod_nightmare.getBool(); }
    inline bool getModTD() const { return cv_mod_touchdevice.getBool() || cv_mod_touchdevice_always.getBool(); }

    inline std::vector<ConVar *> getExperimentalMods() const { return this->experimentalMods; }

    bool isInPlayMode();
    bool isNotInPlayModeOrPaused();
    inline bool isSkinLoading() const { return this->bSkinLoadScheduled; }

    inline bool isSkipScheduled() const { return this->bSkipScheduled; }
    inline bool isSeeking() const { return this->bSeeking; }
    inline float getQuickSaveTime() const { return this->fQuickSaveTime; }

    bool shouldFallBackToLegacySliderRenderer();  // certain mods or actions require Sliders to render dynamically
                                                  // (e.g. wobble or the CS override slider)

    void useMods(FinishedScore *score);
    void updateMods();
    void updateConfineCursor();
    void updateMouseSettings();
    void updateWindowsKeyDisable();

    static Vector2 g_vInternalResolution;

    void updateModsForConVarTemplate(UString oldValue, UString newValue) {
        (void)oldValue;
        (void)newValue;
        this->updateMods();
    }

    void rebuildRenderTargets();
    void reloadFonts();
    void fireResolutionChanged();

    // callbacks
    void onWindowedResolutionChanged(UString oldValue, UString args);
    void onInternalResolutionChanged(UString oldValue, UString args);

    void onSkinReload();
    void onSkinChange(UString oldValue, UString newValue);
    void onAnimationSpeedChange(UString oldValue, UString newValue);
    void updateAnimationSpeed();

    void onSpeedChange(UString oldValue, UString newValue);
    void onDTPresetChange(UString oldValue, UString newValue);
    void onHTPresetChange(UString oldValue, UString newValue);
    void onThumbnailsToggle(UString oldValue, UString newValue);

    void onPlayfieldChange(UString oldValue, UString newValue);

    void onUIScaleChange(UString oldValue, UString newValue);
    void onUIScaleToDPIChange(UString oldValue, UString newValue);
    void onLetterboxingChange(UString oldValue, UString newValue);

    void onConfineCursorWindowedChange(UString oldValue, UString newValue);
    void onConfineCursorFullscreenChange(UString oldValue, UString newValue);

    void onKey1Change(bool pressed, bool mouse);
    void onKey2Change(bool pressed, bool mouse);

    void onModMafhamChange(UString oldValue, UString newValue);
    void onModFPoSuChange(UString oldValue, UString newValue);
    void onModFPoSu3DChange(UString oldValue, UString newValue);
    void onModFPoSu3DSpheresChange(UString oldValue, UString newValue);
    void onModFPoSu3DSpheresAAChange(UString oldValue, UString newValue);

    void onLetterboxingOffsetChange(UString oldValue, UString newValue);

    void onUserCardChange(UString new_username);

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
    u32 watched_user_id = 0;

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
