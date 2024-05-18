//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		yet another ouendan clone, because why not
//
// $NoKeywords: $osu
//===============================================================================//

#ifndef OSU_H
#define OSU_H

#include "App.h"
#include "BanchoNetworking.h"
#include "MouseListener.h"
#include "score.h"

class CWindowManager;

class VolumeOverlay;
class Chat;
class Lobby;
class RoomScreen;
class PromptScreen;
class UIUserContextMenuScreen;
class MainMenu;
class PauseMenu;
class OptionsMenu;
class ModSelector;
class SongBrowser;
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

class Osu : public App, public MouseListener {
   public:
    static ConVar *version;
    static ConVar *debug;
    static ConVar *ui_scale;
    static bool autoUpdater;

    static Vector2 osuBaseResolution;

    static float getImageScaleToFitResolution(Image *img, Vector2 resolution);
    static float getImageScaleToFitResolution(Vector2 size, Vector2 resolution);
    static float getImageScaleToFillResolution(Vector2 size, Vector2 resolution);
    static float getImageScaleToFillResolution(Image *img, Vector2 resolution);
    static float getImageScale(Osu *osu, Vector2 size, float osuSize);
    static float getImageScale(Osu *osu, Image *img, float osuSize);
    static float getUIScale(Osu *osu, float osuResolutionRatio);
    static float getUIScale(Osu *osu);  // NOTE: includes premultiplied dpi scale!

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

    void onPlayStart();  // called when a beatmap has successfully started playing
    void onPlayEnd(bool quit = true,
                   bool aborted = false);  // called when a beatmap is finished playing (or the player quit)

    void toggleModSelection(bool waitForF1KeyUp = false);
    void toggleSongBrowser();
    void toggleOptionsMenu();
    void toggleUserStatsScreen();
    void toggleChangelog();
    void toggleEditor();

    void saveScreenshot();

    void setSkin(UString skin) { onSkinChange("", skin); }
    void reloadSkin() { onSkinReload(); }

    inline Vector2 getScreenSize() const { return g_vInternalResolution; }
    inline int getScreenWidth() const { return (int)g_vInternalResolution.x; }
    inline int getScreenHeight() const { return (int)g_vInternalResolution.y; }

    Beatmap *getSelectedBeatmap();

    inline OptionsMenu *getOptionsMenu() const { return m_optionsMenu; }
    inline SongBrowser *getSongBrowser() const { return m_songBrowser2; }
    inline BackgroundImageHandler *getBackgroundImageHandler() const { return m_backgroundImageHandler; }
    inline Skin *getSkin() const { return m_skin; }
    inline HUD *getHUD() const { return m_hud; }
    inline NotificationOverlay *getNotificationOverlay() const { return m_notificationOverlay; }
    inline TooltipOverlay *getTooltipOverlay() const { return m_tooltipOverlay; }
    inline ModSelector *getModSelector() const { return m_modSelector; }
    inline ModFPoSu *getFPoSu() const { return m_fposu; }
    inline PauseMenu *getPauseMenu() const { return m_pauseMenu; }
    inline MainMenu *getMainMenu() const { return m_mainMenu; }
    inline RankingScreen *getRankingScreen() const { return m_rankingScreen; }
    inline LiveScore *getScore() const { return m_score; }
    inline UpdateHandler *getUpdateHandler() const { return m_updateHandler; }
    inline UserStatsScreen *getUserStatsScreen() const { return m_userStatsScreen; }

    inline RenderTarget *getPlayfieldBuffer() const { return m_playfieldBuffer; }
    inline RenderTarget *getSliderFrameBuffer() const { return m_sliderFrameBuffer; }
    inline RenderTarget *getFrameBuffer() const { return m_frameBuffer; }
    inline RenderTarget *getFrameBuffer2() const { return m_frameBuffer2; }
    inline McFont *getTitleFont() const { return m_titleFont; }
    inline McFont *getSubTitleFont() const { return m_subTitleFont; }
    inline McFont *getSongBrowserFont() const { return m_songBrowserFont; }
    inline McFont *getSongBrowserFontBold() const { return m_songBrowserFontBold; }
    inline McFont *getFontIcons() const { return m_fontIcons; }

