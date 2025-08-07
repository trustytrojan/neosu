#pragma once
// Copyright (c) 2015, PG, All rights reserved.
#include <utility>

#include "CBaseUIButton.h"
#include "MouseListener.h"
#include "OsuScreen.h"
#include "Resource.h"
#include "ResourceManager.h"

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

struct SongsFolderEnumerator : public Resource {
    ~SongsFolderEnumerator() override = default;
    SongsFolderEnumerator() : Resource("SONGS_FOLDER_ENUMERATOR") {
        resourceManager->requestNextLoadAsync();
        resourceManager->loadResource(this);
    }

    SongsFolderEnumerator &operator=(const SongsFolderEnumerator &) = delete;
    SongsFolderEnumerator &operator=(SongsFolderEnumerator &&) = delete;
    SongsFolderEnumerator(const SongsFolderEnumerator &) = delete;
    SongsFolderEnumerator(SongsFolderEnumerator &&) = delete;

    [[nodiscard]] inline const std::vector<std::string> &getEntries() const { return this->entries; }

    // type inspection
    [[nodiscard]] Type getResType() const final { return APPDEFINED; }

   protected:
    void init() override { this->bReady = true; }
    void initAsync() override;
    void destroy() override { ; }

   private:
    std::vector<std::string> entries{};
};

class MainMenuPauseButton : public CBaseUIButton {
   public:
    MainMenuPauseButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text)
        : CBaseUIButton(xPos, yPos, xSize, ySize, std::move(name), std::move(text)) {
        this->bIsPaused = true;
    }

    void draw() override;
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
    ~MainMenu() override;

    void draw() override;
    void mouse_update(bool *propagate_clicks) override;

    BeatmapDifficulty *preloaded_beatmap = NULL;
    BeatmapSet *preloaded_beatmapset = NULL;
    void selectRandomBeatmap();

    void onKeyDown(KeyboardEvent &e) override;

    void onButtonChange(ButtonIndex button, bool down) override;

    void onResolutionChange(Vector2 newResolution) override;

    CBaseUIContainer *setVisible(bool visible) override;

   private:
    void drawVersionInfo();
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

    SongsFolderEnumerator songs_enumerator;
};
