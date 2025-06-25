#include "MainMenu.h"

#include "AnimationHandler.h"
#include "BackgroundImageHandler.h"
#include "Bancho.h"
#include "Beatmap.h"
#include "CBaseUIButton.h"
#include "CBaseUIContainer.h"
#include "ConVar.h"
#include "Database.h"
#include "DatabaseBeatmap.h"
#include "Downloader.h"
#include "Engine.h"
#include "File.h"
#include "GameRules.h"
#include "HUD.h"
#include "Keyboard.h"
#include "Lobby.h"
#include "Mouse.h"
#include "OptionsMenu.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "RichPresence.h"
#include "Skin.h"
#include "SkinImage.h"
#include "SongBrowser/SongBrowser.h"
#include "SoundEngine.h"
#include "UIButton.h"
#include "UpdateHandler.h"
#include "VertexArrayObject.h"

static float button_sound_cooldown = 0.f;

using namespace std;

UString MainMenu::NEOSU_MAIN_BUTTON_TEXT = UString("neosu");
UString MainMenu::NEOSU_MAIN_BUTTON_SUBTEXT = UString("Multiplayer Client");

static MainMenu *g_main_menu = NULL;

class MainMenuCubeButton : public CBaseUIButton {
   public:
    MainMenuCubeButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text);

    void draw() override;

    void onMouseInside() override;
    void onMouseOutside() override;
};

class MainMenuButton : public CBaseUIButton {
   public:
    MainMenuButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text);

    void onMouseDownInside() override;
    void onMouseInside() override;
};

void MainMenuPauseButton::draw() {
    int third = this->vSize.x / 3;

    g->setColor(0xffffffff);

    if(!this->bIsPaused) {
        g->fillRect(this->vPos.x, this->vPos.y, third, this->vSize.y + 1);
        g->fillRect(this->vPos.x + 2 * third, this->vPos.y, third, this->vSize.y + 1);
    } else {
        g->setColor(0xffffffff);
        VertexArrayObject vao;

        const int smoothPixels = 2;

        // center triangle
        vao.addVertex(this->vPos.x, this->vPos.y + smoothPixels);
        vao.addColor(0xffffffff);
        vao.addVertex(this->vPos.x + this->vSize.x, this->vPos.y + this->vSize.y / 2);
        vao.addColor(0xffffffff);
        vao.addVertex(this->vPos.x, this->vPos.y + this->vSize.y - smoothPixels);
        vao.addColor(0xffffffff);

        // top smooth
        vao.addVertex(this->vPos.x, this->vPos.y + smoothPixels);
        vao.addColor(0xffffffff);
        vao.addVertex(this->vPos.x, this->vPos.y);
        vao.addColor(0x00000000);
        vao.addVertex(this->vPos.x + this->vSize.x, this->vPos.y + this->vSize.y / 2);
        vao.addColor(0xffffffff);

        vao.addVertex(this->vPos.x, this->vPos.y);
        vao.addColor(0x00000000);
        vao.addVertex(this->vPos.x + this->vSize.x, this->vPos.y + this->vSize.y / 2);
        vao.addColor(0xffffffff);
        vao.addVertex(this->vPos.x + this->vSize.x, this->vPos.y + this->vSize.y / 2 - smoothPixels);
        vao.addColor(0x00000000);

        // bottom smooth
        vao.addVertex(this->vPos.x, this->vPos.y + this->vSize.y - smoothPixels);
        vao.addColor(0xffffffff);
        vao.addVertex(this->vPos.x, this->vPos.y + this->vSize.y);
        vao.addColor(0x00000000);
        vao.addVertex(this->vPos.x + this->vSize.x, this->vPos.y + this->vSize.y / 2);
        vao.addColor(0xffffffff);

        vao.addVertex(this->vPos.x, this->vPos.y + this->vSize.y);
        vao.addColor(0x00000000);
        vao.addVertex(this->vPos.x + this->vSize.x, this->vPos.y + this->vSize.y / 2);
        vao.addColor(0xffffffff);
        vao.addVertex(this->vPos.x + this->vSize.x, this->vPos.y + this->vSize.y / 2 + smoothPixels);
        vao.addColor(0x00000000);

        g->drawVAO(&vao);
    }

    // draw hover rects
    g->setColor(this->frameColor);
    if(this->bMouseInside && this->bEnabled) {
        if(!this->bActive && !mouse->isLeftDown())
            this->drawHoverRect(3);
        else if(this->bActive)
            this->drawHoverRect(3);
    }
    if(this->bActive && this->bEnabled) this->drawHoverRect(6);
}

MainMenu::MainMenu() : OsuScreen() {
    g_main_menu = this;

    // engine settings
    mouse->addListener(this);

    this->fSizeAddAnim = 0.0f;
    this->fCenterOffsetAnim = 0.0f;
    this->bMenuElementsVisible = false;

    this->fMainMenuAnimTime = 0.0f;
    this->fMainMenuAnimDuration = 0.0f;
    this->fMainMenuAnim = 0.0f;
    this->fMainMenuAnim1 = 0.0f;
    this->fMainMenuAnim2 = 0.0f;
    this->fMainMenuAnim3 = 0.0f;
    this->fMainMenuAnim1Target = 0.0f;
    this->fMainMenuAnim2Target = 0.0f;
    this->fMainMenuAnim3Target = 0.0f;
    this->bInMainMenuRandomAnim = false;
    this->iMainMenuRandomAnimType = 0;
    this->iMainMenuAnimBeatCounter = 0;

    this->bMainMenuAnimFriend = false;
    this->bMainMenuAnimFadeToFriendForNextAnim = false;
    this->bMainMenuAnimFriendScheduled = false;
    this->fMainMenuAnimFriendPercent = 0.0f;
    this->fMainMenuAnimFriendEyeFollowX = 0.0f;
    this->fMainMenuAnimFriendEyeFollowY = 0.0f;

    this->fShutdownScheduledTime = 0.0f;
    this->bWasCleanShutdown = false;

    this->fUpdateStatusTime = 0.0f;
    this->fUpdateButtonTextTime = 0.0f;
    this->fUpdateButtonAnim = 0.0f;
    this->fUpdateButtonAnimTime = 0.0f;
    this->bHasClickedUpdate = false;

    this->fBackgroundFadeInTime = 0.0f;

    this->logo_img = resourceManager->loadImage("neosu.png", "NEOSU_LOGO");
    // background_shader = resourceManager->loadShader("main_menu_bg.vsh", "main_menu_bg.fsh");

    // check if the user has never clicked the changelog for this update
    this->bDidUserUpdateFromOlderVersion = false;
    this->bDrawVersionNotificationArrow = false;
    {
        if(env->fileExists("version.txt")) {
            File versionFile("version.txt");
            if(versionFile.canRead()) {
                float version = std::stof(versionFile.readLine());
                if(version < cv_version.getFloat()) this->bDrawVersionNotificationArrow = true;
                if(version < 35.06) {
                    // SoundEngine choking issues have been fixed, option has been removed from settings menu
                    // We leave the cvar available as it could still be useful for some players
                    cv_restart_sound_engine_before_playing.setValue(false);

                    // 0.5 is shit default value
                    if(cv_songbrowser_search_delay.getFloat() == 0.5f) {
                        cv_songbrowser_search_delay.setValue(0.2f);
                    }

                    // Match osu!stable value
                    if(cv_relax_offset.getFloat() == 0.f) {
                        cv_relax_offset.setValue(-12.f);
                    }

                    osu->getOptionsMenu()->save();
                }
            } else {
                this->bDrawVersionNotificationArrow = true;
            }
        }
    }
    this->bDidUserUpdateFromOlderVersion = this->bDrawVersionNotificationArrow;  // (same logic atm)

    this->setPos(-1, 0);
    this->setSize(osu->getScreenWidth(), osu->getScreenHeight());

    this->cube = new MainMenuCubeButton(0, 0, 1, 1, "", "");
    this->cube->setClickCallback(fastdelegate::MakeDelegate(this, &MainMenu::onCubePressed));
    this->addBaseUIElement(this->cube);

    this->addMainMenuButton("Singleplayer")
        ->setClickCallback(fastdelegate::MakeDelegate(this, &MainMenu::onPlayButtonPressed));
    this->addMainMenuButton("Multiplayer")
        ->setClickCallback(fastdelegate::MakeDelegate(this, &MainMenu::onMultiplayerButtonPressed));
    this->addMainMenuButton("Options (CTRL + O)")
        ->setClickCallback(fastdelegate::MakeDelegate(this, &MainMenu::onOptionsButtonPressed));
    this->addMainMenuButton("Exit")->setClickCallback(fastdelegate::MakeDelegate(this, &MainMenu::onExitButtonPressed));

    this->pauseButton = new MainMenuPauseButton(0, 0, 0, 0, "", "");
    this->pauseButton->setClickCallback(fastdelegate::MakeDelegate(this, &MainMenu::onPausePressed));
    this->addBaseUIElement(this->pauseButton);

    if(env->getOS() == Environment::OS::WINDOWS) {
        this->updateAvailableButton = new UIButton(0, 0, 0, 0, "", "Checking for updates ...");
        this->updateAvailableButton->setUseDefaultSkin();
        this->updateAvailableButton->setClickCallback(fastdelegate::MakeDelegate(this, &MainMenu::onUpdatePressed));
        this->updateAvailableButton->setColor(0x2200ff00);
        this->updateAvailableButton->setTextColor(0x22ffffff);
    }

    this->versionButton = new CBaseUIButton(0, 0, 0, 0, "", "");
    UString versionString = UString::format("Version %.2f", cv_version.getFloat());
    this->versionButton->setText(versionString);
    this->versionButton->setDrawBackground(false);
    this->versionButton->setDrawFrame(false);
    this->versionButton->setClickCallback(fastdelegate::MakeDelegate(this, &MainMenu::onVersionPressed));
    this->addBaseUIElement(this->versionButton);
}

