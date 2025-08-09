#pragma once
// Copyright (c) 2015, PG, All rights reserved.
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
class UserStatsScreen;
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

class Osu final : public MouseListener, public KeyboardListener {
   public:
    static constexpr const Vector2 osuBaseResolution{640.0f, 480.0f};

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

    Osu &operator=(const Osu &) = delete;
    Osu &operator=(Osu &&) = delete;
    Osu(const Osu &) = delete;
    Osu(Osu &&) = delete;

    void draw();
    void update();
    bool isInPlayModeAndNotPaused();

    void onKeyDown(KeyboardEvent &e) override;
    void onKeyUp(KeyboardEvent &e) override;
    void onChar(KeyboardEvent &e) override;
    void stealFocus();

    void onButtonChange(ButtonIndex button, bool down) override;

    void onResolutionChanged(Vector2 newResolution);
    void onDPIChanged();

    void onFocusGained();
    void onFocusLost();
    inline void onRestored() { ; }
    void onMinimized();
    bool onShutdown();

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
    [[nodiscard]] inline BackgroundImageHandler *getBackgroundImageHandler() const {
        return this->backgroundImageHandler;
    }
    [[nodiscard]] inline Skin *getSkin() const { return this->skin; }
    [[nodiscard]] inline HUD *getHUD() const { return this->hud; }
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
    [[nodiscard]] inline bool getModTD() const {
        return cv::mod_touchdevice.getBool() || cv::mod_touchdevice_always.getBool();
    }

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
    void updateOsuFolder();
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
    void onWindowedResolutionChanged(const UString &oldValue, const UString &args);
    void onInternalResolutionChanged(const UString &oldValue, const UString &args);

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

    void onUserCardChange(const UString &new_username);

    void setupSoloud();

    // interfaces
    VolumeOverlay *volumeOverlay = nullptr;
    MainMenu *mainMenu = nullptr;
    OptionsMenu *optionsMenu = nullptr;
    Chat *chat = nullptr;
    Lobby *lobby = nullptr;
    RoomScreen *room = nullptr;
    PromptScreen *prompt = nullptr;
    UIUserContextMenuScreen *user_actions = nullptr;
    SongBrowser *songBrowser2 = nullptr;
    BackgroundImageHandler *backgroundImageHandler = nullptr;
    ModSelector *modSelector = nullptr;
    RankingScreen *rankingScreen = nullptr;
    UserStatsScreen *userStats = nullptr;
    PauseMenu *pauseMenu = nullptr;
    Skin *skin = nullptr;
    HUD *hud = nullptr;
    TooltipOverlay *tooltipOverlay = nullptr;
    NotificationOverlay *notificationOverlay = nullptr;
    LiveScore *score = nullptr;
    Changelog *changelog = nullptr;
    UpdateHandler *updateHandler = nullptr;
    ModFPoSu *fposu = nullptr;
    SpectatorScreen *spectatorScreen = nullptr;

    UserCard *userButton = nullptr;

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
    bool bToggleOptionsMenuScheduled;
    bool bOptionsMenuFullscreen;
    bool bToggleChangelogScheduled;
    bool bToggleEditorScheduled;

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
