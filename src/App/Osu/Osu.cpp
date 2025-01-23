#include "Osu.h"

#include <sstream>

#include "AnimationHandler.h"
#include "BackgroundImageHandler.h"
#include "Bancho.h"
#include "BanchoNetworking.h"
#include "Beatmap.h"
#include "CBaseUIScrollView.h"
#include "CBaseUISlider.h"
#include "CBaseUITextbox.h"
#include "CWindowManager.h"
#include "Changelog.h"
#include "Chat.h"
#include "ConVar.h"
#include "Console.h"
#include "ConsoleBox.h"
#include "Database.h"
#include "DatabaseBeatmap.h"
#include "Downloader.h"
#include "Engine.h"
#include "Environment.h"
#include "GameRules.h"
#include "HUD.h"
#include "HitObject.h"
#include "Icons.h"
#include "KeyBindings.h"
#include "Keyboard.h"
#include "LegacyReplay.h"
#include "Lobby.h"
#include "MainMenu.h"
#include "ModFPoSu.h"
#include "ModSelector.h"
#include "Mouse.h"
#include "NotificationOverlay.h"
#include "OptionsMenu.h"
#include "PauseMenu.h"
#include "PeppyImporter.h"
#include "PromptScreen.h"
#include "RankingScreen.h"
#include "RenderTarget.h"
#include "ResourceManager.h"
#include "RichPresence.h"
#include "RoomScreen.h"
#include "Shader.h"
#include "Skin.h"
#include "SongBrowser/LeaderboardPPCalcThread.h"
#include "SongBrowser/LoudnessCalcThread.h"
#include "SongBrowser/MapCalcThread.h"
#include "SongBrowser/ScoreConverterThread.h"
#include "SongBrowser/SongBrowser.h"
#include "SoundEngine.h"
#include "SpectatorScreen.h"
#include "TooltipOverlay.h"
#include "UIModSelectorModButton.h"
#include "UIUserContextMenu.h"
#include "UpdateHandler.h"
#include "VolumeOverlay.h"
#include "score.h"

#ifdef _WIN32
#include "WinEnvironment.h"
#endif

using namespace std;

Osu *osu = NULL;

Vector2 Osu::g_vInternalResolution;
Vector2 Osu::osuBaseResolution = Vector2(640.0f, 480.0f);

Shader *actual_flashlight_shader = NULL;
Shader *flashlight_shader = NULL;

Osu::Osu() {
    osu = this;

    srand(time(NULL));

    bancho.neosu_version = UString::format("%.2f-" NEOSU_STREAM, cv_version.getFloat());
    bancho.user_agent = UString::format("Mozilla/5.0 (compatible; neosu/%s; +https://" NEOSU_DOMAIN "/)",
                                        bancho.neosu_version.toUtf8());

    // experimental mods list
    m_experimentalMods.push_back(&cv_fposu_mod_strafing);
    m_experimentalMods.push_back(&cv_mod_wobble);
    m_experimentalMods.push_back(&cv_mod_arwobble);
    m_experimentalMods.push_back(&cv_mod_timewarp);
    m_experimentalMods.push_back(&cv_mod_artimewarp);
    m_experimentalMods.push_back(&cv_mod_minimize);
    m_experimentalMods.push_back(&cv_mod_fadingcursor);
    m_experimentalMods.push_back(&cv_mod_fps);
    m_experimentalMods.push_back(&cv_mod_jigsaw1);
    m_experimentalMods.push_back(&cv_mod_jigsaw2);
    m_experimentalMods.push_back(&cv_mod_fullalternate);
    m_experimentalMods.push_back(&cv_mod_reverse_sliders);
    m_experimentalMods.push_back(&cv_mod_no50s);
    m_experimentalMods.push_back(&cv_mod_no100s);
    m_experimentalMods.push_back(&cv_mod_ming3012);
    m_experimentalMods.push_back(&cv_mod_halfwindow);
    m_experimentalMods.push_back(&cv_mod_millhioref);
    m_experimentalMods.push_back(&cv_mod_mafham);
    m_experimentalMods.push_back(&cv_mod_strict_tracking);
    m_experimentalMods.push_back(&cv_playfield_mirror_horizontal);
    m_experimentalMods.push_back(&cv_playfield_mirror_vertical);
    m_experimentalMods.push_back(&cv_mod_wobble2);
    m_experimentalMods.push_back(&cv_mod_shirone);
    m_experimentalMods.push_back(&cv_mod_approach_different);

    env->setWindowTitle("neosu");
    env->setCursorVisible(false);

    engine->getConsoleBox()->setRequireShiftToActivate(true);
    engine->getMouse()->addListener(this);

    cv_vsync.setValue(0.0f);
    cv_fps_max.setValue(420.0f);
    cv_fps_max.setDefaultFloat(420.0f);

    cv_snd_change_check_interval.setDefaultFloat(0.5f);
    cv_snd_change_check_interval.setValue(cv_snd_change_check_interval.getDefaultFloat());

    env->setWindowResizable(false);

    // convar callbacks
    cv_resolution.setValue(UString::format("%ix%i", engine->getScreenWidth(), engine->getScreenHeight()));
    cv_resolution.setCallback(fastdelegate::MakeDelegate(this, &Osu::onInternalResolutionChanged));
    cv_animation_speed_override.setCallback(fastdelegate::MakeDelegate(this, &Osu::onAnimationSpeedChange));
    cv_ui_scale.setCallback(fastdelegate::MakeDelegate(this, &Osu::onUIScaleChange));
    cv_ui_scale_to_dpi.setCallback(fastdelegate::MakeDelegate(this, &Osu::onUIScaleToDPIChange));
    cv_letterboxing.setCallback(fastdelegate::MakeDelegate(this, &Osu::onLetterboxingChange));
    cv_letterboxing_offset_x.setCallback(fastdelegate::MakeDelegate(this, &Osu::onLetterboxingOffsetChange));
    cv_letterboxing_offset_y.setCallback(fastdelegate::MakeDelegate(this, &Osu::onLetterboxingOffsetChange));
    cv_confine_cursor_windowed.setCallback(fastdelegate::MakeDelegate(this, &Osu::onConfineCursorWindowedChange));
    cv_confine_cursor_fullscreen.setCallback(fastdelegate::MakeDelegate(this, &Osu::onConfineCursorFullscreenChange));

    // vars
    m_skin = NULL;
    m_backgroundImageHandler = NULL;
    m_modSelector = NULL;
    m_updateHandler = NULL;

    m_bF1 = false;
    m_bUIToggleCheck = false;
    m_bScoreboardToggleCheck = false;
    m_bEscape = false;
    m_bKeyboardKey1Down = false;
    m_bKeyboardKey12Down = false;
    m_bKeyboardKey2Down = false;
    m_bKeyboardKey22Down = false;
    m_bMouseKey1Down = false;
    m_bMouseKey2Down = false;
    m_bSkipScheduled = false;
    m_bQuickRetryDown = false;
    m_fQuickRetryTime = 0.0f;
    m_bSeeking = false;
    m_bSeekKey = false;
    m_fPrevSeekMousePosX = -1.0f;
    m_fQuickSaveTime = 0.0f;

    m_bToggleModSelectionScheduled = false;
    m_bToggleSongBrowserScheduled = false;
    m_bToggleOptionsMenuScheduled = false;
    m_bOptionsMenuFullscreen = true;
    m_bToggleChangelogScheduled = false;
    m_bToggleEditorScheduled = false;

    m_bShouldCursorBeVisible = false;

    m_bScheduleEndlessModNextBeatmap = false;
    m_iMultiplayerClientNumEscPresses = 0;
    m_bWasBossKeyPaused = false;
    m_bSkinLoadScheduled = false;
    m_bSkinLoadWasReload = false;
    m_skinScheduledToLoad = NULL;
    m_bFontReloadScheduled = false;
    m_bFireResolutionChangedScheduled = false;
    m_bFireDelayedFontReloadAndResolutionChangeToFixDesyncedUIScaleScheduled = false;

    // debug
    m_windowManager = new CWindowManager();

    // renderer
    g_vInternalResolution = engine->getScreenSize();

    m_backBuffer = engine->getResourceManager()->createRenderTarget(0, 0, getScreenWidth(), getScreenHeight());
    m_playfieldBuffer = engine->getResourceManager()->createRenderTarget(0, 0, 64, 64);
    m_sliderFrameBuffer = engine->getResourceManager()->createRenderTarget(0, 0, getScreenWidth(), getScreenHeight());
    m_frameBuffer = engine->getResourceManager()->createRenderTarget(0, 0, 64, 64);
    m_frameBuffer2 = engine->getResourceManager()->createRenderTarget(0, 0, 64, 64);

    // load a few select subsystems very early
    m_notificationOverlay = new NotificationOverlay();
    m_score = new LiveScore(false);
    m_updateHandler = new UpdateHandler();

    // exec the main config file (this must be right here!)
    Console::execConfigFile("underride");  // same as override, but for defaults
    Console::execConfigFile("osu");
    Console::execConfigFile("override");  // used for quickfixing live builds without redeploying/recompiling

    // clear screen in case cfg switched to fullscreen mode
    // (loading the rest of the app can take a bit of time)
    engine->onPaint();

    std::ifstream osuCfgFile(MCENGINE_DATA_DIR "cfg/osu.cfg");
    if(!osuCfgFile.good()) {
        import_settings_from_osu_stable();
    }

    // Initialize sound here so we can load the preferred device from config
    // Avoids initializing the sound device twice, which can take a while depending on the driver
    auto sound_engine = engine->getSound();
    sound_engine->updateOutputDevices(true);
    sound_engine->initializeOutputDevice(sound_engine->getWantedDevice());
    cv_snd_output_device.setValue(sound_engine->getOutputDeviceName());
    cv_snd_freq.setCallback(fastdelegate::MakeDelegate(sound_engine, &SoundEngine::onFreqChanged));
    cv_snd_restart.setCallback(fastdelegate::MakeDelegate(sound_engine, &SoundEngine::restart));
    cv_win_snd_wasapi_exclusive.setCallback(_RESTART_SOUND_ENGINE_ON_CHANGE);
    cv_win_snd_wasapi_buffer_size.setCallback(_RESTART_SOUND_ENGINE_ON_CHANGE);
    cv_win_snd_wasapi_period_size.setCallback(_RESTART_SOUND_ENGINE_ON_CHANGE);
    cv_asio_buffer_size.setCallback(_RESTART_SOUND_ENGINE_ON_CHANGE);

    // Initialize skin after sound engine has started, or else sounds won't load properly
    cv_skin.setCallback(fastdelegate::MakeDelegate(this, &Osu::onSkinChange));
    cv_skin_reload.setCallback(fastdelegate::MakeDelegate(this, &Osu::onSkinReload));
    onSkinChange("", cv_skin.getString());

    // Convar callbacks that should be set after loading the config
    cv_mod_mafham.setCallback(fastdelegate::MakeDelegate(this, &Osu::onModMafhamChange));
    cv_mod_fposu.setCallback(fastdelegate::MakeDelegate(this, &Osu::onModFPoSuChange));
    cv_playfield_mirror_horizontal.setCallback(fastdelegate::MakeDelegate(this, &Osu::updateModsForConVarTemplate));
    cv_playfield_mirror_vertical.setCallback(fastdelegate::MakeDelegate(this, &Osu::updateModsForConVarTemplate));
    cv_playfield_rotation.setCallback(fastdelegate::MakeDelegate(this, &Osu::onPlayfieldChange));
    cv_speed_override.setCallback(fastdelegate::MakeDelegate(this, &Osu::onSpeedChange));
    cv_mod_doubletime_dummy.setCallback(fastdelegate::MakeDelegate(this, &Osu::onDTPresetChange));
    cv_mod_halftime_dummy.setCallback(fastdelegate::MakeDelegate(this, &Osu::onHTPresetChange));

    // load global resources
    const int baseDPI = 96;
    const int newDPI = Osu::getUIScale() * baseDPI;

    McFont *defaultFont =
        engine->getResourceManager()->loadFont("weblysleekuisb.ttf", "FONT_DEFAULT", 15, true, newDPI);
    m_titleFont =
        engine->getResourceManager()->loadFont("SourceSansPro-Semibold.otf", "FONT_OSU_TITLE", 60, true, newDPI);
    m_subTitleFont =
        engine->getResourceManager()->loadFont("SourceSansPro-Semibold.otf", "FONT_OSU_SUBTITLE", 21, true, newDPI);
    m_songBrowserFont =
        engine->getResourceManager()->loadFont("SourceSansPro-Regular.otf", "FONT_OSU_SONGBROWSER", 35, true, newDPI);
    m_songBrowserFontBold =
        engine->getResourceManager()->loadFont("SourceSansPro-Bold.otf", "FONT_OSU_SONGBROWSER_BOLD", 30, true, newDPI);
    m_fontIcons = engine->getResourceManager()->loadFont("fontawesome-webfont.ttf", "FONT_OSU_ICONS", Icons::icons, 26,
                                                         true, newDPI);
    m_fonts.push_back(defaultFont);
    m_fonts.push_back(m_titleFont);
    m_fonts.push_back(m_subTitleFont);
    m_fonts.push_back(m_songBrowserFont);
    m_fonts.push_back(m_songBrowserFontBold);
    m_fonts.push_back(m_fontIcons);

    float averageIconHeight = 0.0f;
    for(int i = 0; i < Icons::icons.size(); i++) {
        UString iconString;
        iconString.insert(0, Icons::icons[i]);
        const float height = m_fontIcons->getStringHeight(iconString);
        if(height > averageIconHeight) averageIconHeight = height;
    }
    m_fontIcons->setHeight(averageIconHeight);

    if(defaultFont->getDPI() != newDPI) {
        m_bFontReloadScheduled = true;
        m_bFireResolutionChangedScheduled = true;
    }

    // load skin
    {
        UString skinFolder = cv_osu_folder.getString();
        skinFolder.append("/");
        skinFolder.append(cv_osu_folder_sub_skins.getString());
        skinFolder.append(cv_skin.getString());
        skinFolder.append("/");
        if(m_skin == NULL)  // the skin may already be loaded by Console::execConfigFile() above
            onSkinChange("", cv_skin.getString());

        // enable async skin loading for user-action skin changes (but not during startup)
        cv_skin_async.setValue(1.0f);
    }

    // load subsystems, add them to the screens array
    m_songBrowser2 = new SongBrowser();
    m_volumeOverlay = new VolumeOverlay();
    m_tooltipOverlay = new TooltipOverlay();
    m_optionsMenu = new OptionsMenu();
    m_mainMenu = new MainMenu();  // has to be after options menu
    m_backgroundImageHandler = new BackgroundImageHandler();
    m_modSelector = new ModSelector();
    m_rankingScreen = new RankingScreen();
    m_pauseMenu = new PauseMenu();
    m_hud = new HUD();
    m_changelog = new Changelog();
    m_fposu = new ModFPoSu();
    m_chat = new Chat();
    m_lobby = new Lobby();
    m_room = new RoomScreen();
    m_prompt = new PromptScreen();
    m_user_actions = new UIUserContextMenuScreen();
    m_spectatorScreen = new SpectatorScreen();

    // the order in this vector will define in which order events are handled/consumed
    m_screens.push_back(m_volumeOverlay);
    m_screens.push_back(m_prompt);
    m_screens.push_back(m_modSelector);
    m_screens.push_back(m_user_actions);
    m_screens.push_back(m_room);
    m_screens.push_back(m_chat);
    m_screens.push_back(m_notificationOverlay);
    m_screens.push_back(m_optionsMenu);
    m_screens.push_back(m_rankingScreen);
    m_screens.push_back(m_spectatorScreen);
    m_screens.push_back(m_pauseMenu);
    m_screens.push_back(m_hud);
    m_screens.push_back(m_songBrowser2);
    m_screens.push_back(m_lobby);
    m_screens.push_back(m_changelog);
    m_screens.push_back(m_mainMenu);
    m_screens.push_back(m_tooltipOverlay);
    m_mainMenu->setVisible(true);

    // update mod settings
    updateMods();

    // Init online functionality (multiplayer/leaderboards/etc)
    init_networking_thread();
    if(cv_mp_autologin.getBool()) {
        reconnect();
    }

    m_updateHandler->checkForUpdates();

#ifdef _WIN32
    // Process cmdline args now, after everything has been initialized
    auto cmd_args = engine->getArgs();
    handle_cmdline_args(cmd_args.toUtf8());
#endif

    // Not the type of shader you want players to tweak or delete, so loading from string
    actual_flashlight_shader = engine->getGraphics()->createShaderFromSource(
        "#version 110\n"
        "varying vec2 tex_coord;\n"
        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.x, gl_Vertex.y, 0.0, 1.0);\n"
        "    gl_FrontColor = gl_Color;\n"
        "    tex_coord = gl_MultiTexCoord0.xy;\n"
        "}",
        "#version 110\n"
        "uniform float max_opacity;\n"
        "uniform float flashlight_radius;\n"
        "uniform vec2 flashlight_center;\n"
        "void main(void) {\n"
        "    float dist = distance(flashlight_center, gl_FragCoord.xy);\n"
        "    float opacity = smoothstep(flashlight_radius, flashlight_radius * 1.4, dist);\n"
        "    opacity = 1.0 - min(opacity, max_opacity);\n"
        "    gl_FragColor = vec4(1.0, 1.0, 0.9, opacity);\n"
        "}");
    flashlight_shader = engine->getGraphics()->createShaderFromSource(
        "#version 110\n"
        "varying vec2 tex_coord;\n"
        "void main() {\n"
        "    gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.x, gl_Vertex.y, 0.0, 1.0);\n"
        "    gl_FrontColor = gl_Color;\n"
        "    tex_coord = gl_MultiTexCoord0.xy;\n"
        "}",
        "#version 110\n"
        "uniform float max_opacity;\n"
        "uniform float flashlight_radius;\n"
        "uniform vec2 flashlight_center;\n"
        "void main(void) {\n"
        "    float dist = distance(flashlight_center, gl_FragCoord.xy);\n"
        "    float opacity = 1.0 - smoothstep(flashlight_radius, flashlight_radius * 1.4, dist);\n"
        "    opacity = 1.0 - min(opacity, max_opacity);\n"
        "    gl_FragColor = vec4(0.0, 0.0, 0.0, opacity);\n"
        "}");
    engine->getResourceManager()->loadResource(actual_flashlight_shader);
    engine->getResourceManager()->loadResource(flashlight_shader);
}