MainMenu::~MainMenu() {
    SAFE_DELETE(this->preloaded_beatmapset);
    SAFE_DELETE(this->updateAvailableButton);

    anim->deleteExistingAnimation(&this->fUpdateButtonAnim);

    anim->deleteExistingAnimation(&this->fMainMenuAnimFriendEyeFollowX);
    anim->deleteExistingAnimation(&this->fMainMenuAnimFriendEyeFollowY);

    anim->deleteExistingAnimation(&this->fMainMenuAnim);
    anim->deleteExistingAnimation(&this->fMainMenuAnim1);
    anim->deleteExistingAnimation(&this->fMainMenuAnim2);
    anim->deleteExistingAnimation(&this->fMainMenuAnim3);

    anim->deleteExistingAnimation(&this->fCenterOffsetAnim);
    anim->deleteExistingAnimation(&this->fStartupAnim);
    anim->deleteExistingAnimation(&this->fStartupAnim2);

    // if the user didn't click on the update notification during this session, quietly remove it so it's not annoying
    if(this->bWasCleanShutdown) this->writeVersionFile();
}

void MainMenu::draw() {
    if(!this->bVisible) return;

    // load server icon
    if(bancho.is_online() && bancho.server_icon_url.length() > 0 && bancho.server_icon == NULL) {
        auto icon_path = UString::format(MCENGINE_DATA_DIR "avatars/%s", bancho.endpoint.toUtf8());
        if(!env->directoryExists(icon_path.toUtf8())) {
            env->createDirectory(icon_path.toUtf8());
        }
        icon_path.append("/server_icon");

        float progress = -1.f;
        std::vector<u8> data;
        int response_code;
        download(bancho.server_icon_url.toUtf8(), &progress, data, &response_code);
        if(progress == -1.f) bancho.server_icon_url = "";
        if(!data.empty()) {
            FILE *file = fopen(icon_path.toUtf8(), "wb");
            if(file != NULL) {
                fwrite(data.data(), data.size(), 1, file);
                fflush(file);
                fclose(file);
            }

            bancho.server_icon = resourceManager->loadImageAbs(icon_path.toUtf8(), icon_path.toUtf8());
        }
    }

    // menu-background
    if(cv_draw_menu_background.getBool()) {
        Image *backgroundImage = osu->getSkin()->getMenuBackground();
        if(backgroundImage != NULL && backgroundImage != osu->getSkin()->getMissingTexture() &&
           backgroundImage->isReady()) {
            const float scale = Osu::getImageScaleToFillResolution(backgroundImage, osu->getScreenSize());

            g->setColor(0xffffffff);
            g->setAlpha(this->fStartupAnim);
            g->pushTransform();
            {
                g->scale(scale, scale);
                g->translate(osu->getScreenWidth() / 2, osu->getScreenHeight() / 2);
                g->drawImage(backgroundImage);
            }
            g->popTransform();
        }
    }

    // XXX: Should do fade transition between beatmap backgrounds when switching to next song
    float alpha = 1.0f;
    if(cv_songbrowser_background_fade_in_duration.getFloat() > 0.0f) {
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
            alpha = clamp<float>((engine->getTime() - this->fBackgroundFadeInTime) /
                                     cv_songbrowser_background_fade_in_duration.getFloat(),
                                 0.0f, 1.0f);
            alpha = 1.0f - (1.0f - alpha) * (1.0f - alpha);
        }
    }

    // background_shader->enable();
    // background_shader->setUniform1f("time", engine->getTime());
    // background_shader->setUniform2f("resolution", osu->getScreenWidth(), osu->getScreenHeight());
    SongBrowser::drawSelectedBeatmapBackgroundImage(alpha);
    // background_shader->disable();

    // main button stuff
    bool haveTimingpoints = false;
    const float div = 1.25f;
    float pulse = 0.0f;
    if(osu->getSelectedBeatmap()->getSelectedDifficulty2() != NULL && osu->getSelectedBeatmap()->getMusic() != NULL &&
       osu->getSelectedBeatmap()->getMusic()->isPlaying()) {
        haveTimingpoints = true;

        const long curMusicPos =
            (long)osu->getSelectedBeatmap()->getMusic()->getPositionMS() +
            (long)(cv_universal_offset.getFloat() * osu->getSelectedBeatmap()->getSpeedMultiplier()) +
            (long)cv_universal_offset_hardcoded.getInt() -
            osu->getSelectedBeatmap()->getSelectedDifficulty2()->getLocalOffset() -
            osu->getSelectedBeatmap()->getSelectedDifficulty2()->getOnlineOffset() -
            (osu->getSelectedBeatmap()->getSelectedDifficulty2()->getVersion() < 5 ? cv_old_beatmap_offset.getInt()
                                                                                   : 0);

        DatabaseBeatmap::TIMING_INFO t =
            osu->getSelectedBeatmap()->getSelectedDifficulty2()->getTimingInfoForTime(curMusicPos);

        if(t.beatLengthBase == 0.0f)  // bah
            t.beatLengthBase = 1.0f;

        this->iMainMenuAnimBeatCounter =
            (curMusicPos - t.offset - (long)(max((long)t.beatLengthBase, (long)1) * 0.5f)) /
            max((long)t.beatLengthBase, (long)1);
        pulse = (float)((curMusicPos - t.offset) % max((long)t.beatLengthBase, (long)1)) /
                t.beatLengthBase;  // modulo must be >= 1
        pulse = clamp<float>(pulse, -1.0f, 1.0f);
        if(pulse < 0.0f) pulse = 1.0f - std::abs(pulse);
    } else
        pulse = (div - fmod(engine->getTime(), div)) / div;

    Vector2 size = this->vSize;
    const float pulseSub = 0.05f * pulse;
    size -= size * pulseSub;
    size += size * this->fSizeAddAnim;
    size *= this->fStartupAnim;
    McRect mainButtonRect = McRect(this->vCenter.x - size.x / 2.0f - this->fCenterOffsetAnim,
                                   this->vCenter.y - size.y / 2.0f, size.x, size.y);

    // draw notification arrow for changelog (version button)
    if(this->bDrawVersionNotificationArrow) {
        float animation = fmod((float)(engine->getTime()) * 3.2f, 2.0f);
        if(animation > 1.0f) animation = 2.0f - animation;
        animation = -animation * (animation - 2);  // quad out
        float offset = osu->getUIScale(45.0f * animation);

        const float scale =
            this->versionButton->getSize().x / osu->getSkin()->getPlayWarningArrow2()->getSizeBaseRaw().x;

        const Vector2 arrowPos = Vector2(
            this->versionButton->getSize().x / 1.75f,
            osu->getScreenHeight() - this->versionButton->getSize().y * 2 - this->versionButton->getSize().y * scale);

        UString notificationText = "Changelog";
        g->setColor(0xffffffff);
        g->pushTransform();
        {
            McFont *smallFont = osu->getSubTitleFont();
            g->translate(arrowPos.x - smallFont->getStringWidth(notificationText) / 2.0f,
                         (-offset * 2) * scale + arrowPos.y -
                             (osu->getSkin()->getPlayWarningArrow2()->getSizeBaseRaw().y * scale) / 1.5f,
                         0);
            g->drawString(smallFont, notificationText);
        }
        g->popTransform();

        g->setColor(0xffffffff);
        g->pushTransform();
        {
            g->rotate(90.0f);
            g->translate(0, -offset * 2, 0);
            osu->getSkin()->getPlayWarningArrow2()->drawRaw(arrowPos, scale);
        }
        g->popTransform();
    }

    // draw container
    OsuScreen::draw();

    // draw update check button
    if(this->updateAvailableButton != NULL) {
        if(osu->getUpdateHandler()->getStatus() == UpdateHandler::STATUS::STATUS_SUCCESS_INSTALLATION) {
            g->push3DScene(McRect(this->updateAvailableButton->getPos().x, this->updateAvailableButton->getPos().y,
                                  this->updateAvailableButton->getSize().x, this->updateAvailableButton->getSize().y));
            g->rotate3DScene(this->fUpdateButtonAnim * 360.0f, 0, 0);
        }
        this->updateAvailableButton->draw();
        if(osu->getUpdateHandler()->getStatus() == UpdateHandler::STATUS::STATUS_SUCCESS_INSTALLATION) g->pop3DScene();
    }

    // draw main button
    bool drawing_full_cube = (this->fMainMenuAnim > 0.0f && this->fMainMenuAnim != 1.0f) ||
                             (haveTimingpoints && this->fMainMenuAnimFriendPercent > 0.0f);

    float inset = 0.0f;
    if(drawing_full_cube) {
        inset = 1.0f - 0.5 * this->fMainMenuAnimFriendPercent;

        g->setDepthBuffer(true);
        g->clearDepthBuffer();
        g->setCulling(true);

        g->push3DScene(mainButtonRect);
        g->offset3DScene(0, 0, mainButtonRect.getWidth() / 2);

        float friendRotation = 0.0f;
        float friendTranslationX = 0.0f;
        float friendTranslationY = 0.0f;
        if(haveTimingpoints && this->fMainMenuAnimFriendPercent > 0.0f) {
            float customPulse = 0.0f;
            if(pulse > 0.5f)
                customPulse = (pulse - 0.5f) / 0.5f;
            else
                customPulse = (0.5f - pulse) / 0.5f;

            customPulse = 1.0f - customPulse;

            const float anim = lerp<float>((1.0f - customPulse) * (1.0f - customPulse), (1.0f - customPulse), 0.25f);
            const float anim2 = anim * (this->iMainMenuAnimBeatCounter % 2 == 1 ? 1.0f : -1.0f);
            const float anim3 = anim;

            friendRotation = anim2 * 13;
            friendTranslationX = -anim2 * mainButtonRect.getWidth() * 0.175;
            friendTranslationY = anim3 * mainButtonRect.getWidth() * 0.10;

            friendRotation *= this->fMainMenuAnimFriendPercent;
            friendTranslationX *= this->fMainMenuAnimFriendPercent;
            friendTranslationY *= this->fMainMenuAnimFriendPercent;
        }

        g->translate3DScene(friendTranslationX, friendTranslationY, 0);
        g->rotate3DScene(this->fMainMenuAnim1 * 360.0f, this->fMainMenuAnim2 * 360.0f,
                         this->fMainMenuAnim3 * 360.0f + friendRotation);
    }

    const Color cubeColor = COLORf(1.0f, lerp<float>(0.0f, 0.5f, this->fMainMenuAnimFriendPercent),
                                   lerp<float>(0.0f, 0.768f, this->fMainMenuAnimFriendPercent),
                                   lerp<float>(0.0f, 0.965f, this->fMainMenuAnimFriendPercent));
    const Color cubeBorderColor = COLORf(1.0f, lerp<float>(1.0f, 0.5f, this->fMainMenuAnimFriendPercent),
                                         lerp<float>(1.0f, 0.768f, this->fMainMenuAnimFriendPercent),
                                         lerp<float>(1.0f, 0.965f, this->fMainMenuAnimFriendPercent));

    // front side
    g->pushTransform();
    g->translate(0, 0, inset);
    g->setColor(cubeColor);
    g->setAlpha(cv_main_menu_alpha.getFloat());
    g->fillRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2 * inset,
                mainButtonRect.getHeight() - 2 * inset);
    g->setColor(cubeBorderColor);
    g->drawRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2 * inset,
                mainButtonRect.getHeight() - 2 * inset);
    g->popTransform();

    // friend
    if(this->fMainMenuAnimFriendPercent > 0.0f) {
        // ears
        {
            const float width = mainButtonRect.getWidth() * 0.11f * 2.0f * (1.0f - pulse * 0.05f);

            const float margin = width * 0.4f;

            const float offset = mainButtonRect.getWidth() * 0.02f;

            VertexArrayObject vao;
            {
                const Vector2 pos = Vector2(mainButtonRect.getX(), mainButtonRect.getY() - offset);

                Vector2 left = pos + Vector2(0, 0);
                Vector2 top = pos + Vector2(width / 2, -width * std::sqrt(3.0f) / 2.0f);
                Vector2 right = pos + Vector2(width, 0);

                Vector2 topRightDir = (top - right);
                {
                    const float temp = topRightDir.x;
                    topRightDir.x = -topRightDir.y;
                    topRightDir.y = temp;
                }

                Vector2 innerLeft = left + topRightDir.normalize() * margin;

                vao.addVertex(left.x, left.y);
                vao.addVertex(top.x, top.y);
                vao.addVertex(innerLeft.x, innerLeft.y);

                Vector2 leftRightDir = (right - left);
                {
                    const float temp = leftRightDir.x;
                    leftRightDir.x = -leftRightDir.y;
                    leftRightDir.y = temp;
                }

                Vector2 innerTop = top + leftRightDir.normalize() * margin;

                vao.addVertex(top.x, top.y);
                vao.addVertex(innerTop.x, innerTop.y);
                vao.addVertex(innerLeft.x, innerLeft.y);

                Vector2 leftTopDir = (left - top);
                {
                    const float temp = leftTopDir.x;
                    leftTopDir.x = -leftTopDir.y;
                    leftTopDir.y = temp;
                }

                Vector2 innerRight = right + leftTopDir.normalize() * margin;

                vao.addVertex(top.x, top.y);
                vao.addVertex(innerRight.x, innerRight.y);
                vao.addVertex(innerTop.x, innerTop.y);

                vao.addVertex(top.x, top.y);
                vao.addVertex(right.x, right.y);
                vao.addVertex(innerRight.x, innerRight.y);

                vao.addVertex(left.x, left.y);
                vao.addVertex(innerLeft.x, innerLeft.y);
                vao.addVertex(innerRight.x, innerRight.y);

                vao.addVertex(left.x, left.y);
                vao.addVertex(innerRight.x, innerRight.y);
                vao.addVertex(right.x, right.y);
            }

            // left
            g->setColor(0xffc8faf1);
            g->setAlpha(this->fMainMenuAnimFriendPercent * cv_main_menu_alpha.getFloat());
            g->drawVAO(&vao);

            // right
            g->pushTransform();
            {
                g->translate(mainButtonRect.getWidth() - width, 0);
                g->drawVAO(&vao);
            }
            g->popTransform();
        }

        float headBob = 0.0f;
        {
            float customPulse = 0.0f;
            if(pulse > 0.5f)
                customPulse = (pulse - 0.5f) / 0.5f;
            else
                customPulse = (0.5f - pulse) / 0.5f;

            customPulse = 1.0f - customPulse;

            if(!haveTimingpoints) customPulse = 1.0f;

            headBob = (customPulse) * (customPulse);
            headBob *= this->fMainMenuAnimFriendPercent;
        }

        const float mouthEyeOffsetY = mainButtonRect.getWidth() * 0.18f + headBob * mainButtonRect.getWidth() * 0.075f;

        // mouth
        {
            const float width = mainButtonRect.getWidth() * 0.10f;
            const float height = mainButtonRect.getHeight() * 0.03f * 1.75;

            const float length = width * std::sqrt(2.0f) * 2;

            const float offsetY = mainButtonRect.getHeight() / 2.0f + mouthEyeOffsetY;

            g->pushTransform();
            {
                g->rotate(135);
                g->translate(mainButtonRect.getX() + length / 2 + mainButtonRect.getWidth() / 2 -
                                 this->fMainMenuAnimFriendEyeFollowX * mainButtonRect.getWidth() * 0.5f,
                             mainButtonRect.getY() + offsetY -
                                 this->fMainMenuAnimFriendEyeFollowY * mainButtonRect.getWidth() * 0.5f);

                g->setColor(0xff000000);
                g->setAlpha(cv_main_menu_alpha.getFloat());
                g->fillRect(0, 0, width, height);
                g->fillRect(width - height / 2.0f, 0, height, width);
                g->fillRect(width - height / 2.0f, width - height / 2.0f, width, height);
                g->fillRect(width * 2 - height, width - height / 2.0f, height, width + height / 2);
            }
            g->popTransform();
        }

        // eyes
        {
            const float width = mainButtonRect.getWidth() * 0.22f;
            const float height = mainButtonRect.getHeight() * 0.03f * 2;

            const float offsetX = mainButtonRect.getWidth() * 0.18f;
            const float offsetY = mainButtonRect.getHeight() * 0.21f + mouthEyeOffsetY;

            const float rotation = 25.0f;

            // left
            g->pushTransform();
            {
                g->translate(-width, 0);
                g->rotate(-rotation);
                g->translate(width, 0);
                g->translate(
                    mainButtonRect.getX() + offsetX - this->fMainMenuAnimFriendEyeFollowX * mainButtonRect.getWidth(),
                    mainButtonRect.getY() + offsetY - this->fMainMenuAnimFriendEyeFollowY * mainButtonRect.getWidth());

                g->setColor(0xff000000);
                g->setAlpha(cv_main_menu_alpha.getFloat());
                g->fillRect(0, 0, width, height);
            }
            g->popTransform();

            // right
            g->pushTransform();
            {
                g->rotate(rotation);
                g->translate(
                    mainButtonRect.getX() + mainButtonRect.getWidth() - offsetX - width -
                        this->fMainMenuAnimFriendEyeFollowX * mainButtonRect.getWidth(),
                    mainButtonRect.getY() + offsetY - this->fMainMenuAnimFriendEyeFollowY * mainButtonRect.getWidth());

                g->setColor(0xff000000);
                g->setAlpha(cv_main_menu_alpha.getFloat());
                g->fillRect(0, 0, width, height);
            }
            g->popTransform();

            // tear
            g->setColor(0xff000000);
            g->setAlpha(cv_main_menu_alpha.getFloat());
            g->fillRect(mainButtonRect.getX() + offsetX + width * 0.375f -
                            this->fMainMenuAnimFriendEyeFollowX * mainButtonRect.getWidth(),
                        mainButtonRect.getY() + offsetY + width / 2.0f -
                            this->fMainMenuAnimFriendEyeFollowY * mainButtonRect.getWidth(),
                        height * 0.75f, width * 0.375f);
        }

        // hands
        {
            const float size = mainButtonRect.getWidth() * 0.2f;

            const float offset = -size * 0.75f;

            float customPulse = 0.0f;
            if(pulse > 0.5f)
                customPulse = (pulse - 0.5f) / 0.5f;
            else
                customPulse = (0.5f - pulse) / 0.5f;

            customPulse = 1.0f - customPulse;

            if(!haveTimingpoints) customPulse = 1.0f;

            const float animLeftMultiplier = (this->iMainMenuAnimBeatCounter % 2 == 0 ? 1.0f : 0.1f);
            const float animRightMultiplier = (this->iMainMenuAnimBeatCounter % 2 == 1 ? 1.0f : 0.1f);

            const float animMoveUp =
                lerp<float>((1.0f - customPulse) * (1.0f - customPulse), (1.0f - customPulse), 0.35f) *
                this->fMainMenuAnimFriendPercent;

            const float animLeftMoveUp = animMoveUp * animLeftMultiplier;
            const float animRightMoveUp = animMoveUp * animRightMultiplier;

            const float animLeftMoveLeft = animRightMoveUp * (this->iMainMenuAnimBeatCounter % 2 == 1 ? 1.0f : 0.0f);
            const float animRightMoveRight = animLeftMoveUp * (this->iMainMenuAnimBeatCounter % 2 == 0 ? 1.0f : 0.0f);

            // left
            g->setColor(0xffd5f6fd);
            g->setAlpha(this->fMainMenuAnimFriendPercent * cv_main_menu_alpha.getFloat());
            g->pushTransform();
            {
                g->rotate(40 - (1.0f - customPulse) * 10 + animLeftMoveLeft * animLeftMoveLeft * 20);
                g->translate(mainButtonRect.getX() - size - offset -
                                 animLeftMoveLeft * mainButtonRect.getWidth() * -0.025f -
                                 animLeftMoveUp * mainButtonRect.getWidth() * 0.25f,
                             mainButtonRect.getY() + mainButtonRect.getHeight() - size -
                                 animLeftMoveUp * mainButtonRect.getHeight() * 0.85f,
                             -0.5f);
                g->fillRect(0, 0, size, size);
            }
            g->popTransform();

            // right
            g->pushTransform();
            {
                g->rotate(50 + (1.0f - customPulse) * 10 - animRightMoveRight * animRightMoveRight * 20);
                g->translate(mainButtonRect.getX() + mainButtonRect.getWidth() + size + offset +
                                 animRightMoveRight * mainButtonRect.getWidth() * -0.025f +
                                 animRightMoveUp * mainButtonRect.getWidth() * 0.25f,
                             mainButtonRect.getY() + mainButtonRect.getHeight() - size -
                                 animRightMoveUp * mainButtonRect.getHeight() * 0.85f,
                             -0.5f);
                g->fillRect(0, 0, size, size);
            }
            g->popTransform();
        }
    }

    // neosu/server logo
    {
        auto logo = this->logo_img;
        if(bancho.server_icon != NULL && bancho.server_icon->isReady() && cv_main_menu_use_server_logo.getBool()) {
            logo = bancho.server_icon;
        }

        float alpha = (1.0f - this->fMainMenuAnimFriendPercent) * (1.0f - this->fMainMenuAnimFriendPercent) *
                      (1.0f - this->fMainMenuAnimFriendPercent);

        float xscale = mainButtonRect.getWidth() / logo->getWidth();
        float yscale = mainButtonRect.getHeight() / logo->getHeight();
        float scale = min(xscale, yscale) * 0.8f;

        g->pushTransform();
        g->setColor(0xffffffff);
        g->setAlpha(alpha);
        g->scale(scale, scale);
        g->translate(this->vCenter.x - this->fCenterOffsetAnim, this->vCenter.y);
        g->drawImage(logo);
        g->popTransform();
    }

    if(drawing_full_cube) {
        // back side
        g->rotate3DScene(0, -180, 0);
        g->pushTransform();
        g->translate(0, 0, inset);
        g->setColor(cubeColor);
        g->setAlpha(cv_main_menu_alpha.getFloat());
        g->fillRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2 * inset,
                    mainButtonRect.getHeight() - 2 * inset);
        g->setColor(cubeBorderColor);
        g->drawRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2 * inset,
                    mainButtonRect.getHeight() - 2 * inset);
        g->popTransform();

        // right side
        g->offset3DScene(0, 0, mainButtonRect.getWidth() / 2);
        g->rotate3DScene(0, 90, 0);
        g->pushTransform();
        g->translate(0, 0, inset);
        g->setColor(cubeColor);
        g->setAlpha(cv_main_menu_alpha.getFloat());
        g->fillRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2 * inset,
                    mainButtonRect.getHeight() - 2 * inset);
        g->setColor(cubeBorderColor);
        g->drawRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2 * inset,
                    mainButtonRect.getHeight() - 2 * inset);
        g->popTransform();
        g->rotate3DScene(0, -90, 0);
        g->offset3DScene(0, 0, 0);

        // left side
        g->offset3DScene(0, 0, mainButtonRect.getWidth() / 2);
        g->rotate3DScene(0, -90, 0);
        g->pushTransform();
        g->translate(0, 0, inset);
        g->setColor(cubeColor);
        g->setAlpha(cv_main_menu_alpha.getFloat());
        g->fillRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2 * inset,
                    mainButtonRect.getHeight() - 2 * inset);
        g->setColor(cubeBorderColor);
        g->drawRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2 * inset,
                    mainButtonRect.getHeight() - 2 * inset);
        g->popTransform();
        g->rotate3DScene(0, 90, 0);
        g->offset3DScene(0, 0, 0);

        // top side
        g->offset3DScene(0, 0, mainButtonRect.getHeight() / 2);
        g->rotate3DScene(90, 0, 0);
        g->pushTransform();
        g->translate(0, 0, inset);
        g->setColor(cubeColor);
        g->setAlpha(cv_main_menu_alpha.getFloat());
        g->fillRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2 * inset,
                    mainButtonRect.getHeight() - 2 * inset);
        g->setColor(cubeBorderColor);
        g->drawRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2 * inset,
                    mainButtonRect.getHeight() - 2 * inset);
        g->popTransform();
        g->rotate3DScene(-90, 0, 0);
        g->offset3DScene(0, 0, 0);

        // bottom side
        g->offset3DScene(0, 0, mainButtonRect.getHeight() / 2);
        g->rotate3DScene(-90, 0, 0);
        g->pushTransform();
        g->translate(0, 0, inset);
        g->setColor(cubeColor);
        g->setAlpha(cv_main_menu_alpha.getFloat());
        g->fillRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2 * inset,
                    mainButtonRect.getHeight() - 2 * inset);
        g->setColor(cubeBorderColor);
        g->drawRect(mainButtonRect.getX() + inset, mainButtonRect.getY() + inset, mainButtonRect.getWidth() - 2 * inset,
                    mainButtonRect.getHeight() - 2 * inset);
        g->popTransform();
        g->rotate3DScene(90, 0, 0);
        g->offset3DScene(0, 0, 0);

        g->pop3DScene();

        g->setCulling(false);
        g->setDepthBuffer(false);
    }
}

