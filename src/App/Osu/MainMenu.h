//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		main menu
//
// $NoKeywords: $osumain
//===============================================================================//

#ifndef OSUMENUMAIN_H
#define OSUMENUMAIN_H

#include "CBaseUIButton.h"
#include "MouseListener.h"
#include "OsuScreen.h"

class McFont;
class Osu;

class Beatmap;
class DatabaseBeatmap;

class HitObject;

class MainMenuCubeButton;
class MainMenuButton;
class UIButton;

class CBaseUIContainer;

class ConVar;

class MainMenuPauseButton : public CBaseUIButton {
   public:
    MainMenuPauseButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text)
        : CBaseUIButton(xPos, yPos, xSize, ySize, name, text) {
        m_bIsPaused = true;
    }

    virtual void draw(Graphics *g);
    void setPaused(bool paused) { m_bIsPaused = paused; }

   private:
    bool m_bIsPaused;
};

class MainMenu : public OsuScreen, public MouseListener {
   public:
    static UString NEOSU_MAIN_BUTTON_TEXT;
    static UString NEOSU_MAIN_BUTTON_SUBTEXT;

    friend class MainMenuCubeButton;
    friend class MainMenuButton;
    void onPausePressed();
    void onCubePressed();

    MainMenu(Osu *osu);
    virtual ~MainMenu();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    DatabaseBeatmap *preloaded_beatmap = nullptr;
    void selectRandomBeatmap();

    virtual void onKeyDown(KeyboardEvent &e);

    virtual void onLeftChange(bool down) { (void)down; }
    virtual void onMiddleChange(bool down);
    virtual void onRightChange(bool down) { (void)down; }

    virtual void onWheelVertical(int delta) { (void)delta; }
    virtual void onWheelHorizontal(int delta) { (void)delta; }

    virtual void onResolutionChange(Vector2 newResolution);

    virtual CBaseUIContainer *setVisible(bool visible);

    void setStartupAnim(bool startupAnim) {
        m_bStartupAnim = startupAnim;
        m_fStartupAnim = m_fStartupAnim2 = (m_bStartupAnim ? 0.0f : 1.0f);
    }

    inline Osu *getOsu() const { return m_osu; }

   private:
    static ConVar *m_osu_universal_offset_ref;
    static ConVar *m_osu_universal_offset_hardcoded_ref;
    static ConVar *m_osu_old_beatmap_offset_ref;
    static ConVar *m_osu_universal_offset_hardcoded_fallback_dsound_ref;
    static ConVar *m_osu_mod_random_ref;
    static ConVar *m_osu_songbrowser_background_fade_in_duration_ref;

    void drawVersionInfo(Graphics *g);
    void updateLayout();

    void animMainButton();
    void animMainButtonBack();

    void setMenuElementsVisible(bool visible, bool animate = true);

    void writeVersionFile();

    MainMenuButton *addMainMenuButton(UString text);

    void onPlayButtonPressed();
    void onMultiplayerButtonPressed();
    void onOptionsButtonPressed();
    void onExitButtonPressed();

    void onUpdatePressed();
    void onVersionPressed();

    float m_fUpdateStatusTime;
    float m_fUpdateButtonTextTime;
    float m_fUpdateButtonAnimTime;
    float m_fUpdateButtonAnim;
    bool m_bHasClickedUpdate;
    bool shuffling = false;

    Vector2 m_vSize;
    Vector2 m_vCenter;
    float m_fSizeAddAnim;
    float m_fCenterOffsetAnim;

    bool m_bMenuElementsVisible;
    float m_fMainMenuButtonCloseTime = 0.f;

    MainMenuCubeButton *m_cube;
    std::vector<MainMenuButton *> m_menuElements;

    MainMenuPauseButton *m_pauseButton;
    UIButton *m_updateAvailableButton;
    CBaseUIButton *m_versionButton;

    bool m_bDrawVersionNotificationArrow;
    bool m_bDidUserUpdateFromOlderVersion;
    bool m_bDidUserUpdateFromOlderVersionLe3300;
    bool m_bDidUserUpdateFromOlderVersionLe3303;

    // custom
    float m_fMainMenuAnimTime;
    float m_fMainMenuAnimDuration;
    float m_fMainMenuAnim;
    float m_fMainMenuAnim1;
    float m_fMainMenuAnim2;
    float m_fMainMenuAnim3;
    float m_fMainMenuAnim1Target;
    float m_fMainMenuAnim2Target;
    float m_fMainMenuAnim3Target;
    bool m_bInMainMenuRandomAnim;
    int m_iMainMenuRandomAnimType;
    unsigned int m_iMainMenuAnimBeatCounter;

    bool m_bMainMenuAnimFriend;
    bool m_bMainMenuAnimFadeToFriendForNextAnim;
    bool m_bMainMenuAnimFriendScheduled;
    float m_fMainMenuAnimFriendPercent;
    float m_fMainMenuAnimFriendEyeFollowX;
    float m_fMainMenuAnimFriendEyeFollowY;

    float m_fShutdownScheduledTime;
    bool m_bWasCleanShutdown;

    bool m_bStartupAnim;
    float m_fStartupAnim;
    float m_fStartupAnim2;
    float m_fPrevShuffleTime;
    float m_fBackgroundFadeInTime;

    McFont *m_titleFont;
};

#endif