Osu::~Osu() {
    sct_abort();
    lct_set_map(NULL);
    loct_abort();
    mct_abort();
    kill_networking_thread();

    // "leak" UpdateHandler object, but not relevant since shutdown:
    // this is the only way of handling instant user shutdown requests properly, there is no solution for active working
    // threads besides letting the OS kill them when the main threads exits. we must not delete the update handler
    // object, because the thread is potentially still accessing members during shutdown
    m_updateHandler->stop();  // tell it to stop at the next cancellation point, depending on the OS/runtime and engine
                              // shutdown time it may get killed before that

    SAFE_DELETE(m_windowManager);

    for(int i = 0; i < m_screens.size(); i++) {
        SAFE_DELETE(m_screens[i]);
    }

    SAFE_DELETE(m_fposu);
    SAFE_DELETE(m_score);
    SAFE_DELETE(m_skin);
    SAFE_DELETE(m_backgroundImageHandler);

    osu = NULL;
}

void Osu::draw(Graphics *g) {
    if(m_skin == NULL || flashlight_shader == NULL)  // sanity check
    {
        g->setColor(0xff000000);
        g->fillRect(0, 0, getScreenWidth(), getScreenHeight());
        return;
    }

    // if we are not using the native window resolution, draw into the buffer
    const bool isBufferedDraw = cv_resolution_enabled.getBool();

    if(isBufferedDraw) m_backBuffer->enable();

    f32 fadingCursorAlpha = 1.f;

    // draw everything in the correct order
    if(isInPlayMode()) {  // if we are playing a beatmap
        Beatmap *beatmap = getSelectedBeatmap();
        const bool isFPoSu = (cv_mod_fposu.getBool());

        if(isFPoSu) m_playfieldBuffer->enable();

        getSelectedBeatmap()->draw(g);

        auto actual_flashlight_enabled = cv_mod_actual_flashlight.getBool();
        if(cv_mod_flashlight.getBool() || actual_flashlight_enabled) {
            // Convert screen mouse -> osu mouse pos
            Vector2 cursorPos = beatmap->getCursorPos();
            Vector2 mouse_position = cursorPos - GameRules::getPlayfieldOffset();
            mouse_position /= GameRules::getPlayfieldScaleFactor();

            // Update flashlight position
            double follow_delay = cv_flashlight_follow_delay.getFloat();
            double frame_time = min(engine->getFrameTime(), follow_delay);
            double t = frame_time / follow_delay;
            t = t * (2.f - t);
            flashlight_position += t * (mouse_position - flashlight_position);
            Vector2 flashlightPos =
                flashlight_position * GameRules::getPlayfieldScaleFactor() + GameRules::getPlayfieldOffset();

            float base_fl_radius = cv_flashlight_radius.getFloat() * GameRules::getPlayfieldScaleFactor();
            float anti_fl_radius = base_fl_radius * 0.625f;
            float fl_radius = base_fl_radius;
            if(getScore()->getCombo() >= 200 || cv_flashlight_always_hard.getBool()) {
                anti_fl_radius = base_fl_radius;
                fl_radius *= 0.625f;
            } else if(getScore()->getCombo() >= 100) {
                anti_fl_radius = base_fl_radius * 0.8125f;
                fl_radius *= 0.8125f;
            }

            if(cv_mod_flashlight.getBool()) {
                // Dim screen when holding a slider
                float opacity = 1.f;
                if(getSelectedBeatmap()->holding_slider && !cv_avoid_flashes.getBool()) {
                    opacity = 0.2f;
                }

                flashlight_shader->enable();
                flashlight_shader->setUniform1f("max_opacity", opacity);
                flashlight_shader->setUniform1f("flashlight_radius", fl_radius);
                flashlight_shader->setUniform2f("flashlight_center", flashlightPos.x,
                                                getScreenSize().y - flashlightPos.y);

                g->setColor(COLOR(255, 0, 0, 0));
                g->fillRect(0, 0, getScreenWidth(), getScreenHeight());

                flashlight_shader->disable();
            }
            if(actual_flashlight_enabled) {
                // Brighten screen when holding a slider
                float opacity = 1.f;
                if(getSelectedBeatmap()->holding_slider && !cv_avoid_flashes.getBool()) {
                    opacity = 0.8f;
                }

                actual_flashlight_shader->enable();
                actual_flashlight_shader->setUniform1f("max_opacity", opacity);
                actual_flashlight_shader->setUniform1f("flashlight_radius", anti_fl_radius);
                actual_flashlight_shader->setUniform2f("flashlight_center", flashlightPos.x,
                                                       getScreenSize().y - flashlightPos.y);

                g->setColor(COLOR(255, 0, 0, 0));
                g->fillRect(0, 0, getScreenWidth(), getScreenHeight());

                actual_flashlight_shader->disable();
            }
        }

        if(!isFPoSu) m_hud->draw(g);

        // quick retry fadeout overlay
        if(m_fQuickRetryTime != 0.0f && m_bQuickRetryDown) {
            float alphaPercent = 1.0f - (m_fQuickRetryTime - engine->getTime()) / cv_quick_retry_delay.getFloat();
            if(engine->getTime() > m_fQuickRetryTime) alphaPercent = 1.0f;

            g->setColor(COLOR((int)(255 * alphaPercent), 0, 0, 0));
            g->fillRect(0, 0, getScreenWidth(), getScreenHeight());
        }

        m_pauseMenu->draw(g);
        m_modSelector->draw(g);
        m_chat->draw(g);
        m_optionsMenu->draw(g);

        if(cv_draw_fps.getBool() && (!isFPoSu)) m_hud->drawFps(g);

        m_windowManager->draw(g);

        if(isFPoSu && cv_draw_cursor_ripples.getBool()) m_hud->drawCursorRipples(g);

        // draw FPoSu cursor trail
        fadingCursorAlpha =
            1.0f - clamp<float>((float)m_score->getCombo() / cv_mod_fadingcursor_combo.getFloat(), 0.0f, 1.0f);
        if(m_pauseMenu->isVisible() || getSelectedBeatmap()->isContinueScheduled() || !cv_mod_fadingcursor.getBool())
            fadingCursorAlpha = 1.0f;
        if(isFPoSu && cv_fposu_draw_cursor_trail.getBool())
            m_hud->drawCursorTrail(g, beatmap->getCursorPos(), fadingCursorAlpha);

        if(isFPoSu) {
            m_playfieldBuffer->disable();
            m_fposu->draw(g);
            m_hud->draw(g);

            if(cv_draw_fps.getBool()) m_hud->drawFps(g);
        }
    } else {  // if we are not playing
        m_spectatorScreen->draw(g);

        m_lobby->draw(g);
        m_room->draw(g);

        if(m_songBrowser2 != NULL) m_songBrowser2->draw(g);

        m_mainMenu->draw(g);
        m_changelog->draw(g);
        m_rankingScreen->draw(g);
        m_chat->draw(g);
        m_user_actions->draw(g);
        m_optionsMenu->draw(g);
        m_modSelector->draw(g);
        m_prompt->draw(g);

        if(cv_draw_fps.getBool()) m_hud->drawFps(g);

        m_windowManager->draw(g);
    }

    m_tooltipOverlay->draw(g);
    m_notificationOverlay->draw(g);
    m_volumeOverlay->draw(g);

    // loading spinner for some async tasks
    if((m_bSkinLoadScheduled && m_skin != m_skinScheduledToLoad)) {
        m_hud->drawLoadingSmall(g, "");
    }

    // draw cursor
    if(isInPlayMode()) {
        Beatmap *beatmap = getSelectedBeatmap();
        Vector2 cursorPos = beatmap->getCursorPos();
        bool drawSecondTrail =
            (cv_mod_autoplay.getBool() || cv_mod_autopilot.getBool() || beatmap->is_watching || beatmap->is_spectating);
        bool updateAndDrawTrail = true;
        if(cv_mod_fposu.getBool()) {
            cursorPos = getScreenSize() / 2.0f;
            updateAndDrawTrail = false;
        }
        m_hud->drawCursor(g, cursorPos, fadingCursorAlpha, drawSecondTrail, updateAndDrawTrail);
    } else {
        m_hud->drawCursor(g, engine->getMouse()->getPos());
    }

    // if we are not using the native window resolution
    if(isBufferedDraw) {
        // draw a scaled version from the buffer to the screen
        m_backBuffer->disable();

        Vector2 offset = Vector2(engine->getGraphics()->getResolution().x / 2 - g_vInternalResolution.x / 2,
                                 engine->getGraphics()->getResolution().y / 2 - g_vInternalResolution.y / 2);
        g->setBlending(false);
        if(cv_letterboxing.getBool()) {
            m_backBuffer->draw(g, offset.x * (1.0f + cv_letterboxing_offset_x.getFloat()),
                               offset.y * (1.0f + cv_letterboxing_offset_y.getFloat()), g_vInternalResolution.x,
                               g_vInternalResolution.y);
        } else {
            if(cv_resolution_keep_aspect_ratio.getBool()) {
                const float scale =
                    getImageScaleToFitResolution(m_backBuffer->getSize(), engine->getGraphics()->getResolution());
                const float scaledWidth = m_backBuffer->getWidth() * scale;
                const float scaledHeight = m_backBuffer->getHeight() * scale;
                m_backBuffer->draw(g,
                                   max(0.0f, engine->getGraphics()->getResolution().x / 2.0f - scaledWidth / 2.0f) *
                                       (1.0f + cv_letterboxing_offset_x.getFloat()),
                                   max(0.0f, engine->getGraphics()->getResolution().y / 2.0f - scaledHeight / 2.0f) *
                                       (1.0f + cv_letterboxing_offset_y.getFloat()),
                                   scaledWidth, scaledHeight);
            } else {
                m_backBuffer->draw(g, 0, 0, engine->getGraphics()->getResolution().x,
                                   engine->getGraphics()->getResolution().y);
            }
        }
        g->setBlending(true);
    }
}