void MainMenu::mouse_update(bool *propagate_clicks) {
    if(!this->bVisible) return;

    this->updateLayout();

    // update and focus handling
    OsuScreen::mouse_update(propagate_clicks);

    if(this->updateAvailableButton != NULL) {
        this->updateAvailableButton->mouse_update(propagate_clicks);
    }

    // handle automatic menu closing
    if(this->fMainMenuButtonCloseTime != 0.0f && engine->getTime() > this->fMainMenuButtonCloseTime) {
        this->fMainMenuButtonCloseTime = 0.0f;
        this->setMenuElementsVisible(false);
    }

    // hide the buttons if the closing animation finished
    if(!anim->isAnimating(&this->fCenterOffsetAnim) && this->fCenterOffsetAnim == 0.0f) {
        for(int i = 0; i < this->menuElements.size(); i++) {
            this->menuElements[i]->setVisible(false);
        }
    }

    // handle delayed shutdown
    if(this->fShutdownScheduledTime != 0.0f &&
       (engine->getTime() > this->fShutdownScheduledTime || !anim->isAnimating(&this->fCenterOffsetAnim))) {
        engine->shutdown();
        this->fShutdownScheduledTime = 0.0f;
    }

    // main button autohide + anim
    if(this->bMenuElementsVisible) {
        this->fMainMenuAnimDuration = 15.0f;
        this->fMainMenuAnimTime = engine->getTime() + this->fMainMenuAnimDuration;
    }
    if(engine->getTime() > this->fMainMenuAnimTime) {
        if(this->bMainMenuAnimFriendScheduled) this->bMainMenuAnimFriend = true;
        if(this->bMainMenuAnimFadeToFriendForNextAnim) this->bMainMenuAnimFriendScheduled = true;

        this->fMainMenuAnimDuration = 10.0f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 5.0f;
        this->fMainMenuAnimTime = engine->getTime() + this->fMainMenuAnimDuration;
        this->animMainButton();
    }

    if(this->bInMainMenuRandomAnim && this->iMainMenuRandomAnimType == 1 && anim->isAnimating(&this->fMainMenuAnim)) {
        Vector2 mouseDelta = (this->cube->getPos() + this->cube->getSize() / 2) - mouse->getPos();
        mouseDelta.x = clamp<float>(mouseDelta.x, -engine->getScreenSize().x / 2, engine->getScreenSize().x / 2);
        mouseDelta.y = clamp<float>(mouseDelta.y, -engine->getScreenSize().y / 2, engine->getScreenSize().y / 2);
        mouseDelta.x /= engine->getScreenSize().x;
        mouseDelta.y /= engine->getScreenSize().y;

        const float decay = clamp<float>((1.0f - this->fMainMenuAnim - 0.075f) / 0.025f, 0.0f, 1.0f);

        const Vector2 pushAngle = Vector2(mouseDelta.y, -mouseDelta.x) * Vector2(0.15f, 0.15f) * decay;

        anim->moveQuadOut(&this->fMainMenuAnim1, pushAngle.x, 0.15f, true);

        anim->moveQuadOut(&this->fMainMenuAnim2, pushAngle.y, 0.15f, true);

        anim->moveQuadOut(&this->fMainMenuAnim3, 0.0f, 0.15f, true);
    }

    {
        this->fMainMenuAnimFriendPercent =
            1.0f - clamp<float>((this->fMainMenuAnimDuration > 0.0f
                                     ? (this->fMainMenuAnimTime - engine->getTime()) / this->fMainMenuAnimDuration
                                     : 0.0f),
                                0.0f, 1.0f);
        this->fMainMenuAnimFriendPercent = clamp<float>((this->fMainMenuAnimFriendPercent - 0.5f) / 0.5f, 0.0f, 1.0f);
        if(this->bMainMenuAnimFriend) this->fMainMenuAnimFriendPercent = 1.0f;
        if(!this->bMainMenuAnimFriendScheduled) this->fMainMenuAnimFriendPercent = 0.0f;

        Vector2 mouseDelta = (this->cube->getPos() + this->cube->getSize() / 2) - mouse->getPos();
        mouseDelta.x = clamp<float>(mouseDelta.x, -engine->getScreenSize().x / 2, engine->getScreenSize().x / 2);
        mouseDelta.y = clamp<float>(mouseDelta.y, -engine->getScreenSize().y / 2, engine->getScreenSize().y / 2);
        mouseDelta.x /= engine->getScreenSize().x;
        mouseDelta.y /= engine->getScreenSize().y;

        const Vector2 pushAngle = Vector2(mouseDelta.x, mouseDelta.y) * 0.1f;

        anim->moveLinear(&this->fMainMenuAnimFriendEyeFollowX, pushAngle.x, 0.25f, true);
        anim->moveLinear(&this->fMainMenuAnimFriendEyeFollowY, pushAngle.y, 0.25f, true);
    }

    // handle update checker and status text
    if(this->updateAvailableButton != NULL) {
        switch(osu->getUpdateHandler()->getStatus()) {
            case UpdateHandler::STATUS::STATUS_UP_TO_DATE:
                if(this->updateAvailableButton->isVisible()) {
                    this->updateAvailableButton->setText("");
                    this->updateAvailableButton->setVisible(false);
                }
                break;
            case UpdateHandler::STATUS::STATUS_CHECKING_FOR_UPDATE:
                this->updateAvailableButton->setText("Checking for updates ...");
                break;
            case UpdateHandler::STATUS::STATUS_DOWNLOADING_UPDATE:
                this->updateAvailableButton->setText("Downloading ...");
                break;
            case UpdateHandler::STATUS::STATUS_INSTALLING_UPDATE:
                this->updateAvailableButton->setText("Installing ...");
                break;
            case UpdateHandler::STATUS::STATUS_SUCCESS_INSTALLATION:
                if(engine->getTime() > this->fUpdateButtonTextTime && anim->isAnimating(&this->fUpdateButtonAnim) &&
                   this->fUpdateButtonAnim > 0.175f) {
                    this->fUpdateButtonTextTime = this->fUpdateButtonAnimTime;

                    this->updateAvailableButton->setColor(0xff00ff00);
                    this->updateAvailableButton->setTextColor(0xffffffff);

                    if(this->updateAvailableButton->getText().find("ready") != -1)
                        this->updateAvailableButton->setText("Click here to restart now!");
                    else
                        this->updateAvailableButton->setText("A new version of neosu is ready!");
                }
                break;
            case UpdateHandler::STATUS::STATUS_ERROR:
                this->updateAvailableButton->setText("Update Error! Click to retry ...");
                break;
        }
    }

    if(osu->getUpdateHandler()->getStatus() == UpdateHandler::STATUS::STATUS_SUCCESS_INSTALLATION &&
       engine->getTime() > this->fUpdateButtonAnimTime) {
        this->fUpdateButtonAnimTime = engine->getTime() + 3.0f;
        this->fUpdateButtonAnim = 0.0f;
        anim->moveQuadInOut(&this->fUpdateButtonAnim, 1.0f, 0.5f, true);
    }

    // Update pause button and shuffle songs
    this->pauseButton->setPaused(true);

    if(soundEngine->isReady()) {
        auto diff2 = osu->getSelectedBeatmap()->getSelectedDifficulty2();
        auto music = osu->getSelectedBeatmap()->getMusic();
        if(music == NULL) {
            this->selectRandomBeatmap();
        } else {
            if(music->isFinished()) {
                this->selectRandomBeatmap();
            } else if(music->isPlaying()) {
                this->pauseButton->setPaused(false);

                // NOTE: We set this every frame, because music loading isn't instant
                music->setLoop(false);

                // load timing points if needed
                // XXX: file io, don't block main thread
                if(diff2 && diff2->getTimingpoints().empty()) {
                    diff2->loadMetadata(false);
                }
            }
        }
    }
}

