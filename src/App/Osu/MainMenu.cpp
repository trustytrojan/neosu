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

#define NEOSU_NEWVERSION_NOTIFICATION_TRIGGER_FILE "version.txt"

class MainMenuCubeButton : public CBaseUIButton {
   public:
    MainMenuCubeButton(MainMenu *mainMenu, float xPos, float yPos, float xSize, float ySize, UString name,
                       UString text);

    virtual void draw(Graphics *g);

    void onMouseInside();
    void onMouseOutside();

    MainMenu *m_mainMenu;
};

class MainMenuButton : public CBaseUIButton {
   public:
    MainMenuButton(MainMenu *mainMenu, float xPos, float yPos, float xSize, float ySize, UString name, UString text);

    void onMouseDownInside();
    void onMouseInside();

   private:
    MainMenu *m_mainMenu;
};

void MainMenuPauseButton::draw(Graphics *g) {
    int third = m_vSize.x / 3;

    g->setColor(0xffffffff);

    if(!m_bIsPaused) {
        g->fillRect(m_vPos.x, m_vPos.y, third, m_vSize.y + 1);
        g->fillRect(m_vPos.x + 2 * third, m_vPos.y, third, m_vSize.y + 1);
    } else {
        g->setColor(0xffffffff);
        VertexArrayObject vao;

        const int smoothPixels = 2;

        // center triangle
        vao.addVertex(m_vPos.x, m_vPos.y + smoothPixels);
        vao.addColor(0xffffffff);
        vao.addVertex(m_vPos.x + m_vSize.x, m_vPos.y + m_vSize.y / 2);
        vao.addColor(0xffffffff);
        vao.addVertex(m_vPos.x, m_vPos.y + m_vSize.y - smoothPixels);
        vao.addColor(0xffffffff);

        // top smooth
        vao.addVertex(m_vPos.x, m_vPos.y + smoothPixels);
        vao.addColor(0xffffffff);
        vao.addVertex(m_vPos.x, m_vPos.y);
        vao.addColor(0x00000000);
        vao.addVertex(m_vPos.x + m_vSize.x, m_vPos.y + m_vSize.y / 2);
        vao.addColor(0xffffffff);

        vao.addVertex(m_vPos.x, m_vPos.y);
        vao.addColor(0x00000000);
        vao.addVertex(m_vPos.x + m_vSize.x, m_vPos.y + m_vSize.y / 2);
        vao.addColor(0xffffffff);
        vao.addVertex(m_vPos.x + m_vSize.x, m_vPos.y + m_vSize.y / 2 - smoothPixels);
        vao.addColor(0x00000000);

        // bottom smooth
        vao.addVertex(m_vPos.x, m_vPos.y + m_vSize.y - smoothPixels);
        vao.addColor(0xffffffff);
        vao.addVertex(m_vPos.x, m_vPos.y + m_vSize.y);
        vao.addColor(0x00000000);
        vao.addVertex(m_vPos.x + m_vSize.x, m_vPos.y + m_vSize.y / 2);
        vao.addColor(0xffffffff);

        vao.addVertex(m_vPos.x, m_vPos.y + m_vSize.y);
        vao.addColor(0x00000000);
        vao.addVertex(m_vPos.x + m_vSize.x, m_vPos.y + m_vSize.y / 2);
        vao.addColor(0xffffffff);
        vao.addVertex(m_vPos.x + m_vSize.x, m_vPos.y + m_vSize.y / 2 + smoothPixels);
        vao.addColor(0x00000000);

        g->drawVAO(&vao);
    }

    // draw hover rects
    g->setColor(m_frameColor);
    if(m_bMouseInside && m_bEnabled) {
        if(!m_bActive && !engine->getMouse()->isLeftDown())
            drawHoverRect(g, 3);
        else if(m_bActive)
            drawHoverRect(g, 3);
    }
    if(m_bActive && m_bEnabled) drawHoverRect(g, 6);
}