void Osu::update() {
    if(m_skin != NULL) m_skin->update();

    if(isInPlayMode() && cv_mod_fposu.getBool()) m_fposu->update();

    bool propagate_clicks = true;
    for(int i = 0; i < m_screens.size() && propagate_clicks; i++) {
        m_screens[i]->mouse_update(&propagate_clicks);
    }

    if(music_unpause_scheduled && engine->getSound()->isReady()) {
        if(getSelectedBeatmap()->getMusic() != NULL) {
            engine->getSound()->play(getSelectedBeatmap()->getMusic());
        }
        music_unpause_scheduled = false;
    }

    // main beatmap update
    m_bSeeking = false;
    if(isInPlayMode()) {
        getSelectedBeatmap()->update();

        // NOTE: force keep loaded background images while playing
        m_backgroundImageHandler->scheduleFreezeCache();

        // skip button clicking
        bool can_skip = getSelectedBeatmap()->isInSkippableSection() && !m_bClickedSkipButton;
        can_skip &= !getSelectedBeatmap()->isPaused() && !m_volumeOverlay->isBusy();
        if(can_skip) {
            const bool isAnyOsuKeyDown = (m_bKeyboardKey1Down || m_bKeyboardKey12Down || m_bKeyboardKey2Down ||
                                          m_bKeyboardKey22Down || m_bMouseKey1Down || m_bMouseKey2Down);
            const bool isAnyKeyDown = (isAnyOsuKeyDown || engine->getMouse()->isLeftDown());

            if(isAnyKeyDown) {
                if(m_hud->getSkipClickRect().contains(engine->getMouse()->getPos())) {
                    if(!m_bSkipScheduled) {
                        m_bSkipScheduled = true;
                        m_bClickedSkipButton = true;

                        if(bancho.is_playing_a_multi_map()) {
                            Packet packet;
                            packet.id = MATCH_SKIP_REQUEST;
                            send_packet(packet);
                        }
                    }
                }
            }
        }

        // skipping
        if(m_bSkipScheduled) {
            const bool isLoading = getSelectedBeatmap()->isLoading();

            if(getSelectedBeatmap()->isInSkippableSection() && !getSelectedBeatmap()->isPaused() && !isLoading) {
                bool can_skip_intro =
                    (cv_skip_intro_enabled.getBool() && getSelectedBeatmap()->m_iCurrentHitObjectIndex < 1);
                bool can_skip_break =
                    (cv_skip_breaks_enabled.getBool() && getSelectedBeatmap()->m_iCurrentHitObjectIndex > 0);
                if(bancho.is_playing_a_multi_map()) {
                    can_skip_intro = bancho.room.all_players_skipped;
                    can_skip_break = false;
                }

                if(can_skip_intro || can_skip_break) {
                    getSelectedBeatmap()->skipEmptySection();
                }
            }

            if(!isLoading) m_bSkipScheduled = false;
        }

        // Reset m_bClickedSkipButton on mouse up
        // We only use m_bClickedSkipButton to prevent seeking when clicking the skip button
        if(m_bClickedSkipButton && !getSelectedBeatmap()->isInSkippableSection()) {
            if(!engine->getMouse()->isLeftDown()) {
                m_bClickedSkipButton = false;
            }
        }

        // scrubbing/seeking
        m_bSeeking = (m_bSeekKey || getSelectedBeatmap()->is_watching);
        m_bSeeking &= !getSelectedBeatmap()->isPaused() && !m_volumeOverlay->isBusy();
        m_bSeeking &= !bancho.is_playing_a_multi_map() && !m_bClickedSkipButton;
        m_bSeeking &= !getSelectedBeatmap()->is_spectating;
        if(m_bSeeking) {
            const float mousePosX = (int)engine->getMouse()->getPos().x;
            const float percent = clamp<float>(mousePosX / (float)getScreenWidth(), 0.0f, 1.0f);

            if(engine->getMouse()->isLeftDown()) {
                if(mousePosX != m_fPrevSeekMousePosX || !cv_scrubbing_smooth.getBool()) {
                    m_fPrevSeekMousePosX = mousePosX;

                    // special case: allow cancelling the failing animation here
                    if(getSelectedBeatmap()->hasFailed()) getSelectedBeatmap()->cancelFailing();

                    getSelectedBeatmap()->seekPercentPlayable(percent);
                } else {
                    // special case: keep player invulnerable even if scrubbing position does not change
                    getSelectedBeatmap()->resetScore();
                }
            } else {
                m_fPrevSeekMousePosX = -1.0f;
            }

            if(engine->getMouse()->isRightDown()) {
                m_fQuickSaveTime = clamp<float>(
                    (float)((getSelectedBeatmap()->getStartTimePlayable() + getSelectedBeatmap()->getLengthPlayable()) *
                            percent) /
                        (float)getSelectedBeatmap()->getLength(),
                    0.0f, 1.0f);
            }
        }

        // quick retry timer
        if(m_bQuickRetryDown && m_fQuickRetryTime != 0.0f && engine->getTime() > m_fQuickRetryTime) {
            m_fQuickRetryTime = 0.0f;

            if(!bancho.is_playing_a_multi_map()) {
                getSelectedBeatmap()->restart(true);
                getSelectedBeatmap()->update();
                m_pauseMenu->setVisible(false);
            }
        }
    }

    // background image cache tick
    m_backgroundImageHandler->update(
        m_songBrowser2->isVisible());  // NOTE: must be before the asynchronous ui toggles due to potential 1-frame
                                       // unloads after invisible songbrowser

    // asynchronous ui toggles
    // TODO: this is cancer, why did I even write this section
    if(m_bToggleModSelectionScheduled) {
        m_bToggleModSelectionScheduled = false;
        m_modSelector->setVisible(!m_modSelector->isVisible());

        if(bancho.is_in_a_multi_room()) {
            m_room->setVisible(!m_modSelector->isVisible());
        } else if(!isInPlayMode() && m_songBrowser2 != NULL) {
            m_songBrowser2->setVisible(!m_modSelector->isVisible());
        }
    }
    if(m_bToggleSongBrowserScheduled) {
        m_bToggleSongBrowserScheduled = false;

        if(m_mainMenu->isVisible() && m_optionsMenu->isVisible()) m_optionsMenu->setVisible(false);

        if(m_songBrowser2 != NULL) m_songBrowser2->setVisible(!m_songBrowser2->isVisible());

        if(bancho.is_in_a_multi_room()) {
            // We didn't select a map; revert to previously selected one
            auto diff2 = m_songBrowser2->m_lastSelectedBeatmap;
            if(diff2 != NULL) {
                bancho.room.map_name = UString::format("%s - %s [%s]", diff2->getArtist().c_str(),
                                                       diff2->getTitle().c_str(), diff2->getDifficultyName().c_str());
                bancho.room.map_md5 = diff2->getMD5Hash();
                bancho.room.map_id = diff2->getID();

                Packet packet;
                packet.id = MATCH_CHANGE_SETTINGS;
                bancho.room.pack(&packet);
                send_packet(packet);

                m_room->on_map_change();
            }
        } else {
            m_mainMenu->setVisible(!(m_songBrowser2 != NULL && m_songBrowser2->isVisible()));
        }

        updateConfineCursor();
    }
    if(m_bToggleOptionsMenuScheduled) {
        m_bToggleOptionsMenuScheduled = false;

        const bool wasFullscreen = m_optionsMenu->isFullscreen();
        m_optionsMenu->setFullscreen(false);
        m_optionsMenu->setVisible(!m_optionsMenu->isVisible());
        if(wasFullscreen) m_mainMenu->setVisible(!m_optionsMenu->isVisible());
    }
    if(m_bToggleChangelogScheduled) {
        m_bToggleChangelogScheduled = false;

        m_mainMenu->setVisible(!m_mainMenu->isVisible());
        m_changelog->setVisible(!m_mainMenu->isVisible());
    }
    if(m_bToggleEditorScheduled) {
        m_bToggleEditorScheduled = false;

        m_mainMenu->setVisible(!m_mainMenu->isVisible());
    }

    // handle cursor visibility if outside of internal resolution
    // TODO: not a critical bug, but the real cursor gets visible way too early if sensitivity is > 1.0f, due to this
    // using scaled/offset getMouse()->getPos()
    if(cv_resolution_enabled.getBool()) {
        McRect internalWindow = McRect(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
        bool cursorVisible = env->isCursorVisible();
        if(!internalWindow.contains(engine->getMouse()->getPos())) {
            if(!cursorVisible) env->setCursorVisible(true);
        } else {
            if(cursorVisible != m_bShouldCursorBeVisible) env->setCursorVisible(m_bShouldCursorBeVisible);
        }
    }

    // endless mod
    if(m_bScheduleEndlessModNextBeatmap) {
        m_bScheduleEndlessModNextBeatmap = false;
        m_songBrowser2->playNextRandomBeatmap();
    }

    // multiplayer update
    receive_api_responses();
    receive_bancho_packets();

    // skin async loading
    if(m_bSkinLoadScheduled) {
        if(m_skinScheduledToLoad != NULL && m_skinScheduledToLoad->isReady()) {
            m_bSkinLoadScheduled = false;

            if(m_skin != m_skinScheduledToLoad) SAFE_DELETE(m_skin);

            m_skin = m_skinScheduledToLoad;

            m_skinScheduledToLoad = NULL;

            // force layout update after all skin elements have been loaded
            fireResolutionChanged();

            // notify if done after reload
            if(m_bSkinLoadWasReload) {
                m_bSkinLoadWasReload = false;

                m_notificationOverlay->addNotification(
                    m_skin->getName().length() > 0 ? UString::format("Skin reloaded! (%s)", m_skin->getName().c_str())
                                                   : UString("Skin reloaded!"),
                    0xffffffff, false, 0.75f);
            }
        }
    }

    // (must be before m_bFontReloadScheduled and m_bFireResolutionChangedScheduled are handled!)
    if(m_bFireDelayedFontReloadAndResolutionChangeToFixDesyncedUIScaleScheduled) {
        m_bFireDelayedFontReloadAndResolutionChangeToFixDesyncedUIScaleScheduled = false;

        m_bFontReloadScheduled = true;
        m_bFireResolutionChangedScheduled = true;
    }

    // delayed font reloads (must be before layout updates!)
    if(m_bFontReloadScheduled) {
        m_bFontReloadScheduled = false;
        reloadFonts();
    }

    // delayed layout updates
    if(m_bFireResolutionChangedScheduled) {
        m_bFireResolutionChangedScheduled = false;
        fireResolutionChanged();
    }
}

void Osu::useMods(FinishedScore *score) { Replay::Mods::use(score->mods); }

void Osu::updateMods() {
    if(bancho.is_in_a_multi_room()) {
        if(bancho.room.mods & (LegacyFlags::DoubleTime | LegacyFlags::Nightcore)) {
            cv_speed_override.setValue(1.5f);
        } else if(bancho.room.mods & (LegacyFlags::HalfTime)) {
            cv_speed_override.setValue(0.75f);
        } else {
            cv_speed_override.setValue(-1.f);
        }

        cv_mod_nofail.setValue(bancho.room.mods & LegacyFlags::NoFail);
        cv_mod_easy.setValue(bancho.room.mods & LegacyFlags::Easy);
        cv_mod_touchdevice.setValue(bancho.room.mods & LegacyFlags::TouchDevice);
        cv_mod_hidden.setValue(bancho.room.mods & LegacyFlags::Hidden);
        cv_mod_hardrock.setValue(bancho.room.mods & LegacyFlags::HardRock);
        cv_mod_suddendeath.setValue(bancho.room.mods & LegacyFlags::SuddenDeath);
        cv_mod_relax.setValue(bancho.room.mods & LegacyFlags::Relax);
        cv_mod_autoplay.setValue(bancho.room.mods & LegacyFlags::Autoplay);
        cv_mod_spunout.setValue(bancho.room.mods & LegacyFlags::SpunOut);
        cv_mod_autopilot.setValue(bancho.room.mods & LegacyFlags::Autopilot);
        cv_mod_perfect.setValue(bancho.room.mods & LegacyFlags::Perfect);
        cv_mod_target.setValue(bancho.room.mods & LegacyFlags::Target);
        cv_mod_scorev2.setValue(bancho.room.win_condition == SCOREV2);
        cv_mod_flashlight.setValue(bancho.room.mods & LegacyFlags::Flashlight);
        cv_mod_nightmare.setValue(false);
        cv_mod_actual_flashlight.setValue(false);

        if(bancho.room.freemods) {
            for(int i = 0; i < 16; i++) {
                if(bancho.room.slots[i].player_id != bancho.user_id) continue;

                cv_mod_nofail.setValue(bancho.room.slots[i].mods & LegacyFlags::NoFail);
                cv_mod_easy.setValue(bancho.room.slots[i].mods & LegacyFlags::Easy);
                cv_mod_touchdevice.setValue(bancho.room.slots[i].mods & LegacyFlags::TouchDevice);
                cv_mod_hidden.setValue(bancho.room.slots[i].mods & LegacyFlags::Hidden);
                cv_mod_hardrock.setValue(bancho.room.slots[i].mods & LegacyFlags::HardRock);
                cv_mod_suddendeath.setValue(bancho.room.slots[i].mods & LegacyFlags::SuddenDeath);
                cv_mod_relax.setValue(bancho.room.slots[i].mods & LegacyFlags::Relax);
                cv_mod_autoplay.setValue(bancho.room.slots[i].mods & LegacyFlags::Autoplay);
                cv_mod_spunout.setValue(bancho.room.slots[i].mods & LegacyFlags::SpunOut);
                cv_mod_autopilot.setValue(bancho.room.slots[i].mods & LegacyFlags::Autopilot);
                cv_mod_perfect.setValue(bancho.room.slots[i].mods & LegacyFlags::Perfect);
                cv_mod_target.setValue(bancho.room.slots[i].mods & LegacyFlags::Target);
            }
        }
    }

    osu->getScore()->mods = Replay::Mods::from_cvars();
    osu->getScore()->setCheated();

    if(m_songBrowser2 != NULL) {
        // Update pp/stars display for current map
        m_songBrowser2->recalculateStarsForSelectedBeatmap(true);
    }

    if(isInPlayMode()) {
        // handle auto/pilot cursor visibility
        m_bShouldCursorBeVisible = cv_mod_autoplay.getBool() || cv_mod_autopilot.getBool() ||
                                   getSelectedBeatmap()->is_watching || getSelectedBeatmap()->is_spectating;
        env->setCursorVisible(m_bShouldCursorBeVisible);

        // notify the possibly running beatmap of mod changes
        // e.g. recalculating stacks dynamically if HR is toggled
        getSelectedBeatmap()->onModUpdate();
    }

    // handle windows key disable/enable
    updateWindowsKeyDisable();
}

void Osu::onKeyDown(KeyboardEvent &key) {
    // global hotkeys

    // global hotkey
    if(key == KEY_O && engine->getKeyboard()->isControlDown()) {
        toggleOptionsMenu();
        key.consume();
        return;
    }

    // special hotkeys
    // reload & recompile shaders
    if(engine->getKeyboard()->isAltDown() && engine->getKeyboard()->isControlDown() && key == KEY_R) {
        Shader *sliderShader = engine->getResourceManager()->getShader("slider");
        Shader *cursorTrailShader = engine->getResourceManager()->getShader("cursortrail");
        Shader *hitcircle3DShader = engine->getResourceManager()->getShader("hitcircle3D");

        if(sliderShader != NULL) sliderShader->reload();
        if(cursorTrailShader != NULL) cursorTrailShader->reload();
        if(hitcircle3DShader != NULL) hitcircle3DShader->reload();

        key.consume();
    }

    // reload skin (alt)
    if(engine->getKeyboard()->isAltDown() && engine->getKeyboard()->isControlDown() && key == KEY_S) {
        onSkinReload();
        key.consume();
    }

    if(key == (KEYCODE)cv_OPEN_SKIN_SELECT_MENU.getInt()) {
        m_optionsMenu->onSkinSelect();
        key.consume();
        return;
    }

    // disable mouse buttons hotkey
    if(key == (KEYCODE)cv_DISABLE_MOUSE_BUTTONS.getInt()) {
        if(cv_disable_mousebuttons.getBool()) {
            cv_disable_mousebuttons.setValue(0.0f);
            m_notificationOverlay->addNotification("Mouse buttons are enabled.");
        } else {
            cv_disable_mousebuttons.setValue(1.0f);
            m_notificationOverlay->addNotification("Mouse buttons are disabled.");
        }
    }

    if(key == (KEYCODE)cv_TOGGLE_MAP_BACKGROUND.getInt()) {
        auto diff = getSelectedBeatmap()->getSelectedDifficulty2();
        if(!diff) {
            m_notificationOverlay->addNotification("No beatmap is currently selected.");
        } else {
            diff->draw_background = !diff->draw_background;
            diff->update_overrides();
        }
        key.consume();
        return;
    }

    // F8 toggle chat
    if(key == (KEYCODE)cv_TOGGLE_CHAT.getInt()) {
        // When options menu is open, instead of toggling chat, close options menu and open chat
        if(bancho.is_online() && m_optionsMenu->isVisible()) {
            m_optionsMenu->setVisible(false);
            m_chat->user_wants_chat = true;
            m_chat->updateVisibility();
        } else {
            m_chat->user_wants_chat = !m_chat->user_wants_chat;
            m_chat->updateVisibility();
        }
    }

    // screenshots
    if(key == (KEYCODE)cv_SAVE_SCREENSHOT.getInt()) saveScreenshot();

    // boss key (minimize + mute)
    if(key == (KEYCODE)cv_BOSS_KEY.getInt()) {
        engine->getEnvironment()->minimize();
        m_bWasBossKeyPaused = getSelectedBeatmap()->isPreviewMusicPlaying();
        getSelectedBeatmap()->pausePreviewMusic(false);
    }

    // local hotkeys (and gameplay keys)

    // while playing (and not in options)
    if(isInPlayMode() && !m_optionsMenu->isVisible() && !m_chat->isVisible()) {
        auto beatmap = getSelectedBeatmap();

        // instant replay
        if((beatmap->isPaused() || beatmap->hasFailed())) {
            if(!key.isConsumed() && key == (KEYCODE)cv_INSTANT_REPLAY.getInt()) {
                if(!beatmap->is_watching && !beatmap->is_spectating) {
                    FinishedScore score;
                    score.replay = beatmap->live_replay;
                    score.beatmap_hash = beatmap->getSelectedDifficulty2()->getMD5Hash();
                    score.mods = getScore()->mods;

                    if(bancho.is_online()) {
                        score.player_id = bancho.user_id;
                        score.playerName = bancho.username.toUtf8();
                    } else {
                        auto local_name = cv_name.getString();
                        score.player_id = 0;
                        score.playerName = local_name.toUtf8();
                    }

                    double percentFinished = beatmap->getPercentFinished();
                    double duration = cv_instant_replay_duration.getFloat() * 1000.f;
                    double offsetPercent = duration / beatmap->getLength();
                    double seekPoint = fmax(0.f, percentFinished - offsetPercent);
                    beatmap->cancelFailing();
                    beatmap->watch(score, seekPoint);
                    return;
                }
            }
        }

        // while playing and not paused
        if(!getSelectedBeatmap()->isPaused()) {
            getSelectedBeatmap()->onKeyDown(key);

            // K1
            {
                const bool isKeyLeftClick = (key == (KEYCODE)cv_LEFT_CLICK.getInt());
                const bool isKeyLeftClick2 = (key == (KEYCODE)cv_LEFT_CLICK_2.getInt());
                if((!m_bKeyboardKey1Down && isKeyLeftClick) || (!m_bKeyboardKey12Down && isKeyLeftClick2)) {
                    if(isKeyLeftClick2)
                        m_bKeyboardKey12Down = true;
                    else
                        m_bKeyboardKey1Down = true;

                    onKey1Change(true, false);

                    if(!getSelectedBeatmap()->hasFailed()) key.consume();
                } else if(isKeyLeftClick || isKeyLeftClick2) {
                    if(!getSelectedBeatmap()->hasFailed()) key.consume();
                }
            }

            // K2
            {
                const bool isKeyRightClick = (key == (KEYCODE)cv_RIGHT_CLICK.getInt());
                const bool isKeyRightClick2 = (key == (KEYCODE)cv_RIGHT_CLICK_2.getInt());
                if((!m_bKeyboardKey2Down && isKeyRightClick) || (!m_bKeyboardKey22Down && isKeyRightClick2)) {
                    if(isKeyRightClick2)
                        m_bKeyboardKey22Down = true;
                    else
                        m_bKeyboardKey2Down = true;

                    onKey2Change(true, false);

                    if(!getSelectedBeatmap()->hasFailed()) key.consume();
                } else if(isKeyRightClick || isKeyRightClick2) {
                    if(!getSelectedBeatmap()->hasFailed()) key.consume();
                }
            }

            // handle skipping
            if(key == KEY_ENTER || key == (KEYCODE)cv_SKIP_CUTSCENE.getInt()) m_bSkipScheduled = true;

            // toggle ui
            if(!key.isConsumed() && key == (KEYCODE)cv_TOGGLE_SCOREBOARD.getInt() && !m_bScoreboardToggleCheck) {
                m_bScoreboardToggleCheck = true;

                if(engine->getKeyboard()->isShiftDown()) {
                    if(!m_bUIToggleCheck) {
                        m_bUIToggleCheck = true;
                        cv_draw_hud.setValue(!cv_draw_hud.getBool());
                        m_notificationOverlay->addNotification(cv_draw_hud.getBool()
                                                                   ? "In-game interface has been enabled."
                                                                   : "In-game interface has been disabled.",
                                                               0xffffffff, false, 0.1f);

                        key.consume();
                    }
                } else {
                    if(bancho.is_playing_a_multi_map()) {
                        cv_draw_scoreboard_mp.setValue(!cv_draw_scoreboard_mp.getBool());
                        m_notificationOverlay->addNotification(
                            cv_draw_scoreboard_mp.getBool() ? "Scoreboard is shown." : "Scoreboard is hidden.",
                            0xffffffff, false, 0.1f);
                    } else {
                        cv_draw_scoreboard.setValue(!cv_draw_scoreboard.getBool());
                        m_notificationOverlay->addNotification(
                            cv_draw_scoreboard.getBool() ? "Scoreboard is shown." : "Scoreboard is hidden.", 0xffffffff,
                            false, 0.1f);
                    }

                    key.consume();
                }
            }

            // allow live mod changing while playing
            if(!key.isConsumed() && (key == KEY_F1 || key == (KEYCODE)cv_TOGGLE_MODSELECT.getInt()) &&
               ((KEY_F1 != (KEYCODE)cv_LEFT_CLICK.getInt() && KEY_F1 != (KEYCODE)cv_LEFT_CLICK_2.getInt()) ||
                (!m_bKeyboardKey1Down && !m_bKeyboardKey12Down)) &&
               ((KEY_F1 != (KEYCODE)cv_RIGHT_CLICK.getInt() && KEY_F1 != (KEYCODE)cv_RIGHT_CLICK_2.getInt()) ||
                (!m_bKeyboardKey2Down && !m_bKeyboardKey22Down)) &&
               !m_bF1 && !getSelectedBeatmap()->hasFailed() &&
               !bancho.is_playing_a_multi_map())  // only if not failed though
            {
                m_bF1 = true;
                toggleModSelection(true);
            }

            // quick save/load
            if(!bancho.is_playing_a_multi_map()) {
                if(key == (KEYCODE)cv_QUICK_SAVE.getInt())
                    m_fQuickSaveTime = getSelectedBeatmap()->getPercentFinished();

                if(key == (KEYCODE)cv_QUICK_LOAD.getInt()) {
                    // special case: allow cancelling the failing animation here
                    if(getSelectedBeatmap()->hasFailed()) getSelectedBeatmap()->cancelFailing();

                    getSelectedBeatmap()->seekPercent(m_fQuickSaveTime);
                }
            }

            // quick seek
            if(!bancho.is_playing_a_multi_map()) {
                const bool backward = (key == (KEYCODE)cv_SEEK_TIME_BACKWARD.getInt());
                const bool forward = (key == (KEYCODE)cv_SEEK_TIME_FORWARD.getInt());

                if(backward || forward) {
                    const unsigned long lengthMS = getSelectedBeatmap()->getLength();
                    const float percentFinished = getSelectedBeatmap()->getPercentFinished();

                    if(lengthMS > 0) {
                        double seekedPercent = 0.0;
                        if(backward)
                            seekedPercent -= (double)cv_seek_delta.getInt() * (1.0 / (double)lengthMS) * 1000.0;
                        else if(forward)
                            seekedPercent += (double)cv_seek_delta.getInt() * (1.0 / (double)lengthMS) * 1000.0;

                        if(seekedPercent != 0.0f) {
                            // special case: allow cancelling the failing animation here
                            if(getSelectedBeatmap()->hasFailed()) getSelectedBeatmap()->cancelFailing();

                            getSelectedBeatmap()->seekPercent(percentFinished + seekedPercent);
                        }
                    }
                }
            }
        }

        // while paused or maybe not paused

        // handle quick restart
        if(((key == (KEYCODE)cv_QUICK_RETRY.getInt() ||
             (engine->getKeyboard()->isControlDown() && !engine->getKeyboard()->isAltDown() && key == KEY_R)) &&
            !m_bQuickRetryDown)) {
            m_bQuickRetryDown = true;
            m_fQuickRetryTime = engine->getTime() + cv_quick_retry_delay.getFloat();
        }

        // handle seeking
        if(key == (KEYCODE)cv_SEEK_TIME.getInt()) m_bSeekKey = true;

        // handle fposu key handling
        m_fposu->onKeyDown(key);
    }

    // forward to all subsystem, if not already consumed
    for(int i = 0; i < m_screens.size(); i++) {
        if(key.isConsumed()) break;

        m_screens[i]->onKeyDown(key);
    }

    // special handling, after subsystems, if still not consumed
    if(!key.isConsumed()) {
        // if playing
        if(isInPlayMode()) {
            // toggle pause menu
            if((key == (KEYCODE)cv_GAME_PAUSE.getInt() || key == KEY_ESCAPE) && !m_bEscape) {
                if(!bancho.is_playing_a_multi_map() || m_iMultiplayerClientNumEscPresses > 1) {
                    if(m_iMultiplayerClientNumEscPresses > 1) {
                        getSelectedBeatmap()->stop(true);
                        m_room->ragequit();
                    } else {
                        // you can open the pause menu while the failing animation is
                        // happening, but never close it then
                        if(!getSelectedBeatmap()->hasFailed() || !m_pauseMenu->isVisible()) {
                            m_bEscape = true;
                            getSelectedBeatmap()->pause();
                            m_pauseMenu->setVisible(getSelectedBeatmap()->isPaused());

                            key.consume();
                        } else  // pressing escape while in failed pause menu
                        {
                            getSelectedBeatmap()->stop(true);
                        }
                    }
                } else {
                    m_iMultiplayerClientNumEscPresses++;
                    if(m_iMultiplayerClientNumEscPresses == 2)
                        m_notificationOverlay->addNotification("Hit 'Escape' once more to exit this multiplayer match.",
                                                               0xffffffff, false, 0.75f);
                }
            }

            // local offset
            if(key == (KEYCODE)cv_INCREASE_LOCAL_OFFSET.getInt()) {
                long offsetAdd = engine->getKeyboard()->isAltDown() ? 1 : 5;
                getSelectedBeatmap()->getSelectedDifficulty2()->setLocalOffset(
                    getSelectedBeatmap()->getSelectedDifficulty2()->getLocalOffset() + offsetAdd);
                m_notificationOverlay->addNotification(
                    UString::format("Local beatmap offset set to %ld ms",
                                    getSelectedBeatmap()->getSelectedDifficulty2()->getLocalOffset()));
            }
            if(key == (KEYCODE)cv_DECREASE_LOCAL_OFFSET.getInt()) {
                long offsetAdd = -(engine->getKeyboard()->isAltDown() ? 1 : 5);
                getSelectedBeatmap()->getSelectedDifficulty2()->setLocalOffset(
                    getSelectedBeatmap()->getSelectedDifficulty2()->getLocalOffset() + offsetAdd);
                m_notificationOverlay->addNotification(
                    UString::format("Local beatmap offset set to %ld ms",
                                    getSelectedBeatmap()->getSelectedDifficulty2()->getLocalOffset()));
            }
        }
    }
}

void Osu::onKeyUp(KeyboardEvent &key) {
    // clicks
    {
        // K1
        {
            const bool isKeyLeftClick = (key == (KEYCODE)cv_LEFT_CLICK.getInt());
            const bool isKeyLeftClick2 = (key == (KEYCODE)cv_LEFT_CLICK_2.getInt());
            if((isKeyLeftClick && m_bKeyboardKey1Down) || (isKeyLeftClick2 && m_bKeyboardKey12Down)) {
                if(isKeyLeftClick2)
                    m_bKeyboardKey12Down = false;
                else
                    m_bKeyboardKey1Down = false;

                if(isInPlayMode()) onKey1Change(false, false);
            }
        }

        // K2
        {
            const bool isKeyRightClick = (key == (KEYCODE)cv_RIGHT_CLICK.getInt());
            const bool isKeyRightClick2 = (key == (KEYCODE)cv_RIGHT_CLICK_2.getInt());
            if((isKeyRightClick && m_bKeyboardKey2Down) || (isKeyRightClick2 && m_bKeyboardKey22Down)) {
                if(isKeyRightClick2)
                    m_bKeyboardKey22Down = false;
                else
                    m_bKeyboardKey2Down = false;

                if(isInPlayMode()) onKey2Change(false, false);
            }
        }
    }

    // forward to all subsystems, if not consumed
    for(int i = 0; i < m_screens.size(); i++) {
        if(key.isConsumed()) break;

        m_screens[i]->onKeyUp(key);
    }

    // misc hotkeys release
    if(key == KEY_F1 || key == (KEYCODE)cv_TOGGLE_MODSELECT.getInt()) m_bF1 = false;
    if(key == (KEYCODE)cv_GAME_PAUSE.getInt() || key == KEY_ESCAPE) m_bEscape = false;
    if(key == KEY_SHIFT) m_bUIToggleCheck = false;
    if(key == (KEYCODE)cv_TOGGLE_SCOREBOARD.getInt()) {
        m_bScoreboardToggleCheck = false;
        m_bUIToggleCheck = false;
    }
    if(key == (KEYCODE)cv_QUICK_RETRY.getInt() || key == KEY_R) m_bQuickRetryDown = false;
    if(key == (KEYCODE)cv_SEEK_TIME.getInt()) m_bSeekKey = false;

    // handle fposu key handling
    m_fposu->onKeyUp(key);
}

void Osu::stealFocus() {
    for(auto screen : m_screens) {
        screen->stealFocus();
    }
}

void Osu::onChar(KeyboardEvent &e) {
    for(int i = 0; i < m_screens.size(); i++) {
        if(e.isConsumed()) break;

        m_screens[i]->onChar(e);
    }
}

void Osu::onLeftChange(bool down) {
    if(isInPlayMode() && !getSelectedBeatmap()->isPaused() && cv_disable_mousebuttons.getBool()) return;

    if(!m_bMouseKey1Down && down) {
        m_bMouseKey1Down = true;
        onKey1Change(true, true);
    } else if(m_bMouseKey1Down) {
        m_bMouseKey1Down = false;
        onKey1Change(false, true);
    }
}

void Osu::onRightChange(bool down) {
    if(isInPlayMode() && !getSelectedBeatmap()->isPaused() && cv_disable_mousebuttons.getBool()) return;

    if(!m_bMouseKey2Down && down) {
        m_bMouseKey2Down = true;
        onKey2Change(true, true);
    } else if(m_bMouseKey2Down) {
        m_bMouseKey2Down = false;
        onKey2Change(false, true);
    }
}

void Osu::toggleModSelection(bool waitForF1KeyUp) {
    m_bToggleModSelectionScheduled = true;
    m_modSelector->setWaitForF1KeyUp(waitForF1KeyUp);
}

void Osu::toggleSongBrowser() { m_bToggleSongBrowserScheduled = true; }

void Osu::toggleOptionsMenu() {
    m_bToggleOptionsMenuScheduled = true;
    m_bOptionsMenuFullscreen = m_mainMenu->isVisible();
}

void Osu::toggleChangelog() { m_bToggleChangelogScheduled = true; }

void Osu::toggleEditor() { m_bToggleEditorScheduled = true; }

void Osu::saveScreenshot() {
    engine->getSound()->play(m_skin->getShutter());
    int screenshotNumber = 0;
    UString screenshot_path;
    do {
        screenshot_path = UString::format("screenshots/screenshot%d.png", screenshotNumber);
        screenshotNumber++;
    } while(env->fileExists(screenshot_path.toUtf8()));

    std::vector<unsigned char> pixels = engine->getGraphics()->getScreenshot();
    Image::saveToImage(&pixels[0], engine->getGraphics()->getResolution().x, engine->getGraphics()->getResolution().y,
                       screenshot_path.toUtf8());
}

void Osu::onPlayEnd(FinishedScore score, bool quit, bool aborted) {
    cv_snd_change_check_interval.setValue(cv_snd_change_check_interval.getDefaultFloat());

    if(!quit) {
        if(!cv_mod_endless.getBool()) {
            // NOTE: the order of these two calls matters
            m_rankingScreen->setScore(score);
            m_rankingScreen->setBeatmapInfo(getSelectedBeatmap(), getSelectedBeatmap()->getSelectedDifficulty2());

            engine->getSound()->play(m_skin->getApplause());
        } else {
            m_bScheduleEndlessModNextBeatmap = true;
            return;  // nothing more to do here
        }
    }

    m_mainMenu->setVisible(false);
    m_modSelector->setVisible(false);
    m_pauseMenu->setVisible(false);

    env->setCursorVisible(false);
    m_bShouldCursorBeVisible = false;

    m_iMultiplayerClientNumEscPresses = 0;

    if(m_songBrowser2 != NULL) m_songBrowser2->onPlayEnd(quit);

    // When playing in multiplayer, screens are toggled in Room
    if(!bancho.is_playing_a_multi_map()) {
        if(quit) {
            // When spectating, we don't want to enable the song browser after song ends
            if(bancho.spectated_player_id == 0) {
                toggleSongBrowser();
            }
        } else {
            m_rankingScreen->setVisible(true);
        }
    }

    updateConfineCursor();
    updateWindowsKeyDisable();
}

Beatmap *Osu::getSelectedBeatmap() {
    if(m_songBrowser2 != NULL) return m_songBrowser2->getSelectedBeatmap();

    return NULL;
}

float Osu::getDifficultyMultiplier() {
    float difficultyMultiplier = 1.0f;

    if(cv_mod_hardrock.getBool()) difficultyMultiplier = 1.4f;
    if(cv_mod_easy.getBool()) difficultyMultiplier = 0.5f;

    return difficultyMultiplier;
}

float Osu::getCSDifficultyMultiplier() {
    float difficultyMultiplier = 1.0f;

    if(cv_mod_hardrock.getBool()) difficultyMultiplier = 1.3f;  // different!
    if(cv_mod_easy.getBool()) difficultyMultiplier = 0.5f;

    return difficultyMultiplier;
}

float Osu::getScoreMultiplier() {
    float multiplier = 1.0f;

    // Dumb formula, but the values for HT/DT were dumb to begin with
    f32 s = getSelectedBeatmap()->getSpeedMultiplier();
    if(s > 1.f) {
        multiplier *= (0.24 * s) + 0.76;
    } else if(s < 1.f) {
        multiplier *= 0.008 * std::exp(4.81588 * s);
    }

    if(cv_mod_easy.getBool() || (cv_mod_nofail.getBool() && !cv_mod_scorev2.getBool())) multiplier *= 0.50f;
    if(cv_mod_hardrock.getBool()) {
        if(cv_mod_scorev2.getBool())
            multiplier *= 1.10f;
        else
            multiplier *= 1.06f;
    }
    if(cv_mod_flashlight.getBool()) multiplier *= 1.12f;
    if(cv_mod_hidden.getBool()) multiplier *= 1.06f;
    if(cv_mod_spunout.getBool()) multiplier *= 0.90f;

    if(cv_mod_relax.getBool() || cv_mod_autopilot.getBool()) multiplier *= 0.f;

    return multiplier;
}

float Osu::getAnimationSpeedMultiplier() {
    float animationSpeedMultiplier = getSelectedBeatmap()->getSpeedMultiplier();

    if(cv_animation_speed_override.getFloat() >= 0.0f) return max(cv_animation_speed_override.getFloat(), 0.05f);

    return animationSpeedMultiplier;
}

bool Osu::isInPlayMode() { return (m_songBrowser2 != NULL && m_songBrowser2->hasSelectedAndIsPlaying()); }

bool Osu::isNotInPlayModeOrPaused() { return !isInPlayMode() || getSelectedBeatmap()->isPaused(); }

bool Osu::shouldFallBackToLegacySliderRenderer() {
    return cv_force_legacy_slider_renderer.getBool() || cv_mod_wobble.getBool() || cv_mod_wobble2.getBool() ||
           cv_mod_minimize.getBool() || m_modSelector->isCSOverrideSliderActive()
        /* || (m_osu_playfield_rotation->getFloat() < -0.01f || m_osu_playfield_rotation->getFloat() > 0.01f)*/;
}

void Osu::onResolutionChanged(Vector2 newResolution) {
    debugLog("Osu::onResolutionChanged(%i, %i), minimized = %i\n", (int)newResolution.x, (int)newResolution.y,
             (int)engine->isMinimized());

    if(engine->isMinimized()) return;  // ignore if minimized

    const float prevUIScale = getUIScale();

    if(!cv_resolution_enabled.getBool()) {
        g_vInternalResolution = newResolution;
    } else if(!engine->isMinimized()) {  // if we just got minimized, ignore the resolution change (for the internal
                                         // stuff)
        // clamp upwards to internal resolution (osu_resolution)
        if(g_vInternalResolution.x < m_vInternalResolution.x) g_vInternalResolution.x = m_vInternalResolution.x;
        if(g_vInternalResolution.y < m_vInternalResolution.y) g_vInternalResolution.y = m_vInternalResolution.y;

        // clamp downwards to engine resolution
        if(newResolution.x < g_vInternalResolution.x) g_vInternalResolution.x = newResolution.x;
        if(newResolution.y < g_vInternalResolution.y) g_vInternalResolution.y = newResolution.y;

        // disable internal resolution on specific conditions
        bool windowsBorderlessHackCondition =
            (env->getOS() == Environment::OS::WINDOWS && env->isFullscreen() && env->isFullscreenWindowedBorderless() &&
             (int)g_vInternalResolution.y == (int)env->getNativeScreenSize().y);  // HACKHACK
        if(((int)g_vInternalResolution.x == engine->getScreenWidth() &&
            (int)g_vInternalResolution.y == engine->getScreenHeight()) ||
           !env->isFullscreen() || windowsBorderlessHackCondition) {
            debugLog("Internal resolution == Engine resolution || !Fullscreen, disabling resampler (%i, %i)\n",
                     (int)(g_vInternalResolution == engine->getScreenSize()), (int)(!env->isFullscreen()));
            cv_resolution_enabled.setValue(0.0f);
            g_vInternalResolution = engine->getScreenSize();
        }
    }

    // update dpi specific engine globals
    cv_ui_scrollview_scrollbarwidth.setValue(15.0f * Osu::getUIScale());  // not happy with this as a convar

    // interfaces
    for(int i = 0; i < m_screens.size(); i++) {
        m_screens[i]->onResolutionChange(g_vInternalResolution);
    }

    // rendertargets
    rebuildRenderTargets();

    // mouse scale/offset
    updateMouseSettings();

    // cursor clipping
    updateConfineCursor();

    // see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=323
    struct LossyComparisonToFixExcessFPUPrecisionBugBecauseFuckYou {
        static bool equalEpsilon(float f1, float f2) { return std::abs(f1 - f2) < 0.00001f; }
    };

    // a bit hacky, but detect resolution-specific-dpi-scaling changes and force a font and layout reload after a 1
    // frame delay (1/2)
    if(!LossyComparisonToFixExcessFPUPrecisionBugBecauseFuckYou::equalEpsilon(getUIScale(), prevUIScale))
        m_bFireDelayedFontReloadAndResolutionChangeToFixDesyncedUIScaleScheduled = true;
}

void Osu::onDPIChanged() {
    // delay
    m_bFontReloadScheduled = true;
    m_bFireResolutionChangedScheduled = true;
}

void Osu::rebuildRenderTargets() {
    debugLog("Osu::rebuildRenderTargets: %fx%f\n", g_vInternalResolution.x, g_vInternalResolution.y);

    m_backBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);

    if(cv_mod_fposu.getBool())
        m_playfieldBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
    else
        m_playfieldBuffer->rebuild(0, 0, 64, 64);

    m_sliderFrameBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y,
                                 Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X);

    if(cv_mod_mafham.getBool()) {
        m_frameBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
        m_frameBuffer2->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
    } else {
        m_frameBuffer->rebuild(0, 0, 64, 64);
        m_frameBuffer2->rebuild(0, 0, 64, 64);
    }
}