void MainMenu::selectRandomBeatmap() {
    auto sb = osu->getSongBrowser();
    if(db->isFinished() && !sb->beatmaps.empty()) {
        sb->selectRandomBeatmap();
        RichPresence::onMainMenu();
    } else {
        // Database is not loaded yet, load a random map and select it
        auto songs_folder = db->getOsuSongsFolder();
        auto mapset_folders = env->getFoldersInFolder(songs_folder);
        auto mapset_folders2 = env->getFoldersInFolder(MCENGINE_DATA_DIR "maps/");
        auto nb_mapsets = mapset_folders.size();
        auto nb_mapsets2 = mapset_folders2.size();
        if(nb_mapsets + nb_mapsets2 == 0) return;

        sb->getSelectedBeatmap()->deselect();
        SAFE_DELETE(this->preloaded_beatmapset);

        for(int i = 0; i < 10; i++) {
            u32 lucky_number = rand() % (nb_mapsets + nb_mapsets2);
            bool is_neosu_set = lucky_number > nb_mapsets;
            if(is_neosu_set) lucky_number -= nb_mapsets;

            auto mapset_folder = is_neosu_set ? MCENGINE_DATA_DIR "maps/" : songs_folder;
            mapset_folder.append((is_neosu_set ? mapset_folders2 : mapset_folders)[lucky_number]);
            mapset_folder.append("/");

            BeatmapSet *set = db->loadRawBeatmap(mapset_folder);
            if(set == NULL) {
                debugLog("Failed to load beatmap set '%s'\n", mapset_folder.c_str());
                continue;
            }

            auto beatmap_diffs = set->getDifficulties();
            if(beatmap_diffs.size() == 0) {
                debugLog("Beatmap '%s' has no difficulties!\n", mapset_folder.c_str());
                delete set;
                continue;
            }

            this->preloaded_beatmapset = set;

            // We're picking a random diff and not the first one, because diffs of the same set
            // can have their own separate sound file.
            this->preloaded_beatmap = beatmap_diffs[rand() % beatmap_diffs.size()];
            this->preloaded_beatmap->do_not_store = true;

            osu->getSongBrowser()->onDifficultySelected(this->preloaded_beatmap, false);
            RichPresence::onMainMenu();

            return;
        }

        debugLog("Failed to pick random beatmap...\n");
    }
}