MainMenu::MainMenu() : OsuScreen() {
    // engine settings
    engine->getMouse()->addListener(this);

    m_fSizeAddAnim = 0.0f;
    m_fCenterOffsetAnim = 0.0f;
    m_bMenuElementsVisible = false;

    m_fMainMenuAnimTime = 0.0f;
    m_fMainMenuAnimDuration = 0.0f;
    m_fMainMenuAnim = 0.0f;
    m_fMainMenuAnim1 = 0.0f;
    m_fMainMenuAnim2 = 0.0f;
    m_fMainMenuAnim3 = 0.0f;
    m_fMainMenuAnim1Target = 0.0f;
    m_fMainMenuAnim2Target = 0.0f;
    m_fMainMenuAnim3Target = 0.0f;
    m_bInMainMenuRandomAnim = false;
    m_iMainMenuRandomAnimType = 0;
    m_iMainMenuAnimBeatCounter = 0;

    m_bMainMenuAnimFriend = false;
    m_bMainMenuAnimFadeToFriendForNextAnim = false;
    m_bMainMenuAnimFriendScheduled = false;
    m_fMainMenuAnimFriendPercent = 0.0f;
    m_fMainMenuAnimFriendEyeFollowX = 0.0f;
    m_fMainMenuAnimFriendEyeFollowY = 0.0f;

    m_fShutdownScheduledTime = 0.0f;
    m_bWasCleanShutdown = false;

    m_fUpdateStatusTime = 0.0f;
    m_fUpdateButtonTextTime = 0.0f;
    m_fUpdateButtonAnim = 0.0f;
    m_fUpdateButtonAnimTime = 0.0f;
    m_bHasClickedUpdate = false;

    m_bStartupAnim = true;
    m_fStartupAnim = 0.0f;
    m_fStartupAnim2 = 0.0f;

    m_fBackgroundFadeInTime = 0.0f;

    logo_img = engine->getResourceManager()->loadImage("neosu.png", "NEOSU_LOGO");
    // background_shader = engine->getResourceManager()->loadShader("main_menu_bg.vsh", "main_menu_bg.fsh");

    // check if the user has never clicked the changelog for this update
    m_bDidUserUpdateFromOlderVersion = false;
    m_bDrawVersionNotificationArrow = false;
    {
        if(env->fileExists(NEOSU_NEWVERSION_NOTIFICATION_TRIGGER_FILE)) {
            File versionFile(NEOSU_NEWVERSION_NOTIFICATION_TRIGGER_FILE);
            if(versionFile.canRead()) {
                float version = std::stof(versionFile.readLine());
                if(version < cv_version.getFloat() - 0.0001f) m_bDrawVersionNotificationArrow = true;
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
                m_bDrawVersionNotificationArrow = true;
            }
        }
    }
    m_bDidUserUpdateFromOlderVersion = m_bDrawVersionNotificationArrow;  // (same logic atm)

    setPos(-1, 0);
    setSize(osu->getScreenWidth(), osu->getScreenHeight());

    m_cube = new MainMenuCubeButton(this, 0, 0, 1, 1, "", "");
    m_cube->setClickCallback(fastdelegate::MakeDelegate(this, &MainMenu::onCubePressed));
    addBaseUIElement(m_cube);

    addMainMenuButton("Singleplayer")
        ->setClickCallback(fastdelegate::MakeDelegate(this, &MainMenu::onPlayButtonPressed));
    addMainMenuButton("Multiplayer")
        ->setClickCallback(fastdelegate::MakeDelegate(this, &MainMenu::onMultiplayerButtonPressed));
    addMainMenuButton("Options (CTRL + O)")
        ->setClickCallback(fastdelegate::MakeDelegate(this, &MainMenu::onOptionsButtonPressed));
    addMainMenuButton("Exit")->setClickCallback(fastdelegate::MakeDelegate(this, &MainMenu::onExitButtonPressed));

    m_pauseButton = new MainMenuPauseButton(0, 0, 0, 0, "", "");
    m_pauseButton->setClickCallback(fastdelegate::MakeDelegate(this, &MainMenu::onPausePressed));
    addBaseUIElement(m_pauseButton);

    if(env->getOS() == Environment::OS::WINDOWS) {
        m_updateAvailableButton = new UIButton(0, 0, 0, 0, "", "Checking for updates ...");
        m_updateAvailableButton->setUseDefaultSkin();
        m_updateAvailableButton->setClickCallback(fastdelegate::MakeDelegate(this, &MainMenu::onUpdatePressed));
        m_updateAvailableButton->setColor(0x2200ff00);
        m_updateAvailableButton->setTextColor(0x22ffffff);
    }

    m_versionButton = new CBaseUIButton(0, 0, 0, 0, "", "");
    UString versionString = UString::format("Version %.2f", cv_version.getFloat());
    m_versionButton->setText(versionString);
    m_versionButton->setDrawBackground(false);
    m_versionButton->setDrawFrame(false);
    m_versionButton->setClickCallback(fastdelegate::MakeDelegate(this, &MainMenu::onVersionPressed));
    addBaseUIElement(m_versionButton);
}

MainMenu::~MainMenu() {
    SAFE_DELETE(preloaded_beatmapset);
    SAFE_DELETE(m_updateAvailableButton);

    anim->deleteExistingAnimation(&m_fUpdateButtonAnim);

    anim->deleteExistingAnimation(&m_fMainMenuAnimFriendEyeFollowX);
    anim->deleteExistingAnimation(&m_fMainMenuAnimFriendEyeFollowY);

    anim->deleteExistingAnimation(&m_fMainMenuAnim);
    anim->deleteExistingAnimation(&m_fMainMenuAnim1);
    anim->deleteExistingAnimation(&m_fMainMenuAnim2);
    anim->deleteExistingAnimation(&m_fMainMenuAnim3);

    anim->deleteExistingAnimation(&m_fCenterOffsetAnim);
    anim->deleteExistingAnimation(&m_fStartupAnim);
    anim->deleteExistingAnimation(&m_fStartupAnim2);

    // if the user didn't click on the update notification during this session, quietly remove it so it's not annoying
    if(m_bWasCleanShutdown) writeVersionFile();
}