void Osu::reloadFonts() {
    const int baseDPI = 96;
    const int newDPI = Osu::getUIScale() * baseDPI;

    for(McFont *font : m_fonts) {
        if(font->getDPI() != newDPI) {
            font->setDPI(newDPI);
            font->reload();
        }
    }
}

void Osu::updateMouseSettings() {
    // mouse scaling & offset
    Vector2 offset = Vector2(0, 0);
    Vector2 scale = Vector2(1, 1);
    if(cv_resolution_enabled.getBool()) {
        if(cv_letterboxing.getBool()) {
            // special case for osu: since letterboxed raw input absolute to window should mean the 'game' window, and
            // not the 'engine' window, no offset scaling is necessary
            if(cv_mouse_raw_input_absolute_to_window.getBool())
                offset = -Vector2((engine->getScreenWidth() / 2 - g_vInternalResolution.x / 2),
                                  (engine->getScreenHeight() / 2 - g_vInternalResolution.y / 2));
            else
                offset = -Vector2((engine->getScreenWidth() / 2 - g_vInternalResolution.x / 2) *
                                      (1.0f + cv_letterboxing_offset_x.getFloat()),
                                  (engine->getScreenHeight() / 2 - g_vInternalResolution.y / 2) *
                                      (1.0f + cv_letterboxing_offset_y.getFloat()));

            scale = Vector2(g_vInternalResolution.x / engine->getScreenWidth(),
                            g_vInternalResolution.y / engine->getScreenHeight());
        }
    }

    engine->getMouse()->setOffset(offset);
    engine->getMouse()->setScale(scale);
}