void MainMenu::onKeyDown(KeyboardEvent &e) {
    OsuScreen::onKeyDown(e);  // only used for options menu
    if(!this->bVisible || e.isConsumed()) return;

    if(!osu->getOptionsMenu()->isMouseInside()) {
        if(e == KEY_RIGHT || e == KEY_F2) this->selectRandomBeatmap();
    }

    if(e == KEY_C || e == KEY_F4) this->onPausePressed();

    if(!this->bMenuElementsVisible) {
        if(e == KEY_P || e == KEY_ENTER) this->cube->click();
    } else {
        if(e == KEY_P || e == KEY_ENTER) this->onPlayButtonPressed();
        if(e == KEY_O) this->onOptionsButtonPressed();
        if(e == KEY_E || e == KEY_X) this->onExitButtonPressed();

        if(e == KEY_ESCAPE) this->setMenuElementsVisible(false);
    }
}

void MainMenu::onMiddleChange(bool down) {
    if(!this->bVisible) return;

    // debug anims
    if(down && !anim->isAnimating(&this->fMainMenuAnim) && !this->bMenuElementsVisible) {
        if(keyboard->isShiftDown()) {
            this->bMainMenuAnimFriend = true;
            this->bMainMenuAnimFriendScheduled = true;
            this->bMainMenuAnimFadeToFriendForNextAnim = true;
        }

        this->animMainButton();
        this->fMainMenuAnimDuration = 15.0f;
        this->fMainMenuAnimTime = engine->getTime() + this->fMainMenuAnimDuration;
    }
}

