#pragma once
#include "CBaseUIButton.h"
#include "MouseListener.h"
#include "OsuScreen.h"

class Image;

class Beatmap;
class DatabaseBeatmap;
typedef DatabaseBeatmap BeatmapDifficulty;
typedef DatabaseBeatmap BeatmapSet;

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
        this->bIsPaused = true;
    }

    virtual void draw(Graphics *g);
    void setPaused(bool paused) { this->bIsPaused = paused; }

   private:
    bool bIsPaused;
};

class MainMenu : public OsuScreen, public MouseListener {
   public:
    static UString NEOSU_MAIN_BUTTON_TEXT;
    static UString NEOSU_MAIN_BUTTON_SUBTEXT;

    friend class MainMenuCubeButton;
    friend class MainMenuButton;
    void onPausePressed();
    void onCubePressed();

    MainMenu();
    virtual ~MainMenu();

    virtual void draw(Graphics *g);
    virtual void mouse_update(bool *propagate_clicks);

    BeatmapDifficulty *preloaded_beatmap = NULL;
    BeatmapSet *preloaded_beatmapset = NULL;
    void selectRandomBeatmap();

    virtual void onKeyDown(KeyboardEvent &e);

    virtual void onLeftChange(bool down) { (void)down; }
    virtual void onMiddleChange(bool down);
    virtual void onRightChange(bool down) { (void)down; }

    virtual void onWheelVertical(int delta) { (void)delta; }
    virtual void onWheelHorizontal(int delta) { (void)delta; }

    virtual void onResolutionChange(Vector2 newResolution);

    virtual CBaseUIContainer *setVisible(bool visible);

   private:
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

    float fUpdateStatusTime;
    float fUpdateButtonTextTime;
    float fUpdateButtonAnimTime;
    float fUpdateButtonAnim;
    bool bHasClickedUpdate;
    bool shuffling = false;

    Vector2 vSize;
    Vector2 vCenter;
    float fSizeAddAnim;
    float fCenterOffsetAnim;

    bool bMenuElementsVisible;
    float fMainMenuButtonCloseTime = 0.f;

    MainMenuCubeButton *cube;
    std::vector<MainMenuButton *> menuElements;

    MainMenuPauseButton *pauseButton;
    UIButton *updateAvailableButton = NULL;
    CBaseUIButton *versionButton;

    bool bDrawVersionNotificationArrow;
    bool bDidUserUpdateFromOlderVersion;

    // custom
    float fMainMenuAnimTime;
    float fMainMenuAnimDuration;
    float fMainMenuAnim;
    float fMainMenuAnim1;
    float fMainMenuAnim2;
    float fMainMenuAnim3;
    float fMainMenuAnim1Target;
    float fMainMenuAnim2Target;
    float fMainMenuAnim3Target;
    bool bInMainMenuRandomAnim;
    int iMainMenuRandomAnimType;
    unsigned int iMainMenuAnimBeatCounter;

    bool bMainMenuAnimFriend;
    bool bMainMenuAnimFadeToFriendForNextAnim;
    bool bMainMenuAnimFriendScheduled;
    float fMainMenuAnimFriendPercent;
    float fMainMenuAnimFriendEyeFollowX;
    float fMainMenuAnimFriendEyeFollowY;

    float fShutdownScheduledTime;
    bool bWasCleanShutdown;

    bool bStartupAnim = true;
    f32 fStartupAnim = 0.f;
    f32 fStartupAnim2 = 0.f;
    float fPrevShuffleTime;
    float fBackgroundFadeInTime;

    Image *logo_img;
    Shader *background_shader = NULL;
};