void Osu::updateWindowsKeyDisable() {
    if(cv_debug.getBool()) debugLog("Osu::updateWindowsKeyDisable()\n");

    if(cv_win_disable_windows_key_while_playing.getBool()) {
        const bool isPlayerPlaying =
            engine->hasFocus() && isInPlayMode() &&
            (!getSelectedBeatmap()->isPaused() || getSelectedBeatmap()->isRestartScheduled()) &&
            !cv_mod_autoplay.getBool();
        cv_win_disable_windows_key.setValue(isPlayerPlaying ? 1.0f : 0.0f);
    }
}

void Osu::fireResolutionChanged() { onResolutionChanged(g_vInternalResolution); }

void Osu::onInternalResolutionChanged(UString oldValue, UString args) {
    if(args.length() < 7) return;

    const float prevUIScale = getUIScale();

    std::vector<UString> resolution = args.split("x");
    if(resolution.size() != 2)
        debugLog(
            "Error: Invalid parameter count for command 'osu_resolution'! (Usage: e.g. \"osu_resolution 1280x720\")");
    else {
        int width = resolution[0].toFloat();
        int height = resolution[1].toFloat();

        if(width < 300 || height < 240)
            debugLog("Error: Invalid values for resolution for command 'osu_resolution'!");
        else {
            Vector2 newInternalResolution = Vector2(width, height);

            // clamp requested internal resolution to current renderer resolution
            // however, this could happen while we are transitioning into fullscreen. therefore only clamp when not in
            // fullscreen or not in fullscreen transition
            bool isTransitioningIntoFullscreenHack =
                engine->getGraphics()->getResolution().x < env->getNativeScreenSize().x ||
                engine->getGraphics()->getResolution().y < env->getNativeScreenSize().y;
            if(!env->isFullscreen() || !isTransitioningIntoFullscreenHack) {
                if(newInternalResolution.x > engine->getGraphics()->getResolution().x)
                    newInternalResolution.x = engine->getGraphics()->getResolution().x;
                if(newInternalResolution.y > engine->getGraphics()->getResolution().y)
                    newInternalResolution.y = engine->getGraphics()->getResolution().y;
            }

            // enable and store, then force onResolutionChanged()
            cv_resolution_enabled.setValue(1.0f);
            g_vInternalResolution = newInternalResolution;
            m_vInternalResolution = newInternalResolution;
            fireResolutionChanged();
        }
    }

    // a bit hacky, but detect resolution-specific-dpi-scaling changes and force a font and layout reload after a 1
    // frame delay (2/2)
    if(getUIScale() != prevUIScale) m_bFireDelayedFontReloadAndResolutionChangeToFixDesyncedUIScaleScheduled = true;
}