void MainMenu::onResolutionChange(Vector2 newResolution) {
    this->updateLayout();
    this->setMenuElementsVisible(this->bMenuElementsVisible);
}

CBaseUIContainer *MainMenu::setVisible(bool visible) {
    this->bVisible = visible;

    if(visible) {
        RichPresence::onMainMenu();

        if(!bancho.spectators.empty()) {
            Packet packet;
            packet.id = OUT_SPECTATE_FRAMES;
            write<i32>(&packet, 0);
            write<u16>(&packet, 0);
            write<u8>(&packet, LiveReplayBundle::Action::NONE);
            write<ScoreFrame>(&packet, ScoreFrame::get());
            write<u16>(&packet, osu->getSelectedBeatmap()->spectator_sequence++);
            send_packet(packet);
        }

        this->updateLayout();

        this->fMainMenuAnimDuration = 15.0f;
        this->fMainMenuAnimTime = engine->getTime() + this->fMainMenuAnimDuration;

        if(this->bStartupAnim) {
            this->bStartupAnim = false;
            anim->moveQuadOut(&this->fStartupAnim, 1.0f, cv_main_menu_startup_anim_duration.getFloat());
            anim->moveQuartOut(&this->fStartupAnim2, 1.0f, cv_main_menu_startup_anim_duration.getFloat() * 6.0f,
                               cv_main_menu_startup_anim_duration.getFloat() * 0.5f);
        }
    } else {
        this->setMenuElementsVisible(false, false);
    }

    return this;
}