    float getDifficultyMultiplier();
    float getCSDifficultyMultiplier();
    float getScoreMultiplier();
    float getRawSpeedMultiplier();  // without override
    float getSpeedMultiplier();     // with override
    float getAnimationSpeedMultiplier();

    inline bool getModAuto() const { return m_bModAuto; }
    inline bool getModAutopilot() const { return m_bModAutopilot; }
    inline bool getModRelax() const { return m_bModRelax; }
    inline bool getModSpunout() const { return m_bModSpunout; }
    inline bool getModTarget() const { return m_bModTarget; }
    inline bool getModScorev2() const { return m_bModScorev2; }
    inline bool getModFlashlight() const { return m_bModFlashlight; }
    inline bool getModDT() const { return m_bModDT; }
    inline bool getModNC() const { return m_bModNC; }
    inline bool getModNF() const { return m_bModNF; }
    inline bool getModHT() const { return m_bModHT; }
    inline bool getModDC() const { return m_bModDC; }
    inline bool getModHD() const { return m_bModHD; }
    inline bool getModHR() const { return m_bModHR; }
    inline bool getModEZ() const { return m_bModEZ; }
    inline bool getModSD() const { return m_bModSD; }
    inline bool getModSS() const { return m_bModSS; }
    inline bool getModNightmare() const { return m_bModNightmare; }
    inline bool getModTD() const { return m_bModTD; }

    inline std::vector<ConVar *> getExperimentalMods() const { return m_experimentalMods; }

    bool isInPlayMode();
    bool isNotInPlayModeOrPaused();
    inline bool isSkinLoading() const { return m_bSkinLoadScheduled; }

    inline bool isSkipScheduled() const { return m_bSkipScheduled; }
    inline bool isSeeking() const { return m_bSeeking; }
    inline float getQuickSaveTime() const { return m_fQuickSaveTime; }

    bool shouldFallBackToLegacySliderRenderer();  // certain mods or actions require Sliders to render dynamically
                                                  // (e.g. wobble or the CS override slider)

    bool useMods(FinishedScore *score);
    void updateMods();
    void updateConfineCursor();
    void updateMouseSettings();
    void updateWindowsKeyDisable();

    static Vector2 g_vInternalResolution;

    void updateModsForConVarTemplate(UString oldValue, UString newValue) {
        (void)oldValue;
        (void)newValue;
        updateMods();
    }

    void rebuildRenderTargets();
    void reloadFonts();
    void fireResolutionChanged();

    // callbacks
    void onInternalResolutionChanged(UString oldValue, UString args);

    void onSkinReload();
    void onSkinChange(UString oldValue, UString newValue);
    void onAnimationSpeedChange(UString oldValue, UString newValue);
    void updateAnimationSpeed();

    void onSpeedChange(UString oldValue, UString newValue);

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

    void onNotification(UString args);

    // convar refs
    ConVar *m_osu_folder_ref;
    ConVar *m_osu_folder_sub_skins_ref;
    ConVar *m_osu_draw_hud_ref;
    ConVar *m_osu_draw_scoreboard;
    ConVar *m_osu_draw_scoreboard_mp;
    ConVar *m_osu_draw_cursor_ripples_ref;
    ConVar *m_osu_mod_fps_ref;
    ConVar *m_osu_mod_wobble_ref;
    ConVar *m_osu_mod_wobble2_ref;
    ConVar *m_osu_mod_minimize_ref;
    ConVar *m_osu_playfield_rotation;
    ConVar *m_osu_playfield_stretch_x;
    ConVar *m_osu_playfield_stretch_y;
    ConVar *m_fposu_draw_cursor_trail_ref;
    ConVar *m_osu_mod_mafham_ref;
    ConVar *m_osu_mod_fposu_ref;
    ConVar *m_snd_change_check_interval_ref;
    ConVar *m_ui_scrollview_scrollbarwidth_ref;
    ConVar *m_mouse_raw_input_absolute_to_window_ref;
    ConVar *m_win_disable_windows_key_ref;