void Osu::onFocusGained() {
    // cursor clipping
    updateConfineCursor();

    if(m_bWasBossKeyPaused) {
        m_bWasBossKeyPaused = false;
        getSelectedBeatmap()->pausePreviewMusic();
    }

    updateWindowsKeyDisable();
    m_volumeOverlay->gainFocus();
}

void Osu::onFocusLost() {
    if(isInPlayMode() && !getSelectedBeatmap()->isPaused() && cv_pause_on_focus_loss.getBool()) {
        if(!bancho.is_playing_a_multi_map() && !getSelectedBeatmap()->is_watching &&
           !getSelectedBeatmap()->is_spectating) {
            getSelectedBeatmap()->pause(false);
            m_pauseMenu->setVisible(true);
            m_modSelector->setVisible(false);
        }
    }

    updateWindowsKeyDisable();
    m_volumeOverlay->loseFocus();

    // release cursor clip
    env->setCursorClip(false, McRect());
}

void Osu::onMinimized() { m_volumeOverlay->loseFocus(); }

bool Osu::onShutdown() {
    debugLog("Osu::onShutdown()\n");

    if(!cv_alt_f4_quits_even_while_playing.getBool() && isInPlayMode()) {
        getSelectedBeatmap()->stop();
        return false;
    }

    // save everything
    m_optionsMenu->save();
    db->save();

    disconnect();

    // the only time where a shutdown could be problematic is while an update is being installed, so we block it here
    return m_updateHandler == NULL || m_updateHandler->getStatus() != UpdateHandler::STATUS::STATUS_INSTALLING_UPDATE;
}