void MainMenu::updateLayout() {
    const float dpiScale = Osu::getUIScale();

    this->vCenter = osu->getScreenSize() / 2.0f;
    const float size = Osu::getUIScale(324.0f);
    this->vSize = Vector2(size, size);

    this->cube->setRelPos(this->vCenter - this->vSize / 2.0f - Vector2(this->fCenterOffsetAnim, 0.0f));
    this->cube->setSize(this->vSize);

    this->pauseButton->setSize(30 * dpiScale, 30 * dpiScale);
    this->pauseButton->setRelPos(osu->getScreenWidth() - this->pauseButton->getSize().x * 2 - 10 * dpiScale,
                                 this->pauseButton->getSize().y + 10 * dpiScale);

    if(this->updateAvailableButton != NULL) {
        this->updateAvailableButton->setSize(375 * dpiScale, 50 * dpiScale);
        this->updateAvailableButton->setPos(
            osu->getScreenWidth() / 2 - this->updateAvailableButton->getSize().x / 2,
            osu->getScreenHeight() - this->updateAvailableButton->getSize().y - 10 * dpiScale);
    }

    this->versionButton->onResized();  // HACKHACK: framework, setSizeToContent() does not update string metrics
    this->versionButton->setSizeToContent(8 * dpiScale, 8 * dpiScale);
    this->versionButton->setRelPos(-1, osu->getScreenSize().y - this->versionButton->getSize().y);

    int numButtons = this->menuElements.size();
    int menuElementHeight = this->vSize.y / numButtons;
    int menuElementPadding = numButtons > 3 ? this->vSize.y * 0.04f : this->vSize.y * 0.075f;
    menuElementHeight -= (numButtons - 1) * menuElementPadding;
    int menuElementExtraWidth = this->vSize.x * 0.06f;

    float offsetPercent = this->fCenterOffsetAnim / (this->vSize.x / 2.0f);
    float curY = this->cube->getRelPos().y +
                 (this->vSize.y - menuElementHeight * numButtons - (numButtons - 1) * menuElementPadding) / 2.0f;
    for(int i = 0; i < this->menuElements.size(); i++) {
        curY += (i > 0 ? menuElementHeight + menuElementPadding : 0.0f);

        this->menuElements[i]->onResized();  // HACKHACK: framework, setSize() does not update string metrics
        this->menuElements[i]->setRelPos(this->cube->getRelPos().x + this->cube->getSize().x * offsetPercent -
                                             menuElementExtraWidth * offsetPercent +
                                             menuElementExtraWidth * (1.0f - offsetPercent),
                                         curY);
        this->menuElements[i]->setSize(this->cube->getSize().x + menuElementExtraWidth * offsetPercent -
                                           2.0f * menuElementExtraWidth * (1.0f - offsetPercent),
                                       menuElementHeight);
        this->menuElements[i]->setTextColor(
            COLORf(offsetPercent * offsetPercent * offsetPercent * offsetPercent, 1.0f, 1.0f, 1.0f));
        this->menuElements[i]->setFrameColor(COLORf(offsetPercent, 1.0f, 1.0f, 1.0f));
        this->menuElements[i]->setBackgroundColor(
            COLORf(offsetPercent * cv_main_menu_alpha.getFloat(), 0.0f, 0.0f, 0.0f));
    }

    this->setSize(osu->getScreenSize() + Vector2(1, 1));
    this->update_pos();
}

void MainMenu::animMainButton() {
    this->bInMainMenuRandomAnim = true;

    this->iMainMenuRandomAnimType = (rand() % 4) == 1 ? 1 : 0;
    if(!this->bMainMenuAnimFadeToFriendForNextAnim && cv_main_menu_friend.getBool() &&
       env->getOS() == Environment::OS::WINDOWS)  // NOTE: z buffer bullshit on other platforms >:(
        this->bMainMenuAnimFadeToFriendForNextAnim = (rand() % 24) == 1;

    this->fMainMenuAnim = 0.0f;
    this->fMainMenuAnim1 = 0.0f;
    this->fMainMenuAnim2 = 0.0f;

    if(this->iMainMenuRandomAnimType == 0) {
        this->fMainMenuAnim3 = 1.0f;

        this->fMainMenuAnim1Target = (rand() % 2) == 1 ? 1.0f : -1.0f;
        this->fMainMenuAnim2Target = (rand() % 2) == 1 ? 1.0f : -1.0f;
        this->fMainMenuAnim3Target = (rand() % 2) == 1 ? 1.0f : -1.0f;

        const float randomDuration1 = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 3.5f;
        const float randomDuration2 = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 3.5f;
        const float randomDuration3 = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 3.5f;

        anim->moveQuadOut(&this->fMainMenuAnim, 1.0f,
                          1.5f + max(randomDuration1, max(randomDuration2, randomDuration3)));
        anim->moveQuadOut(&this->fMainMenuAnim1, this->fMainMenuAnim1Target, 1.5f + randomDuration1);
        anim->moveQuadOut(&this->fMainMenuAnim2, this->fMainMenuAnim2Target, 1.5f + randomDuration2);
        anim->moveQuadOut(&this->fMainMenuAnim3, this->fMainMenuAnim3Target, 1.5f + randomDuration3);
    } else {
        this->fMainMenuAnim3 = 0.0f;

        this->fMainMenuAnim1Target = 0.0f;
        this->fMainMenuAnim2Target = 0.0f;
        this->fMainMenuAnim3Target = 0.0f;

        this->fMainMenuAnim = 0.0f;
        anim->moveQuadOut(&this->fMainMenuAnim, 1.0f, 5.0f);
    }
}

void MainMenu::animMainButtonBack() {
    this->bInMainMenuRandomAnim = false;

    if(anim->isAnimating(&this->fMainMenuAnim)) {
        anim->moveQuadOut(&this->fMainMenuAnim, 1.0f, 0.25f, true);
        anim->moveQuadOut(&this->fMainMenuAnim1, this->fMainMenuAnim1Target, 0.25f, true);
        anim->moveQuadOut(&this->fMainMenuAnim1, 0.0f, 0.0f, 0.25f);
        anim->moveQuadOut(&this->fMainMenuAnim2, this->fMainMenuAnim2Target, 0.25f, true);
        anim->moveQuadOut(&this->fMainMenuAnim2, 0.0f, 0.0f, 0.25f);
        anim->moveQuadOut(&this->fMainMenuAnim3, this->fMainMenuAnim3Target, 0.10f, true);
        anim->moveQuadOut(&this->fMainMenuAnim3, 0.0f, 0.0f, 0.1f);
    }
}

void MainMenu::setMenuElementsVisible(bool visible, bool animate) {
    this->bMenuElementsVisible = visible;

    if(visible) {
        if(this->bMenuElementsVisible &&
           this->vSize.x / 2.0f < this->fCenterOffsetAnim)  // so we don't see the ends of the menu element buttons
                                                            // if the window gets smaller
            this->fCenterOffsetAnim = this->vSize.x / 2.0f;

        if(animate)
            anim->moveQuadInOut(&this->fCenterOffsetAnim, this->vSize.x / 2.0f, 0.35f, 0.0f, true);
        else {
            anim->deleteExistingAnimation(&this->fCenterOffsetAnim);
            this->fCenterOffsetAnim = this->vSize.x / 2.0f;
        }

        this->fMainMenuButtonCloseTime = engine->getTime() + 6.0f;

        for(int i = 0; i < this->menuElements.size(); i++) {
            this->menuElements[i]->setVisible(true);
            this->menuElements[i]->setEnabled(true);
        }
    } else {
        if(animate)
            anim->moveQuadOut(&this->fCenterOffsetAnim, 0.0f,
                              0.5f * (this->fCenterOffsetAnim / (this->vSize.x / 2.0f)) *
                                  (this->fShutdownScheduledTime != 0.0f ? 0.4f : 1.0f),
                              0.0f, true);
        else {
            anim->deleteExistingAnimation(&this->fCenterOffsetAnim);
            this->fCenterOffsetAnim = 0.0f;
        }

        this->fMainMenuButtonCloseTime = 0.0f;

        for(int i = 0; i < this->menuElements.size(); i++) {
            this->menuElements[i]->setEnabled(false);
        }
    }
}