    // interfaces
    VolumeOverlay *m_volumeOverlay;
    MainMenu *m_mainMenu;
    OptionsMenu *m_optionsMenu;
    Chat *m_chat = nullptr;
    Lobby *m_lobby = nullptr;
    RoomScreen *m_room = nullptr;
    PromptScreen *m_prompt = nullptr;
    UIUserContextMenuScreen *m_user_actions = nullptr;
    SongBrowser *m_songBrowser2 = nullptr;
    BackgroundImageHandler *m_backgroundImageHandler;
    ModSelector *m_modSelector;
    RankingScreen *m_rankingScreen;
    UserStatsScreen *m_userStatsScreen;
    PauseMenu *m_pauseMenu;
    Skin *m_skin;
    HUD *m_hud;
    TooltipOverlay *m_tooltipOverlay;
    NotificationOverlay *m_notificationOverlay;
    LiveScore *m_score;
    Changelog *m_changelog;
    UpdateHandler *m_updateHandler;
    ModFPoSu *m_fposu;

    std::vector<OsuScreen *> m_screens;

    // rendering
    RenderTarget *m_backBuffer;
    RenderTarget *m_playfieldBuffer;
    RenderTarget *m_sliderFrameBuffer;
    RenderTarget *m_frameBuffer;
    RenderTarget *m_frameBuffer2;
    Vector2 m_vInternalResolution;
    Vector2 flashlight_position;
    bool holding_slider = false;

    // mods
    bool m_bModAuto;
    bool m_bModAutopilot;
    bool m_bModRelax;
    bool m_bModSpunout;
    bool m_bModTarget;
    bool m_bModScorev2;
    bool m_bModFlashlight = false;
    bool m_bModDT;
    bool m_bModNC;
    bool m_bModNF;
    bool m_bModHT;
    bool m_bModDC;
    bool m_bModHD;
    bool m_bModHR;
    bool m_bModEZ;
    bool m_bModSD;
    bool m_bModSS;
    bool m_bModNightmare;
    bool m_bModTD;

    std::vector<ConVar *> m_experimentalMods;

    // keys
    bool m_bF1;
    bool m_bUIToggleCheck;
    bool m_bScoreboardToggleCheck;
    bool m_bEscape;
    bool m_bKeyboardKey1Down;
    bool m_bKeyboardKey12Down;
    bool m_bKeyboardKey2Down;
    bool m_bKeyboardKey22Down;
    bool m_bMouseKey1Down;
    bool m_bMouseKey2Down;
    bool m_bSkipScheduled;
    bool m_bQuickRetryDown;
    float m_fQuickRetryTime;
    bool m_bSeekKey;
    bool m_bSeeking;
    bool m_bClickedSkipButton = false;
    float m_fPrevSeekMousePosX;
    float m_fQuickSaveTime;

    // async toggles
    // TODO: this way of doing things is bullshit
    bool m_bToggleModSelectionScheduled;
    bool m_bToggleSongBrowserScheduled;
    bool m_bToggleOptionsMenuScheduled;
    bool m_bOptionsMenuFullscreen;
    bool m_bToggleUserStatsScreenScheduled;
    bool m_bToggleChangelogScheduled;
    bool m_bToggleEditorScheduled;

    // cursor
    bool m_bShouldCursorBeVisible;

    // global resources
    std::vector<McFont *> m_fonts;
    McFont *m_titleFont;
    McFont *m_subTitleFont;
    McFont *m_songBrowserFont;
    McFont *m_songBrowserFontBold;
    McFont *m_fontIcons;

    // debugging
    CWindowManager *m_windowManager;

    // replay
    FinishedScore replay_score;

    // custom
    bool m_bScheduleEndlessModNextBeatmap;
    int m_iMultiplayerClientNumEscPresses;
    bool m_bWasBossKeyPaused;
    bool m_bSkinLoadScheduled;
    bool m_bSkinLoadWasReload;
    Skin *m_skinScheduledToLoad;
    bool m_bFontReloadScheduled;
    bool m_bFireResolutionChangedScheduled;
    bool m_bFireDelayedFontReloadAndResolutionChangeToFixDesyncedUIScaleScheduled;
};

#endif