void MainMenu::draw(Graphics *g) {
    if(!m_bVisible) return;

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

            bancho.server_icon = engine->getResourceManager()->loadImageAbs(icon_path.toUtf8(), icon_path.toUtf8());
            bancho.server_icon_url = "";
        }
    }

    // menu-background
    if(cv_draw_menu_background.getBool()) {
        Image *backgroundImage = osu->getSkin()->getMenuBackground();
        if(backgroundImage != NULL && backgroundImage != osu->getSkin()->getMissingTexture() &&
           backgroundImage->isReady()) {
            const float scale = Osu::getImageScaleToFillResolution(backgroundImage, osu->getScreenSize());

            g->setColor(0xffffffff);
            g->setAlpha(m_fStartupAnim);
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
            m_fBackgroundFadeInTime = engine->getTime();
        else if(m_fBackgroundFadeInTime > 0.0f && engine->getTime() > m_fBackgroundFadeInTime) {
            alpha = clamp<float>(
                (engine->getTime() - m_fBackgroundFadeInTime) / cv_songbrowser_background_fade_in_duration.getFloat(),
                0.0f, 1.0f);
            alpha = 1.0f - (1.0f - alpha) * (1.0f - alpha);
        }
    }

    // background_shader->enable();
    // background_shader->setUniform1f("time", engine->getTime());
    // background_shader->setUniform2f("resolution", osu->getScreenWidth(), osu->getScreenHeight());
    SongBrowser::drawSelectedBeatmapBackgroundImage(g, alpha);
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
            (long)(cv_universal_offset.getFloat() * osu->getSpeedMultiplier()) +
            (long)cv_universal_offset_hardcoded.getInt() -
            osu->getSelectedBeatmap()->getSelectedDifficulty2()->getLocalOffset() -
            osu->getSelectedBeatmap()->getSelectedDifficulty2()->getOnlineOffset() -
            (osu->getSelectedBeatmap()->getSelectedDifficulty2()->getVersion() < 5 ? cv_old_beatmap_offset.getInt()
                                                                                   : 0);

        DatabaseBeatmap::TIMING_INFO t =
            osu->getSelectedBeatmap()->getSelectedDifficulty2()->getTimingInfoForTime(curMusicPos);

        if(t.beatLengthBase == 0.0f)  // bah
            t.beatLengthBase = 1.0f;

        m_iMainMenuAnimBeatCounter = (curMusicPos - t.offset - (long)(max((long)t.beatLengthBase, (long)1) * 0.5f)) /
                                     max((long)t.beatLengthBase, (long)1);
        pulse = (float)((curMusicPos - t.offset) % max((long)t.beatLengthBase, (long)1)) /
                t.beatLengthBase;  // modulo must be >= 1
        pulse = clamp<float>(pulse, -1.0f, 1.0f);
        if(pulse < 0.0f) pulse = 1.0f - std::abs(pulse);
    } else
        pulse = (div - fmod(engine->getTime(), div)) / div;

    Vector2 size = m_vSize;
    const float pulseSub = 0.05f * pulse;
    size -= size * pulseSub;
    size += size * m_fSizeAddAnim;
    size *= m_fStartupAnim;
    McRect mainButtonRect =
        McRect(m_vCenter.x - size.x / 2.0f - m_fCenterOffsetAnim, m_vCenter.y - size.y / 2.0f, size.x, size.y);

    // draw notification arrow for changelog (version button)
    if(m_bDrawVersionNotificationArrow) {
        float animation = fmod((float)(engine->getTime()) * 3.2f, 2.0f);
        if(animation > 1.0f) animation = 2.0f - animation;
        animation = -animation * (animation - 2);  // quad out
        float offset = osu->getUIScale(45.0f * animation);

        const float scale = m_versionButton->getSize().x / osu->getSkin()->getPlayWarningArrow2()->getSizeBaseRaw().x;

        const Vector2 arrowPos =
            Vector2(m_versionButton->getSize().x / 1.75f,
                    osu->getScreenHeight() - m_versionButton->getSize().y * 2 - m_versionButton->getSize().y * scale);

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
            osu->getSkin()->getPlayWarningArrow2()->drawRaw(g, arrowPos, scale);
        }
        g->popTransform();
    }

    // draw container
    OsuScreen::draw(g);

    // draw update check button
    if(m_updateAvailableButton != NULL) {
        if(osu->getUpdateHandler()->getStatus() == UpdateHandler::STATUS::STATUS_SUCCESS_INSTALLATION) {
            g->push3DScene(McRect(m_updateAvailableButton->getPos().x, m_updateAvailableButton->getPos().y,
                                  m_updateAvailableButton->getSize().x, m_updateAvailableButton->getSize().y));
            g->rotate3DScene(m_fUpdateButtonAnim * 360.0f, 0, 0);
        }
        m_updateAvailableButton->draw(g);
        if(osu->getUpdateHandler()->getStatus() == UpdateHandler::STATUS::STATUS_SUCCESS_INSTALLATION) g->pop3DScene();
    }

    // draw main button
    bool drawing_full_cube = (m_fMainMenuAnim > 0.0f && m_fMainMenuAnim != 1.0f) ||
                             (haveTimingpoints && m_fMainMenuAnimFriendPercent > 0.0f);

    float inset = 0.0f;
    if(drawing_full_cube) {
        inset = 1.0f - 0.5 * m_fMainMenuAnimFriendPercent;

        g->setDepthBuffer(true);
        g->clearDepthBuffer();
        g->setCulling(true);

        g->push3DScene(mainButtonRect);
        g->offset3DScene(0, 0, mainButtonRect.getWidth() / 2);

        float friendRotation = 0.0f;
        float friendTranslationX = 0.0f;
        float friendTranslationY = 0.0f;
        if(haveTimingpoints && m_fMainMenuAnimFriendPercent > 0.0f) {
            float customPulse = 0.0f;
            if(pulse > 0.5f)
                customPulse = (pulse - 0.5f) / 0.5f;
            else
                customPulse = (0.5f - pulse) / 0.5f;

            customPulse = 1.0f - customPulse;

            const float anim = lerp<float>((1.0f - customPulse) * (1.0f - customPulse), (1.0f - customPulse), 0.25f);
            const float anim2 = anim * (m_iMainMenuAnimBeatCounter % 2 == 1 ? 1.0f : -1.0f);
            const float anim3 = anim;

            friendRotation = anim2 * 13;
            friendTranslationX = -anim2 * mainButtonRect.getWidth() * 0.175;
            friendTranslationY = anim3 * mainButtonRect.getWidth() * 0.10;

            friendRotation *= m_fMainMenuAnimFriendPercent;
            friendTranslationX *= m_fMainMenuAnimFriendPercent;
            friendTranslationY *= m_fMainMenuAnimFriendPercent;
        }

        g->translate3DScene(friendTranslationX, friendTranslationY, 0);
        g->rotate3DScene(m_fMainMenuAnim1 * 360.0f, m_fMainMenuAnim2 * 360.0f,
                         m_fMainMenuAnim3 * 360.0f + friendRotation);
    }

    const Color cubeColor = COLORf(1.0f, lerp<float>(0.0f, 0.5f, m_fMainMenuAnimFriendPercent),
                                   lerp<float>(0.0f, 0.768f, m_fMainMenuAnimFriendPercent),
                                   lerp<float>(0.0f, 0.965f, m_fMainMenuAnimFriendPercent));
    const Color cubeBorderColor = COLORf(1.0f, lerp<float>(1.0f, 0.5f, m_fMainMenuAnimFriendPercent),
                                         lerp<float>(1.0f, 0.768f, m_fMainMenuAnimFriendPercent),
                                         lerp<float>(1.0f, 0.965f, m_fMainMenuAnimFriendPercent));

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
    if(m_fMainMenuAnimFriendPercent > 0.0f) {
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
            g->setAlpha(m_fMainMenuAnimFriendPercent * cv_main_menu_alpha.getFloat());
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
            headBob *= m_fMainMenuAnimFriendPercent;
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
                                 m_fMainMenuAnimFriendEyeFollowX * mainButtonRect.getWidth() * 0.5f,
                             mainButtonRect.getY() + offsetY -
                                 m_fMainMenuAnimFriendEyeFollowY * mainButtonRect.getWidth() * 0.5f);

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
                    mainButtonRect.getX() + offsetX - m_fMainMenuAnimFriendEyeFollowX * mainButtonRect.getWidth(),
                    mainButtonRect.getY() + offsetY - m_fMainMenuAnimFriendEyeFollowY * mainButtonRect.getWidth());

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
                        m_fMainMenuAnimFriendEyeFollowX * mainButtonRect.getWidth(),
                    mainButtonRect.getY() + offsetY - m_fMainMenuAnimFriendEyeFollowY * mainButtonRect.getWidth());

                g->setColor(0xff000000);
                g->setAlpha(cv_main_menu_alpha.getFloat());
                g->fillRect(0, 0, width, height);
            }
            g->popTransform();

            // tear
            g->setColor(0xff000000);
            g->setAlpha(cv_main_menu_alpha.getFloat());
            g->fillRect(mainButtonRect.getX() + offsetX + width * 0.375f -
                            m_fMainMenuAnimFriendEyeFollowX * mainButtonRect.getWidth(),
                        mainButtonRect.getY() + offsetY + width / 2.0f -
                            m_fMainMenuAnimFriendEyeFollowY * mainButtonRect.getWidth(),
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

            const float animLeftMultiplier = (m_iMainMenuAnimBeatCounter % 2 == 0 ? 1.0f : 0.1f);
            const float animRightMultiplier = (m_iMainMenuAnimBeatCounter % 2 == 1 ? 1.0f : 0.1f);

            const float animMoveUp =
                lerp<float>((1.0f - customPulse) * (1.0f - customPulse), (1.0f - customPulse), 0.35f) *
                m_fMainMenuAnimFriendPercent;

            const float animLeftMoveUp = animMoveUp * animLeftMultiplier;
            const float animRightMoveUp = animMoveUp * animRightMultiplier;

            const float animLeftMoveLeft = animRightMoveUp * (m_iMainMenuAnimBeatCounter % 2 == 1 ? 1.0f : 0.0f);
            const float animRightMoveRight = animLeftMoveUp * (m_iMainMenuAnimBeatCounter % 2 == 0 ? 1.0f : 0.0f);

            // left
            g->setColor(0xffd5f6fd);
            g->setAlpha(m_fMainMenuAnimFriendPercent * cv_main_menu_alpha.getFloat());
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
        auto logo = logo_img;
        if(bancho.server_icon != NULL && bancho.server_icon->isReady() && cv_main_menu_use_server_logo.getBool()) {
            logo = bancho.server_icon;
        }

        float alpha = (1.0f - m_fMainMenuAnimFriendPercent) * (1.0f - m_fMainMenuAnimFriendPercent) *
                      (1.0f - m_fMainMenuAnimFriendPercent);

        float xscale = mainButtonRect.getWidth() / logo->getWidth();
        float yscale = mainButtonRect.getHeight() / logo->getHeight();
        float scale = min(xscale, yscale) * 0.8f;

        g->pushTransform();
        g->setColor(0xffffffff);
        g->setAlpha(alpha);
        g->scale(scale, scale);
        g->translate(m_vCenter.x - m_fCenterOffsetAnim, m_vCenter.y);
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
    if(!m_bVisible) return;

    updateLayout();

    // update and focus handling
    OsuScreen::mouse_update(propagate_clicks);

    if(m_updateAvailableButton != NULL) {
        m_updateAvailableButton->mouse_update(propagate_clicks);
    }

    // handle automatic menu closing
    if(m_fMainMenuButtonCloseTime != 0.0f && engine->getTime() > m_fMainMenuButtonCloseTime) {
        m_fMainMenuButtonCloseTime = 0.0f;
        setMenuElementsVisible(false);
    }

    // hide the buttons if the closing animation finished
    if(!anim->isAnimating(&m_fCenterOffsetAnim) && m_fCenterOffsetAnim == 0.0f) {
        for(int i = 0; i < m_menuElements.size(); i++) {
            m_menuElements[i]->setVisible(false);
        }
    }

    // handle delayed shutdown
    if(m_fShutdownScheduledTime != 0.0f &&
       (engine->getTime() > m_fShutdownScheduledTime || !anim->isAnimating(&m_fCenterOffsetAnim))) {
        engine->shutdown();
        m_fShutdownScheduledTime = 0.0f;
    }

    // main button autohide + anim
    if(m_bMenuElementsVisible) {
        m_fMainMenuAnimDuration = 15.0f;
        m_fMainMenuAnimTime = engine->getTime() + m_fMainMenuAnimDuration;
    }
    if(engine->getTime() > m_fMainMenuAnimTime) {
        if(m_bMainMenuAnimFriendScheduled) m_bMainMenuAnimFriend = true;
        if(m_bMainMenuAnimFadeToFriendForNextAnim) m_bMainMenuAnimFriendScheduled = true;

        m_fMainMenuAnimDuration = 10.0f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 5.0f;
        m_fMainMenuAnimTime = engine->getTime() + m_fMainMenuAnimDuration;
        animMainButton();
    }

    if(m_bInMainMenuRandomAnim && m_iMainMenuRandomAnimType == 1 && anim->isAnimating(&m_fMainMenuAnim)) {
        Vector2 mouseDelta = (m_cube->getPos() + m_cube->getSize() / 2) - engine->getMouse()->getPos();
        mouseDelta.x = clamp<float>(mouseDelta.x, -engine->getScreenSize().x / 2, engine->getScreenSize().x / 2);
        mouseDelta.y = clamp<float>(mouseDelta.y, -engine->getScreenSize().y / 2, engine->getScreenSize().y / 2);
        mouseDelta.x /= engine->getScreenSize().x;
        mouseDelta.y /= engine->getScreenSize().y;

        const float decay = clamp<float>((1.0f - m_fMainMenuAnim - 0.075f) / 0.025f, 0.0f, 1.0f);

        const Vector2 pushAngle = Vector2(mouseDelta.y, -mouseDelta.x) * Vector2(0.15f, 0.15f) * decay;

        anim->moveQuadOut(&m_fMainMenuAnim1, pushAngle.x, 0.15f, true);

        anim->moveQuadOut(&m_fMainMenuAnim2, pushAngle.y, 0.15f, true);

        anim->moveQuadOut(&m_fMainMenuAnim3, 0.0f, 0.15f, true);
    }

    {
        m_fMainMenuAnimFriendPercent =
            1.0f - clamp<float>((m_fMainMenuAnimDuration > 0.0f
                                     ? (m_fMainMenuAnimTime - engine->getTime()) / m_fMainMenuAnimDuration
                                     : 0.0f),
                                0.0f, 1.0f);
        m_fMainMenuAnimFriendPercent = clamp<float>((m_fMainMenuAnimFriendPercent - 0.5f) / 0.5f, 0.0f, 1.0f);
        if(m_bMainMenuAnimFriend) m_fMainMenuAnimFriendPercent = 1.0f;
        if(!m_bMainMenuAnimFriendScheduled) m_fMainMenuAnimFriendPercent = 0.0f;

        Vector2 mouseDelta = (m_cube->getPos() + m_cube->getSize() / 2) - engine->getMouse()->getPos();
        mouseDelta.x = clamp<float>(mouseDelta.x, -engine->getScreenSize().x / 2, engine->getScreenSize().x / 2);
        mouseDelta.y = clamp<float>(mouseDelta.y, -engine->getScreenSize().y / 2, engine->getScreenSize().y / 2);
        mouseDelta.x /= engine->getScreenSize().x;
        mouseDelta.y /= engine->getScreenSize().y;

        const Vector2 pushAngle = Vector2(mouseDelta.x, mouseDelta.y) * 0.1f;

        anim->moveLinear(&m_fMainMenuAnimFriendEyeFollowX, pushAngle.x, 0.25f, true);
        anim->moveLinear(&m_fMainMenuAnimFriendEyeFollowY, pushAngle.y, 0.25f, true);
    }

    // handle update checker and status text
    if(m_updateAvailableButton != NULL) {
        switch(osu->getUpdateHandler()->getStatus()) {
            case UpdateHandler::STATUS::STATUS_UP_TO_DATE:
                if(m_updateAvailableButton->isVisible()) {
                    m_updateAvailableButton->setText("");
                    m_updateAvailableButton->setVisible(false);
                }
                break;
            case UpdateHandler::STATUS::STATUS_CHECKING_FOR_UPDATE:
                m_updateAvailableButton->setText("Checking for updates ...");
                break;
            case UpdateHandler::STATUS::STATUS_DOWNLOADING_UPDATE:
                m_updateAvailableButton->setText("Downloading ...");
                break;
            case UpdateHandler::STATUS::STATUS_INSTALLING_UPDATE:
                m_updateAvailableButton->setText("Installing ...");
                break;
            case UpdateHandler::STATUS::STATUS_SUCCESS_INSTALLATION:
                if(engine->getTime() > m_fUpdateButtonTextTime && anim->isAnimating(&m_fUpdateButtonAnim) &&
                   m_fUpdateButtonAnim > 0.175f) {
                    m_fUpdateButtonTextTime = m_fUpdateButtonAnimTime;

                    m_updateAvailableButton->setColor(0xff00ff00);
                    m_updateAvailableButton->setTextColor(0xffffffff);

                    if(m_updateAvailableButton->getText().find("ready") != -1)
                        m_updateAvailableButton->setText("Click here to restart now!");
                    else
                        m_updateAvailableButton->setText("A new version of neosu is ready!");
                }
                break;
            case UpdateHandler::STATUS::STATUS_ERROR:
                m_updateAvailableButton->setText("Update Error! Click to retry ...");
                break;
        }
    }

    if(osu->getUpdateHandler()->getStatus() == UpdateHandler::STATUS::STATUS_SUCCESS_INSTALLATION &&
       engine->getTime() > m_fUpdateButtonAnimTime) {
        m_fUpdateButtonAnimTime = engine->getTime() + 3.0f;
        m_fUpdateButtonAnim = 0.0f;
        anim->moveQuadInOut(&m_fUpdateButtonAnim, 1.0f, 0.5f, true);
    }

    // Update pause button and shuffle songs
    m_pauseButton->setPaused(true);

    if(engine->getSound()->isReady()) {
        auto diff2 = osu->getSelectedBeatmap()->getSelectedDifficulty2();
        auto music = osu->getSelectedBeatmap()->getMusic();
        if(music == NULL) {
            selectRandomBeatmap();
        } else {
            if(music->isFinished()) {
                selectRandomBeatmap();
            } else if(music->isPlaying()) {
                m_pauseButton->setPaused(false);

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
    if(sb->getDatabase()->isFinished() && !sb->m_beatmaps.empty()) {
        osu->getSongBrowser()->selectRandomBeatmap();
    } else {
        // Database is not loaded yet, load a random map and select it
        auto songs_folder = osu->getSongBrowser()->getDatabase()->getOsuSongsFolder();
        auto mapset_folders = env->getFoldersInFolder(songs_folder);
        auto mapset_folders2 = env->getFoldersInFolder(MCENGINE_DATA_DIR "maps/");
        auto nb_mapsets = mapset_folders.size();
        auto nb_mapsets2 = mapset_folders2.size();
        if(nb_mapsets + nb_mapsets2 == 0) return;

        osu->getSongBrowser()->getSelectedBeatmap()->deselect();
        SAFE_DELETE(preloaded_beatmapset);

        for(int i = 0; i < 10; i++) {
            u32 lucky_number = rand() % (nb_mapsets + nb_mapsets2);
            bool is_neosu_set = lucky_number > nb_mapsets;
            if(is_neosu_set) lucky_number -= nb_mapsets;

            auto mapset_folder = is_neosu_set ? MCENGINE_DATA_DIR "maps/" : songs_folder;
            mapset_folder.append((is_neosu_set ? mapset_folders2 : mapset_folders)[lucky_number]);
            mapset_folder.append("/");

            BeatmapSet *set = osu->getSongBrowser()->getDatabase()->loadRawBeatmap(mapset_folder);
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

            preloaded_beatmapset = set;

            // We're picking a random diff and not the first one, because diffs of the same set
            // can have their own separate sound file.
            preloaded_beatmap = beatmap_diffs[rand() % beatmap_diffs.size()];
            preloaded_beatmap->do_not_store = true;

            osu->getSongBrowser()->onDifficultySelected(preloaded_beatmap, false);

            return;
        }

        debugLog("Failed to pick random beatmap...\n");
    }
}

void MainMenu::onKeyDown(KeyboardEvent &e) {
    OsuScreen::onKeyDown(e);  // only used for options menu
    if(!m_bVisible || e.isConsumed()) return;

    if(!osu->getOptionsMenu()->isMouseInside()) {
        if(e == KEY_RIGHT || e == KEY_F2) selectRandomBeatmap();
    }

    if(e == KEY_C || e == KEY_F4) onPausePressed();

    if(!m_bMenuElementsVisible) {
        if(e == KEY_P || e == KEY_ENTER) m_cube->click();
    } else {
        if(e == KEY_P || e == KEY_ENTER) onPlayButtonPressed();
        if(e == KEY_O) onOptionsButtonPressed();
        if(e == KEY_E || e == KEY_X) onExitButtonPressed();

        if(e == KEY_ESCAPE) setMenuElementsVisible(false);
    }
}

void MainMenu::onMiddleChange(bool down) {
    if(!m_bVisible) return;

    // debug anims
    if(down && !anim->isAnimating(&m_fMainMenuAnim) && !m_bMenuElementsVisible) {
        if(engine->getKeyboard()->isShiftDown()) {
            m_bMainMenuAnimFriend = true;
            m_bMainMenuAnimFriendScheduled = true;
            m_bMainMenuAnimFadeToFriendForNextAnim = true;
        }

        animMainButton();
        m_fMainMenuAnimDuration = 15.0f;
        m_fMainMenuAnimTime = engine->getTime() + m_fMainMenuAnimDuration;
    }
}

void MainMenu::onResolutionChange(Vector2 newResolution) {
    updateLayout();
    setMenuElementsVisible(m_bMenuElementsVisible);
}

CBaseUIContainer *MainMenu::setVisible(bool visible) {
    m_bVisible = visible;

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

        updateLayout();

        m_fMainMenuAnimDuration = 15.0f;
        m_fMainMenuAnimTime = engine->getTime() + m_fMainMenuAnimDuration;

        if(m_bStartupAnim) {
            m_bStartupAnim = false;
            anim->moveQuadOut(&m_fStartupAnim, 1.0f, cv_main_menu_startup_anim_duration.getFloat(),
                              (float)engine->getTimeReal());
            anim->moveQuartOut(&m_fStartupAnim2, 1.0f, cv_main_menu_startup_anim_duration.getFloat() * 6.0f,
                               (float)engine->getTimeReal() + cv_main_menu_startup_anim_duration.getFloat() * 0.5f);
        }
    } else {
        setMenuElementsVisible(false, false);
    }

    return this;
}

void MainMenu::updateLayout() {
    const float dpiScale = Osu::getUIScale();

    m_vCenter = osu->getScreenSize() / 2.0f;
    const float size = Osu::getUIScale(324.0f);
    m_vSize = Vector2(size, size);

    m_cube->setRelPos(m_vCenter - m_vSize / 2.0f - Vector2(m_fCenterOffsetAnim, 0.0f));
    m_cube->setSize(m_vSize);

    m_pauseButton->setSize(30 * dpiScale, 30 * dpiScale);
    m_pauseButton->setRelPos(osu->getScreenWidth() - m_pauseButton->getSize().x * 2 - 10 * dpiScale,
                             m_pauseButton->getSize().y + 10 * dpiScale);

    if(m_updateAvailableButton != NULL) {
        m_updateAvailableButton->setSize(375 * dpiScale, 50 * dpiScale);
        m_updateAvailableButton->setPos(osu->getScreenWidth() / 2 - m_updateAvailableButton->getSize().x / 2,
                                        osu->getScreenHeight() - m_updateAvailableButton->getSize().y - 10 * dpiScale);
    }

    m_versionButton->onResized();  // HACKHACK: framework, setSizeToContent() does not update string metrics
    m_versionButton->setSizeToContent(8 * dpiScale, 8 * dpiScale);
    m_versionButton->setRelPos(-1, osu->getScreenSize().y - m_versionButton->getSize().y);

    int numButtons = m_menuElements.size();
    int menuElementHeight = m_vSize.y / numButtons;
    int menuElementPadding = numButtons > 3 ? m_vSize.y * 0.04f : m_vSize.y * 0.075f;
    menuElementHeight -= (numButtons - 1) * menuElementPadding;
    int menuElementExtraWidth = m_vSize.x * 0.06f;

    float offsetPercent = m_fCenterOffsetAnim / (m_vSize.x / 2.0f);
    float curY = m_cube->getRelPos().y +
                 (m_vSize.y - menuElementHeight * numButtons - (numButtons - 1) * menuElementPadding) / 2.0f;
    for(int i = 0; i < m_menuElements.size(); i++) {
        curY += (i > 0 ? menuElementHeight + menuElementPadding : 0.0f);

        m_menuElements[i]->onResized();  // HACKHACK: framework, setSize() does not update string metrics
        m_menuElements[i]->setRelPos(m_cube->getRelPos().x + m_cube->getSize().x * offsetPercent -
                                         menuElementExtraWidth * offsetPercent +
                                         menuElementExtraWidth * (1.0f - offsetPercent),
                                     curY);
        m_menuElements[i]->setSize(m_cube->getSize().x + menuElementExtraWidth * offsetPercent -
                                       2.0f * menuElementExtraWidth * (1.0f - offsetPercent),
                                   menuElementHeight);
        m_menuElements[i]->setTextColor(
            COLORf(offsetPercent * offsetPercent * offsetPercent * offsetPercent, 1.0f, 1.0f, 1.0f));
        m_menuElements[i]->setFrameColor(COLORf(offsetPercent, 1.0f, 1.0f, 1.0f));
        m_menuElements[i]->setBackgroundColor(COLORf(offsetPercent * cv_main_menu_alpha.getFloat(), 0.0f, 0.0f, 0.0f));
    }

    setSize(osu->getScreenSize() + Vector2(1, 1));
    update_pos();
}

void MainMenu::animMainButton() {
    m_bInMainMenuRandomAnim = true;

    m_iMainMenuRandomAnimType = (rand() % 4) == 1 ? 1 : 0;
    if(!m_bMainMenuAnimFadeToFriendForNextAnim && cv_main_menu_friend.getBool() &&
       env->getOS() == Environment::OS::WINDOWS)  // NOTE: z buffer bullshit on other platforms >:(
        m_bMainMenuAnimFadeToFriendForNextAnim = (rand() % 24) == 1;

    m_fMainMenuAnim = 0.0f;
    m_fMainMenuAnim1 = 0.0f;
    m_fMainMenuAnim2 = 0.0f;

    if(m_iMainMenuRandomAnimType == 0) {
        m_fMainMenuAnim3 = 1.0f;

        m_fMainMenuAnim1Target = (rand() % 2) == 1 ? 1.0f : -1.0f;
        m_fMainMenuAnim2Target = (rand() % 2) == 1 ? 1.0f : -1.0f;
        m_fMainMenuAnim3Target = (rand() % 2) == 1 ? 1.0f : -1.0f;

        const float randomDuration1 = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 3.5f;
        const float randomDuration2 = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 3.5f;
        const float randomDuration3 = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 3.5f;

        anim->moveQuadOut(&m_fMainMenuAnim, 1.0f, 1.5f + max(randomDuration1, max(randomDuration2, randomDuration3)));
        anim->moveQuadOut(&m_fMainMenuAnim1, m_fMainMenuAnim1Target, 1.5f + randomDuration1);
        anim->moveQuadOut(&m_fMainMenuAnim2, m_fMainMenuAnim2Target, 1.5f + randomDuration2);
        anim->moveQuadOut(&m_fMainMenuAnim3, m_fMainMenuAnim3Target, 1.5f + randomDuration3);
    } else {
        m_fMainMenuAnim3 = 0.0f;

        m_fMainMenuAnim1Target = 0.0f;
        m_fMainMenuAnim2Target = 0.0f;
        m_fMainMenuAnim3Target = 0.0f;

        m_fMainMenuAnim = 0.0f;
        anim->moveQuadOut(&m_fMainMenuAnim, 1.0f, 5.0f);
    }
}

void MainMenu::animMainButtonBack() {
    m_bInMainMenuRandomAnim = false;

    if(anim->isAnimating(&m_fMainMenuAnim)) {
        anim->moveQuadOut(&m_fMainMenuAnim, 1.0f, 0.25f, true);
        anim->moveQuadOut(&m_fMainMenuAnim1, m_fMainMenuAnim1Target, 0.25f, true);
        anim->moveQuadOut(&m_fMainMenuAnim1, 0.0f, 0.0f, 0.25f);
        anim->moveQuadOut(&m_fMainMenuAnim2, m_fMainMenuAnim2Target, 0.25f, true);
        anim->moveQuadOut(&m_fMainMenuAnim2, 0.0f, 0.0f, 0.25f);
        anim->moveQuadOut(&m_fMainMenuAnim3, m_fMainMenuAnim3Target, 0.10f, true);
        anim->moveQuadOut(&m_fMainMenuAnim3, 0.0f, 0.0f, 0.1f);
    }
}

void MainMenu::setMenuElementsVisible(bool visible, bool animate) {
    m_bMenuElementsVisible = visible;

    if(visible) {
        if(m_bMenuElementsVisible &&
           m_vSize.x / 2.0f <
               m_fCenterOffsetAnim)  // so we don't see the ends of the menu element buttons if the window gets smaller
            m_fCenterOffsetAnim = m_vSize.x / 2.0f;

        if(animate)
            anim->moveQuadInOut(&m_fCenterOffsetAnim, m_vSize.x / 2.0f, 0.35f, 0.0f, true);
        else {
            anim->deleteExistingAnimation(&m_fCenterOffsetAnim);
            m_fCenterOffsetAnim = m_vSize.x / 2.0f;
        }

        m_fMainMenuButtonCloseTime = engine->getTime() + 6.0f;

        for(int i = 0; i < m_menuElements.size(); i++) {
            m_menuElements[i]->setVisible(true);
            m_menuElements[i]->setEnabled(true);
        }
    } else {
        if(animate)
            anim->moveQuadOut(
                &m_fCenterOffsetAnim, 0.0f,
                0.5f * (m_fCenterOffsetAnim / (m_vSize.x / 2.0f)) * (m_fShutdownScheduledTime != 0.0f ? 0.4f : 1.0f),
                0.0f, true);
        else {
            anim->deleteExistingAnimation(&m_fCenterOffsetAnim);
            m_fCenterOffsetAnim = 0.0f;
        }

        m_fMainMenuButtonCloseTime = 0.0f;

        for(int i = 0; i < m_menuElements.size(); i++) {
            m_menuElements[i]->setEnabled(false);
        }
    }
}

void MainMenu::writeVersionFile() {
    // remember, don't show the notification arrow until the version changes again
    std::ofstream versionFile(NEOSU_NEWVERSION_NOTIFICATION_TRIGGER_FILE, std::ios::out | std::ios::trunc);
    if(versionFile.good()) versionFile << cv_version.getFloat();
}

MainMenuButton *MainMenu::addMainMenuButton(UString text) {
    MainMenuButton *button = new MainMenuButton(this, m_vSize.x, 0, 1, 1, "", text);
    button->setFont(osu->getSubTitleFont());
    button->setVisible(false);

    m_menuElements.push_back(button);
    addBaseUIElement(button);
    return button;
}

void MainMenu::onCubePressed() {
    engine->getSound()->play(osu->getSkin()->m_clickMainMenuCube);

    anim->moveQuadInOut(&m_fSizeAddAnim, 0.0f, 0.06f, 0.0f, false);
    anim->moveQuadInOut(&m_fSizeAddAnim, 0.12f, 0.06f, 0.07f, false);

    // if the menu is already visible, this counts as pressing the play button
    if(m_bMenuElementsVisible)
        onPlayButtonPressed();
    else
        setMenuElementsVisible(true);

    if(anim->isAnimating(&m_fMainMenuAnim) && m_bInMainMenuRandomAnim)
        animMainButtonBack();
    else {
        m_bInMainMenuRandomAnim = false;

        Vector2 mouseDelta = (m_cube->getPos() + m_cube->getSize() / 2) - engine->getMouse()->getPos();
        mouseDelta.x = clamp<float>(mouseDelta.x, -m_cube->getSize().x / 2, m_cube->getSize().x / 2);
        mouseDelta.y = clamp<float>(mouseDelta.y, -m_cube->getSize().y / 2, m_cube->getSize().y / 2);
        mouseDelta.x /= m_cube->getSize().x;
        mouseDelta.y /= m_cube->getSize().y;

        const Vector2 pushAngle = Vector2(mouseDelta.y, -mouseDelta.x) * Vector2(0.15f, 0.15f);

        m_fMainMenuAnim = 0.001f;
        anim->moveQuadOut(&m_fMainMenuAnim, 1.0f, 0.15f + 0.4f, true);

        if(!anim->isAnimating(&m_fMainMenuAnim1)) m_fMainMenuAnim1 = 0.0f;

        anim->moveQuadOut(&m_fMainMenuAnim1, pushAngle.x, 0.15f, true);
        anim->moveQuadOut(&m_fMainMenuAnim1, 0.0f, 0.4f, 0.15f);

        if(!anim->isAnimating(&m_fMainMenuAnim2)) m_fMainMenuAnim2 = 0.0f;

        anim->moveQuadOut(&m_fMainMenuAnim2, pushAngle.y, 0.15f, true);
        anim->moveQuadOut(&m_fMainMenuAnim2, 0.0f, 0.4f, 0.15f);

        if(!anim->isAnimating(&m_fMainMenuAnim3)) m_fMainMenuAnim3 = 0.0f;

        anim->moveQuadOut(&m_fMainMenuAnim3, 0.0f, 0.15f, true);
    }
}

void MainMenu::onPlayButtonPressed() {
    m_bMainMenuAnimFriend = false;
    m_bMainMenuAnimFadeToFriendForNextAnim = false;
    m_bMainMenuAnimFriendScheduled = false;

    osu->toggleSongBrowser();

    engine->getSound()->play(osu->getSkin()->m_menuHit);
    engine->getSound()->play(osu->getSkin()->m_clickSingleplayer);
}

void MainMenu::onMultiplayerButtonPressed() {
    if(bancho.user_id <= 0) {
        osu->m_optionsMenu->askForLoginDetails();
        return;
    }

    setVisible(false);
    osu->m_lobby->setVisible(true);

    engine->getSound()->play(osu->getSkin()->m_menuHit);
    engine->getSound()->play(osu->getSkin()->m_clickMultiplayer);
}

void MainMenu::onOptionsButtonPressed() {
    if(!osu->getOptionsMenu()->isVisible()) osu->toggleOptionsMenu();

    engine->getSound()->play(osu->getSkin()->m_clickOptions);
}

void MainMenu::onExitButtonPressed() {
    m_fShutdownScheduledTime = engine->getTime() + 0.3f;
    m_bWasCleanShutdown = true;
    setMenuElementsVisible(false);

    engine->getSound()->play(osu->getSkin()->m_clickExit);
}

void MainMenu::onPausePressed() {
    if(osu->getSelectedBeatmap()->isPreviewMusicPlaying()) {
        osu->getSelectedBeatmap()->pausePreviewMusic();
    } else {
        auto music = osu->getSelectedBeatmap()->getMusic();
        if(music != NULL) {
            engine->getSound()->play(music);
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
    m_bDrawVersionNotificationArrow = false;
    writeVersionFile();
    osu->toggleChangelog();
}

MainMenuCubeButton::MainMenuCubeButton(MainMenu *mainMenu, float xPos, float yPos, float xSize, float ySize,
                                       UString name, UString text)
    : CBaseUIButton(xPos, yPos, xSize, ySize, name, text) {
    m_mainMenu = mainMenu;
}

void MainMenuCubeButton::draw(Graphics *g) {
    // draw nothing
}

void MainMenuCubeButton::onMouseInside() {
    anim->moveQuadInOut(&m_mainMenu->m_fSizeAddAnim, 0.12f, 0.15f, 0.0f, true);

    CBaseUIButton::onMouseInside();

    if(button_sound_cooldown + 0.05f < engine->getTime()) {
        engine->getSound()->play(osu->getSkin()->m_hoverMainMenuCube);
        button_sound_cooldown = engine->getTime();
    }
}

void MainMenuCubeButton::onMouseOutside() {
    anim->moveQuadInOut(&m_mainMenu->m_fSizeAddAnim, 0.0f, 0.15f, 0.0f, true);

    CBaseUIButton::onMouseOutside();
}

MainMenuButton::MainMenuButton(MainMenu *mainMenu, float xPos, float yPos, float xSize, float ySize, UString name,
                               UString text)
    : CBaseUIButton(xPos, yPos, xSize, ySize, name, text) {
    m_mainMenu = mainMenu;
}

void MainMenuButton::onMouseDownInside() {
    if(m_mainMenu->m_cube->isMouseInside()) return;
    CBaseUIButton::onMouseDownInside();
}

void MainMenuButton::onMouseInside() {
    if(m_mainMenu->m_cube->isMouseInside()) return;
    CBaseUIButton::onMouseInside();

    if(button_sound_cooldown + 0.05f < engine->getTime()) {
        if(getText() == UString("Singleplayer")) {
            engine->getSound()->play(osu->getSkin()->m_hoverSingleplayer);
        } else if(getText() == UString("Multiplayer")) {
            engine->getSound()->play(osu->getSkin()->m_hoverMultiplayer);
        } else if(getText() == UString("Options (CTRL + O)")) {
            engine->getSound()->play(osu->getSkin()->m_hoverOptions);
        } else if(getText() == UString("Exit")) {
            engine->getSound()->play(osu->getSkin()->m_hoverExit);
        }

        button_sound_cooldown = engine->getTime();
    }
}