void MainMenu::writeVersionFile() {
    // remember, don't show the notification arrow until the version changes again
    std::ofstream versionFile("version.txt", std::ios::out | std::ios::trunc);
    if(versionFile.good()) versionFile << cv_version.getFloat();
}

MainMenuButton *MainMenu::addMainMenuButton(UString text) {
    MainMenuButton *button = new MainMenuButton(this->vSize.x, 0, 1, 1, "", text);
    button->setFont(osu->getSubTitleFont());
    button->setVisible(false);

    this->menuElements.push_back(button);
    this->addBaseUIElement(button);
    return button;
}

void MainMenu::onCubePressed() {
    soundEngine->play(osu->getSkin()->clickMainMenuCube);

    anim->moveQuadInOut(&this->fSizeAddAnim, 0.0f, 0.06f, 0.0f, false);
    anim->moveQuadInOut(&this->fSizeAddAnim, 0.12f, 0.06f, 0.07f, false);

    // if the menu is already visible, this counts as pressing the play button
    if(this->bMenuElementsVisible)
        this->onPlayButtonPressed();
    else
        this->setMenuElementsVisible(true);

    if(anim->isAnimating(&this->fMainMenuAnim) && this->bInMainMenuRandomAnim)
        this->animMainButtonBack();
    else {
        this->bInMainMenuRandomAnim = false;

        Vector2 mouseDelta = (this->cube->getPos() + this->cube->getSize() / 2) - mouse->getPos();
        mouseDelta.x = clamp<float>(mouseDelta.x, -this->cube->getSize().x / 2, this->cube->getSize().x / 2);
        mouseDelta.y = clamp<float>(mouseDelta.y, -this->cube->getSize().y / 2, this->cube->getSize().y / 2);
        mouseDelta.x /= this->cube->getSize().x;
        mouseDelta.y /= this->cube->getSize().y;

        const Vector2 pushAngle = Vector2(mouseDelta.y, -mouseDelta.x) * Vector2(0.15f, 0.15f);

        this->fMainMenuAnim = 0.001f;
        anim->moveQuadOut(&this->fMainMenuAnim, 1.0f, 0.15f + 0.4f, true);

        if(!anim->isAnimating(&this->fMainMenuAnim1)) this->fMainMenuAnim1 = 0.0f;

        anim->moveQuadOut(&this->fMainMenuAnim1, pushAngle.x, 0.15f, true);
        anim->moveQuadOut(&this->fMainMenuAnim1, 0.0f, 0.4f, 0.15f);

        if(!anim->isAnimating(&this->fMainMenuAnim2)) this->fMainMenuAnim2 = 0.0f;

        anim->moveQuadOut(&this->fMainMenuAnim2, pushAngle.y, 0.15f, true);
        anim->moveQuadOut(&this->fMainMenuAnim2, 0.0f, 0.4f, 0.15f);

        if(!anim->isAnimating(&this->fMainMenuAnim3)) this->fMainMenuAnim3 = 0.0f;

        anim->moveQuadOut(&this->fMainMenuAnim3, 0.0f, 0.15f, true);
    }
}

void MainMenu::onPlayButtonPressed() {
    this->bMainMenuAnimFriend = false;
    this->bMainMenuAnimFadeToFriendForNextAnim = false;
    this->bMainMenuAnimFriendScheduled = false;

    osu->toggleSongBrowser();

    soundEngine->play(osu->getSkin()->menuHit);
    soundEngine->play(osu->getSkin()->clickSingleplayer);
}

void MainMenu::onMultiplayerButtonPressed() {
    if(bancho.user_id <= 0) {
        osu->optionsMenu->askForLoginDetails();
        return;
    }

    this->setVisible(false);
    osu->lobby->setVisible(true);

    soundEngine->play(osu->getSkin()->menuHit);
    soundEngine->play(osu->getSkin()->clickMultiplayer);
}

void MainMenu::onOptionsButtonPressed() {
    if(!osu->getOptionsMenu()->isVisible()) osu->toggleOptionsMenu();

    soundEngine->play(osu->getSkin()->clickOptions);
}

void MainMenu::onExitButtonPressed() {
    this->fShutdownScheduledTime = engine->getTime() + 0.3f;
    this->bWasCleanShutdown = true;
    this->setMenuElementsVisible(false);

    soundEngine->play(osu->getSkin()->clickExit);
}

void MainMenu::onPausePressed() {
    if(osu->getSelectedBeatmap()->isPreviewMusicPlaying()) {
        osu->getSelectedBeatmap()->pausePreviewMusic();
    } else {
        auto music = osu->getSelectedBeatmap()->getMusic();
        if(music != NULL) {
            soundEngine->play(music);
        }
    }
}

void MainMenu::onUpdatePressed() {
    if(osu->getUpdateHandler()->getStatus() == UpdateHandler::STATUS::STATUS_SUCCESS_INSTALLATION)
        engine->restart();
    else if(osu->getUpdateHandler()->getStatus() == UpdateHandler::STATUS::STATUS_ERROR)
        osu->getUpdateHandler()->checkForUpdates();
}

void MainMenu::onVersionPressed() {
    this->bDrawVersionNotificationArrow = false;
    this->writeVersionFile();
    osu->toggleChangelog();
}

MainMenuCubeButton::MainMenuCubeButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text)
    : CBaseUIButton(xPos, yPos, xSize, ySize, name, text) {}

void MainMenuCubeButton::draw() {
    // draw nothing
}

void MainMenuCubeButton::onMouseInside() {
    anim->moveQuadInOut(&g_main_menu->fSizeAddAnim, 0.12f, 0.15f, 0.0f, true);

    CBaseUIButton::onMouseInside();

    if(button_sound_cooldown + 0.05f < engine->getTime()) {
        soundEngine->play(osu->getSkin()->hoverMainMenuCube);
        button_sound_cooldown = engine->getTime();
    }
}

void MainMenuCubeButton::onMouseOutside() {
    anim->moveQuadInOut(&g_main_menu->fSizeAddAnim, 0.0f, 0.15f, 0.0f, true);

    CBaseUIButton::onMouseOutside();
}

MainMenuButton::MainMenuButton(float xPos, float yPos, float xSize, float ySize, UString name, UString text)
    : CBaseUIButton(xPos, yPos, xSize, ySize, name, text) {}

void MainMenuButton::onMouseDownInside() {
    if(g_main_menu->cube->isMouseInside()) return;
    CBaseUIButton::onMouseDownInside();
}

void MainMenuButton::onMouseInside() {
    if(g_main_menu->cube->isMouseInside()) return;
    CBaseUIButton::onMouseInside();

    if(button_sound_cooldown + 0.05f < engine->getTime()) {
        if(this->getText() == UString("Singleplayer")) {
            soundEngine->play(osu->getSkin()->hoverSingleplayer);
        } else if(this->getText() == UString("Multiplayer")) {
            soundEngine->play(osu->getSkin()->hoverMultiplayer);
        } else if(this->getText() == UString("Options (CTRL + O)")) {
            soundEngine->play(osu->getSkin()->hoverOptions);
        } else if(this->getText() == UString("Exit")) {
            soundEngine->play(osu->getSkin()->hoverExit);
        }

        button_sound_cooldown = engine->getTime();
    }
}