void Osu::onSkinReload() {
    m_bSkinLoadWasReload = true;
    onSkinChange("", cv_skin.getString());
}

void Osu::onSkinChange(UString oldValue, UString newValue) {
    if(m_skin != NULL) {
        if(m_bSkinLoadScheduled || m_skinScheduledToLoad != NULL) return;
        if(newValue.length() < 1) return;
    }

    if(newValue == UString("default")) {
        m_skinScheduledToLoad = new Skin(newValue, MCENGINE_DATA_DIR "materials/default/", true);
        if(m_skin == NULL) m_skin = m_skinScheduledToLoad;
        m_bSkinLoadScheduled = true;
        return;
    }

    std::string neosuSkinFolder = MCENGINE_DATA_DIR "skins/";
    neosuSkinFolder.append(newValue.toUtf8());
    neosuSkinFolder.append("/");
    if(env->directoryExists(neosuSkinFolder)) {
        m_skinScheduledToLoad = new Skin(newValue, neosuSkinFolder, false);
    } else {
        UString ppySkinFolder = cv_osu_folder.getString();
        ppySkinFolder.append("/");
        ppySkinFolder.append(cv_osu_folder_sub_skins.getString());
        ppySkinFolder.append(newValue);
        ppySkinFolder.append("/");
        std::string sf = ppySkinFolder.toUtf8();
        m_skinScheduledToLoad = new Skin(newValue, sf, false);
    }

    // initial load
    if(m_skin == NULL) m_skin = m_skinScheduledToLoad;

    m_bSkinLoadScheduled = true;
}

void Osu::updateAnimationSpeed() {
    if(getSkin() != NULL) {
        float speed = getAnimationSpeedMultiplier() / getSelectedBeatmap()->getSpeedMultiplier();
        getSkin()->setAnimationSpeed(speed >= 0.0f ? speed : 0.0f);
    }
}

void Osu::onAnimationSpeedChange(UString oldValue, UString newValue) { updateAnimationSpeed(); }

void Osu::onSpeedChange(UString oldValue, UString newValue) {
    float speed = newValue.toFloat();
    getSelectedBeatmap()->setSpeed(speed >= 0.0f ? speed : getSelectedBeatmap()->getSpeedMultiplier());
    updateAnimationSpeed();

    // HACKHACK: Update DT/HT buttons, speed slider etc
    cv_mod_doubletime_dummy.setValue(speed == 1.5f, false);
    osu->getModSelector()->m_modButtonDoubletime->setOn(speed == 1.5f, true);
    cv_mod_halftime_dummy.setValue(speed == 0.75f, false);
    osu->getModSelector()->m_modButtonHalftime->setOn(speed == 0.75f, true);
    osu->getModSelector()->updateButtons(true);
    osu->getModSelector()->updateScoreMultiplierLabelText();
    osu->getModSelector()->updateOverrideSliderLabels();
}

void Osu::onDTPresetChange(UString oldValue, UString newValue) {
    cv_speed_override.setValue(cv_mod_doubletime_dummy.getBool() ? 1.5f : -1.f);
    osu->getModSelector()->m_speedSlider->setValue(cv_speed_override.getFloat(), false, false);
}

void Osu::onHTPresetChange(UString oldValue, UString newValue) {
    cv_speed_override.setValue(cv_mod_halftime_dummy.getBool() ? 0.75f : -1.f);
    osu->getModSelector()->m_speedSlider->setValue(cv_speed_override.getFloat(), false, false);
}

void Osu::onPlayfieldChange(UString oldValue, UString newValue) { getSelectedBeatmap()->onModUpdate(); }

void Osu::onUIScaleChange(UString oldValue, UString newValue) {
    const float oldVal = oldValue.toFloat();
    const float newVal = newValue.toFloat();

    if(oldVal != newVal) {
        // delay
        m_bFontReloadScheduled = true;
        m_bFireResolutionChangedScheduled = true;
    }
}

void Osu::onUIScaleToDPIChange(UString oldValue, UString newValue) {
    const bool oldVal = oldValue.toFloat() > 0.0f;
    const bool newVal = newValue.toFloat() > 0.0f;

    if(oldVal != newVal) {
        // delay
        m_bFontReloadScheduled = true;
        m_bFireResolutionChangedScheduled = true;
    }
}

void Osu::onLetterboxingChange(UString oldValue, UString newValue) {
    if(cv_resolution_enabled.getBool()) {
        bool oldVal = oldValue.toFloat() > 0.0f;
        bool newVal = newValue.toFloat() > 0.0f;

        if(oldVal != newVal) m_bFireResolutionChangedScheduled = true;  // delay
    }
}

void Osu::updateConfineCursor() {
    if(cv_debug.getBool()) debugLog("Osu::updateConfineCursor()\n");

    if((cv_confine_cursor_fullscreen.getBool() && env->isFullscreen()) ||
       (cv_confine_cursor_windowed.getBool() && !env->isFullscreen()) ||
       (isInPlayMode() && !m_pauseMenu->isVisible() && !getModAuto() && !getModAutopilot() &&
        !getSelectedBeatmap()->is_watching && !getSelectedBeatmap()->is_spectating))
        env->setCursorClip(true, McRect());
    else
        env->setCursorClip(false, McRect());
}

void Osu::onConfineCursorWindowedChange(UString oldValue, UString newValue) { updateConfineCursor(); }

void Osu::onConfineCursorFullscreenChange(UString oldValue, UString newValue) { updateConfineCursor(); }

void Osu::onKey1Change(bool pressed, bool mouse) {
    int numKeys1Down = 0;
    if(m_bKeyboardKey1Down) numKeys1Down++;
    if(m_bKeyboardKey12Down) numKeys1Down++;
    if(m_bMouseKey1Down) numKeys1Down++;

    const bool isKeyPressed1Allowed =
        (numKeys1Down == 1);  // all key1 keys (incl. multiple bindings) act as one single key with state handover

    // WARNING: if paused, keyReleased*() will be called out of sequence every time due to the fix. do not put actions
    // in it
    if(isInPlayMode() /* && !getSelectedBeatmap()->isPaused()*/)  // NOTE: allow keyup even while beatmap is paused, to
                                                                  // correctly not-continue immediately due to pressed
                                                                  // keys
    {
        if(!(mouse && cv_disable_mousebuttons.getBool())) {
            // quickfix
            if(cv_disable_mousebuttons.getBool()) m_bMouseKey1Down = false;

            if(pressed && isKeyPressed1Allowed && !getSelectedBeatmap()->isPaused())  // see above note
                getSelectedBeatmap()->keyPressed1(mouse);
            else if(!m_bKeyboardKey1Down && !m_bKeyboardKey12Down && !m_bMouseKey1Down)
                getSelectedBeatmap()->keyReleased1(mouse);
        }
    }

    // cursor anim + ripples
    const bool doAnimate =
        !(isInPlayMode() && !getSelectedBeatmap()->isPaused() && mouse && cv_disable_mousebuttons.getBool());
    if(doAnimate) {
        if(pressed && isKeyPressed1Allowed) {
            m_hud->animateCursorExpand();
            m_hud->addCursorRipple(engine->getMouse()->getPos());
        } else if(!m_bKeyboardKey1Down && !m_bKeyboardKey12Down && !m_bMouseKey1Down && !m_bKeyboardKey2Down &&
                  !m_bKeyboardKey22Down && !m_bMouseKey2Down)
            m_hud->animateCursorShrink();
    }
}

void Osu::onKey2Change(bool pressed, bool mouse) {
    int numKeys2Down = 0;
    if(m_bKeyboardKey2Down) numKeys2Down++;
    if(m_bKeyboardKey22Down) numKeys2Down++;
    if(m_bMouseKey2Down) numKeys2Down++;

    const bool isKeyPressed2Allowed =
        (numKeys2Down == 1);  // all key2 keys (incl. multiple bindings) act as one single key with state handover

    // WARNING: if paused, keyReleased*() will be called out of sequence every time due to the fix. do not put actions
    // in it
    if(isInPlayMode() /* && !getSelectedBeatmap()->isPaused()*/)  // NOTE: allow keyup even while beatmap is paused, to
                                                                  // correctly not-continue immediately due to pressed
                                                                  // keys
    {
        if(!(mouse && cv_disable_mousebuttons.getBool())) {
            // quickfix
            if(cv_disable_mousebuttons.getBool()) m_bMouseKey2Down = false;

            if(pressed && isKeyPressed2Allowed && !getSelectedBeatmap()->isPaused())  // see above note
                getSelectedBeatmap()->keyPressed2(mouse);
            else if(!m_bKeyboardKey2Down && !m_bKeyboardKey22Down && !m_bMouseKey2Down)
                getSelectedBeatmap()->keyReleased2(mouse);
        }
    }

    // cursor anim + ripples
    const bool doAnimate =
        !(isInPlayMode() && !getSelectedBeatmap()->isPaused() && mouse && cv_disable_mousebuttons.getBool());
    if(doAnimate) {
        if(pressed && isKeyPressed2Allowed) {
            m_hud->animateCursorExpand();
            m_hud->addCursorRipple(engine->getMouse()->getPos());
        } else if(!m_bKeyboardKey2Down && !m_bKeyboardKey22Down && !m_bMouseKey2Down && !m_bKeyboardKey1Down &&
                  !m_bKeyboardKey12Down && !m_bMouseKey1Down)
            m_hud->animateCursorShrink();
    }
}

void Osu::onModMafhamChange(UString oldValue, UString newValue) { rebuildRenderTargets(); }

void Osu::onModFPoSuChange(UString oldValue, UString newValue) { rebuildRenderTargets(); }

void Osu::onModFPoSu3DChange(UString oldValue, UString newValue) { rebuildRenderTargets(); }

void Osu::onModFPoSu3DSpheresChange(UString oldValue, UString newValue) { rebuildRenderTargets(); }

void Osu::onModFPoSu3DSpheresAAChange(UString oldValue, UString newValue) { rebuildRenderTargets(); }

void Osu::onLetterboxingOffsetChange(UString oldValue, UString newValue) { updateMouseSettings(); }

float Osu::getImageScaleToFitResolution(Vector2 size, Vector2 resolution) {
    return resolution.x / size.x > resolution.y / size.y ? resolution.y / size.y : resolution.x / size.x;
}

float Osu::getImageScaleToFitResolution(Image *img, Vector2 resolution) {
    return getImageScaleToFitResolution(Vector2(img->getWidth(), img->getHeight()), resolution);
}

float Osu::getImageScaleToFillResolution(Vector2 size, Vector2 resolution) {
    return resolution.x / size.x < resolution.y / size.y ? resolution.y / size.y : resolution.x / size.x;
}

float Osu::getImageScaleToFillResolution(Image *img, Vector2 resolution) {
    return getImageScaleToFillResolution(Vector2(img->getWidth(), img->getHeight()), resolution);
}

float Osu::getImageScale(Vector2 size, float osuSize) {
    int swidth = osu->getScreenWidth();
    int sheight = osu->getScreenHeight();

    if(swidth * 3 > sheight * 4)
        swidth = sheight * 4 / 3;
    else
        sheight = swidth * 3 / 4;

    const float xMultiplier = swidth / osuBaseResolution.x;
    const float yMultiplier = sheight / osuBaseResolution.y;

    const float xDiameter = osuSize * xMultiplier;
    const float yDiameter = osuSize * yMultiplier;

    return xDiameter / size.x > yDiameter / size.y ? xDiameter / size.x : yDiameter / size.y;
}

float Osu::getImageScale(Image *img, float osuSize) {
    return getImageScale(Vector2(img->getWidth(), img->getHeight()), osuSize);
}

float Osu::getUIScale(float osuResolutionRatio) {
    int swidth = osu->getScreenWidth();
    int sheight = osu->getScreenHeight();

    if(swidth * 3 > sheight * 4)
        swidth = sheight * 4 / 3;
    else
        sheight = swidth * 3 / 4;

    const float xMultiplier = swidth / osuBaseResolution.x;
    const float yMultiplier = sheight / osuBaseResolution.y;

    const float xDiameter = osuResolutionRatio * xMultiplier;
    const float yDiameter = osuResolutionRatio * yMultiplier;

    return xDiameter > yDiameter ? xDiameter : yDiameter;
}

float Osu::getUIScale() {
    if(osu != NULL) {
        if(osu->getScreenWidth() < cv_ui_scale_to_dpi_minimum_width.getInt() ||
           osu->getScreenHeight() < cv_ui_scale_to_dpi_minimum_height.getInt())
            return cv_ui_scale.getFloat();
    } else if(engine->getScreenWidth() < cv_ui_scale_to_dpi_minimum_width.getInt() ||
              engine->getScreenHeight() < cv_ui_scale_to_dpi_minimum_height.getInt())
        return cv_ui_scale.getFloat();

    return ((cv_ui_scale_to_dpi.getBool() ? env->getDPIScale() : 1.0f) * cv_ui_scale.getFloat());
}

bool Osu::findIgnoreCase(const std::string &haystack, const std::string &needle) {
    auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
                          [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); });

    return (it != haystack.end());
}
