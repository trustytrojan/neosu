#include "Osu.h"

#include <sstream>

#include "AnimationHandler.h"
#include "BackgroundImageHandler.h"
#include "Bancho.h"
#include "BanchoNetworking.h"
#include "Beatmap.h"
#include "CBaseUIScrollView.h"
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
#include "Lobby.h"
#include "MainMenu.h"
#include "ModFPoSu.h"
#include "ModSelector.h"
#include "Mouse.h"
#include "NotificationOverlay.h"
#include "OptionsMenu.h"
#include "PauseMenu.h"
#include "PromptScreen.h"
#include "RankingScreen.h"
#include "RenderTarget.h"
#include "Replay.h"
#include "ResourceManager.h"
#include "RichPresence.h"
#include "RoomScreen.h"
#include "Shader.h"
#include "Skin.h"
#include "SongBrowser/SongBrowser.h"
#include "SoundEngine.h"
#include "SpectatorScreen.h"
#include "TooltipOverlay.h"
#include "UIModSelectorModButton.h"
#include "UIUserContextMenu.h"
#include "UpdateHandler.h"
#include "VolumeOverlay.h"
#include "score.h"

using namespace std;

Osu *osu = NULL;

// release configuration
ConVar auto_update("auto_update", true, FCVAR_DEFAULT);
ConVar osu_version("osu_version", 35.08f, FCVAR_DEFAULT | FCVAR_HIDDEN);

#ifdef _DEBUG
ConVar osu_debug("osu_debug", true, FCVAR_DEFAULT);
#else
ConVar osu_debug("osu_debug", false, FCVAR_DEFAULT);
#endif

ConVar osu_disable_mousebuttons("osu_disable_mousebuttons", false, FCVAR_DEFAULT);
ConVar osu_disable_mousewheel("osu_disable_mousewheel", false, FCVAR_DEFAULT);
ConVar osu_confine_cursor_windowed("osu_confine_cursor_windowed", false, FCVAR_DEFAULT);
ConVar osu_confine_cursor_fullscreen("osu_confine_cursor_fullscreen", true, FCVAR_DEFAULT);

ConVar osu_skin("osu_skin", "default", FCVAR_DEFAULT);
ConVar osu_skin_reload("osu_skin_reload");

ConVar osu_volume_master("osu_volume_master", 1.0f, FCVAR_DEFAULT);
ConVar osu_volume_master_inactive("osu_volume_master_inactive", 0.25f, FCVAR_DEFAULT);
ConVar osu_volume_effects("osu_volume_effects", 1.0f, FCVAR_DEFAULT);
ConVar osu_volume_music("osu_volume_music", 0.4f, FCVAR_DEFAULT);
ConVar osu_volume_change_interval("osu_volume_change_interval", 0.05f, FCVAR_DEFAULT);
ConVar osu_hud_volume_duration("osu_hud_volume_duration", 1.0f, FCVAR_DEFAULT);
ConVar osu_hud_volume_size_multiplier("osu_hud_volume_size_multiplier", 1.5f, FCVAR_DEFAULT);

ConVar osu_speed_override("osu_speed_override", -1.0f, FCVAR_UNLOCKED);
ConVar osu_animation_speed_override("osu_animation_speed_override", -1.0f, FCVAR_LOCKED);

ConVar osu_pause_on_focus_loss("osu_pause_on_focus_loss", true, FCVAR_DEFAULT);
ConVar osu_quick_retry_delay("osu_quick_retry_delay", 0.27f, FCVAR_DEFAULT);
ConVar osu_scrubbing_smooth("osu_scrubbing_smooth", true, FCVAR_DEFAULT);
ConVar osu_skip_intro_enabled("osu_skip_intro_enabled", true, FCVAR_DEFAULT,
                              "enables/disables skip button for intro until first hitobject");
ConVar osu_skip_breaks_enabled("osu_skip_breaks_enabled", true, FCVAR_DEFAULT,
                               "enables/disables skip button for breaks in the middle of beatmaps");
ConVar osu_seek_delta("osu_seek_delta", 5, FCVAR_DEFAULT,
                      "how many seconds to skip backward/forward when quick seeking");

ConVar osu_mods("osu_mods", "", FCVAR_DEFAULT);
ConVar osu_mod_touchdevice("osu_mod_touchdevice", false, FCVAR_DEFAULT, "used for force applying touch pp nerf always");
ConVar osu_mod_fadingcursor("osu_mod_fadingcursor", false, FCVAR_UNLOCKED);
ConVar osu_mod_fadingcursor_combo("osu_mod_fadingcursor_combo", 50.0f, FCVAR_DEFAULT);
ConVar osu_mod_endless("osu_mod_endless", false, FCVAR_LOCKED);

ConVar osu_notification("osu_notification");
ConVar osu_notification_color_r("osu_notification_color_r", 255, FCVAR_DEFAULT);
ConVar osu_notification_color_g("osu_notification_color_g", 255, FCVAR_DEFAULT);
ConVar osu_notification_color_b("osu_notification_color_b", 255, FCVAR_DEFAULT);

ConVar osu_ui_scale("osu_ui_scale", 1.0f, FCVAR_DEFAULT, "multiplier");
ConVar osu_ui_scale_to_dpi("osu_ui_scale_to_dpi", true, FCVAR_DEFAULT,
                           "whether the game should scale its UI based on the DPI reported by your operating system");
ConVar osu_ui_scale_to_dpi_minimum_width(
    "osu_ui_scale_to_dpi_minimum_width", 2200, FCVAR_DEFAULT,
    "any in-game resolutions below this will have osu_ui_scale_to_dpi force disabled");
ConVar osu_ui_scale_to_dpi_minimum_height(
    "osu_ui_scale_to_dpi_minimum_height", 1300, FCVAR_DEFAULT,
    "any in-game resolutions below this will have osu_ui_scale_to_dpi force disabled");
ConVar osu_letterboxing("osu_letterboxing", true, FCVAR_DEFAULT);
ConVar osu_letterboxing_offset_x("osu_letterboxing_offset_x", 0.0f, FCVAR_DEFAULT);
ConVar osu_letterboxing_offset_y("osu_letterboxing_offset_y", 0.0f, FCVAR_DEFAULT);
ConVar osu_resolution("osu_resolution", "1280x720", FCVAR_DEFAULT);
ConVar osu_resolution_enabled("osu_resolution_enabled", false, FCVAR_DEFAULT);
ConVar osu_resolution_keep_aspect_ratio("osu_resolution_keep_aspect_ratio", false, FCVAR_DEFAULT);
ConVar osu_force_legacy_slider_renderer("osu_force_legacy_slider_renderer", false, FCVAR_DEFAULT,
                                        "on some older machines, this may be faster than vertexbuffers");

ConVar osu_draw_fps("osu_draw_fps", true, FCVAR_DEFAULT);

ConVar osu_alt_f4_quits_even_while_playing("osu_alt_f4_quits_even_while_playing", true, FCVAR_DEFAULT);
ConVar osu_win_disable_windows_key_while_playing("osu_win_disable_windows_key_while_playing", true, FCVAR_DEFAULT);

ConVar avoid_flashes("avoid_flashes", false, FCVAR_DEFAULT, "disable flashing elements (like FL dimming on sliders)");
ConVar flashlight_radius("flashlight_radius", 100.f, FCVAR_LOCKED);
ConVar flashlight_follow_delay("flashlight_follow_delay", 0.120f, FCVAR_LOCKED);
ConVar flashlight_always_hard("flashlight_always_hard", false, FCVAR_DEFAULT,
                              "always use 200+ combo flashlight radius");

ConVar main_menu_use_server_logo("main_menu_use_server_logo", true, FCVAR_DEFAULT);
ConVar start_first_main_menu_song_at_preview_point("start_first_main_menu_song_at_preview_point", false, FCVAR_DEFAULT);
ConVar nightcore_enjoyer("nightcore_enjoyer", false, FCVAR_DEFAULT,
                         "automatically select nightcore when speed modifying");
ConVar scoreboard_animations("scoreboard_animations", true, FCVAR_DEFAULT, "animate in-game scoreboard");
ConVar instant_replay_duration("instant_replay_duration", 15.f, FCVAR_DEFAULT,
                               "instant replay (F2) duration, in seconds");
ConVar normalize_loudness("normalize_loudness", false, FCVAR_DEFAULT, "normalize loudness across songs");
ConVar restart_sound_engine_before_playing("restart_sound_engine_before_playing", false, FCVAR_DEFAULT,
                                           "jank fix for users who experience sound issues after playing for a while");
ConVar instafade("instafade", false, FCVAR_DEFAULT, "don't draw hitcircle fadeout animations");
ConVar sort_skins_by_name("sort_skins_by_name", true, FCVAR_DEFAULT, "set to false to use old behavior");

ConVar use_https("use_https", true, FCVAR_DEFAULT);
ConVar mp_server("mp_server", "akatsuki.gg", FCVAR_DEFAULT);
ConVar mp_password("mp_password", "", FCVAR_DEFAULT | FCVAR_HIDDEN);
ConVar mp_autologin("mp_autologin", false, FCVAR_DEFAULT);
ConVar submit_scores("submit_scores", false, FCVAR_DEFAULT);
ConVar submit_after_pause("submit_after_pause", true, FCVAR_DEFAULT);

// If catboy.best doesn't work for you, here are some alternatives:
// - https://api.osu.direct/d/
// - https://chimu.moe/d/
// - https://api.nerinyan.moe/d/
// - https://osu.gatari.pw/d/
// - https://osu.sayobot.cn/osu.php?s=
ConVar beatmap_mirror("beatmap_mirror", "https://catboy.best/d/", FCVAR_DEFAULT,
                      "mirror from which beatmapsets will be downloaded");

ConVar *Osu::version = &osu_version;
ConVar *Osu::debug = &osu_debug;
ConVar *Osu::ui_scale = &osu_ui_scale;
Vector2 Osu::g_vInternalResolution;
Vector2 Osu::osuBaseResolution = Vector2(640.0f, 480.0f);

Shader *flashlight_shader = NULL;

Osu::Osu() {
    osu = this;

    srand(time(NULL));

    bancho.neosu_version = UString::format("%.2f-" NEOSU_STREAM, osu_version.getFloat());
    bancho.user_agent =
        UString::format("Mozilla/5.0 (compatible; neosu/%s; +" NEOSU_URL "/)", bancho.neosu_version.toUtf8());

    // convar refs
    m_osu_folder_ref = convar->getConVarByName("osu_folder");
    m_osu_folder_sub_skins_ref = convar->getConVarByName("osu_folder_sub_skins");
    m_osu_draw_hud_ref = convar->getConVarByName("osu_draw_hud");
    m_osu_draw_scoreboard = convar->getConVarByName("osu_draw_scoreboard");
    m_osu_draw_scoreboard_mp = convar->getConVarByName("osu_draw_scoreboard_mp");
    m_osu_draw_cursor_ripples_ref = convar->getConVarByName("osu_draw_cursor_ripples");
    m_osu_mod_fps_ref = convar->getConVarByName("osu_mod_fps");
    m_osu_mod_minimize_ref = convar->getConVarByName("osu_mod_minimize");
    m_osu_mod_wobble_ref = convar->getConVarByName("osu_mod_wobble");
    m_osu_mod_wobble2_ref = convar->getConVarByName("osu_mod_wobble2");
    m_osu_playfield_rotation = convar->getConVarByName("osu_playfield_rotation");
    m_osu_playfield_stretch_x = convar->getConVarByName("osu_playfield_stretch_x");
    m_osu_playfield_stretch_y = convar->getConVarByName("osu_playfield_stretch_y");
    m_fposu_draw_cursor_trail_ref = convar->getConVarByName("fposu_draw_cursor_trail");
    m_osu_mod_mafham_ref = convar->getConVarByName("osu_mod_mafham");
    m_osu_mod_fposu_ref = convar->getConVarByName("osu_mod_fposu");
    m_snd_change_check_interval_ref = convar->getConVarByName("snd_change_check_interval");
    m_ui_scrollview_scrollbarwidth_ref = convar->getConVarByName("ui_scrollview_scrollbarwidth");
    m_mouse_raw_input_absolute_to_window_ref = convar->getConVarByName("mouse_raw_input_absolute_to_window");
    m_win_disable_windows_key_ref = convar->getConVarByName("win_disable_windows_key");

    // experimental mods list
    m_experimentalMods.push_back(convar->getConVarByName("fposu_mod_strafing"));
    m_experimentalMods.push_back(convar->getConVarByName("fposu_mod_3d_depthwobble"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_wobble"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_arwobble"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_timewarp"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_artimewarp"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_minimize"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_fadingcursor"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_fps"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_jigsaw1"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_jigsaw2"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_fullalternate"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_random"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_reverse_sliders"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_no50s"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_no100s"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_ming3012"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_halfwindow"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_millhioref"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_mafham"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_strict_tracking"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_playfield_mirror_horizontal"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_playfield_mirror_vertical"));

    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_wobble2"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_shirone"));
    m_experimentalMods.push_back(convar->getConVarByName("osu_mod_approach_different"));

    env->setWindowTitle("neosu");
    env->setCursorVisible(false);

    engine->getConsoleBox()->setRequireShiftToActivate(true);
    engine->getMouse()->addListener(this);

    convar->getConVarByName("vsync")->setValue(0.0f);
    convar->getConVarByName("fps_max")->setValue(420.0f);
    convar->getConVarByName("fps_max")->setDefaultFloat(420.0f);

    m_snd_change_check_interval_ref->setDefaultFloat(0.5f);
    m_snd_change_check_interval_ref->setValue(m_snd_change_check_interval_ref->getDefaultFloat());

    osu_resolution.setValue(UString::format("%ix%i", engine->getScreenWidth(), engine->getScreenHeight()));

    env->setWindowResizable(false);

    // generate default osu! appdata user path
    std::string userDataPath = env->getUserDataPath();
    if(userDataPath.length() > 1) {
        std::string defaultOsuFolder = userDataPath;
        defaultOsuFolder.append(env->getOS() == Environment::OS::WINDOWS ? "\\osu!\\" : "/osu!/");
        m_osu_folder_ref->setValue(defaultOsuFolder.c_str());
    }

    // convar callbacks
    osu_speed_override.setCallback(fastdelegate::MakeDelegate(this, &Osu::onSpeedChange));
    osu_animation_speed_override.setCallback(fastdelegate::MakeDelegate(this, &Osu::onAnimationSpeedChange));

    m_osu_playfield_rotation->setCallback(fastdelegate::MakeDelegate(this, &Osu::onPlayfieldChange));
    m_osu_playfield_stretch_x->setCallback(fastdelegate::MakeDelegate(this, &Osu::onPlayfieldChange));
    m_osu_playfield_stretch_y->setCallback(fastdelegate::MakeDelegate(this, &Osu::onPlayfieldChange));

    osu_mods.setCallback(fastdelegate::MakeDelegate(this, &Osu::updateModsForConVarTemplate));

    osu_resolution.setCallback(fastdelegate::MakeDelegate(this, &Osu::onInternalResolutionChanged));
    osu_ui_scale.setCallback(fastdelegate::MakeDelegate(this, &Osu::onUIScaleChange));
    osu_ui_scale_to_dpi.setCallback(fastdelegate::MakeDelegate(this, &Osu::onUIScaleToDPIChange));
    osu_letterboxing.setCallback(fastdelegate::MakeDelegate(this, &Osu::onLetterboxingChange));
    osu_letterboxing_offset_x.setCallback(fastdelegate::MakeDelegate(this, &Osu::onLetterboxingOffsetChange));
    osu_letterboxing_offset_y.setCallback(fastdelegate::MakeDelegate(this, &Osu::onLetterboxingOffsetChange));

    osu_confine_cursor_windowed.setCallback(fastdelegate::MakeDelegate(this, &Osu::onConfineCursorWindowedChange));
    osu_confine_cursor_fullscreen.setCallback(fastdelegate::MakeDelegate(this, &Osu::onConfineCursorFullscreenChange));

    convar->getConVarByName("osu_playfield_mirror_horizontal")
        ->setCallback(fastdelegate::MakeDelegate(
            this, &Osu::updateModsForConVarTemplate));  // force a mod update on Beatmap if changed
    convar->getConVarByName("osu_playfield_mirror_vertical")
        ->setCallback(fastdelegate::MakeDelegate(
            this, &Osu::updateModsForConVarTemplate));  // force a mod update on Beatmap if changed

    osu_notification.setCallback(fastdelegate::MakeDelegate(this, &Osu::onNotification));

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

    m_bModAuto = false;
    m_bModAutopilot = false;
    m_bModRelax = false;
    m_bModSpunout = false;
    m_bModTarget = false;
    m_bModScorev2 = false;
    m_bModFlashlight = false;
    m_bModDT = false;
    m_bModNC = false;
    m_bModNF = false;
    m_bModHT = false;
    m_bModDC = false;
    m_bModHD = false;
    m_bModHR = false;
    m_bModEZ = false;
    m_bModSD = false;
    m_bModSS = false;
    m_bModNightmare = false;
    m_bModTD = false;

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
    m_score = new LiveScore();
    m_updateHandler = new UpdateHandler();

    // exec the main config file (this must be right here!)
    Console::execConfigFile("underride");  // same as override, but for defaults
    Console::execConfigFile("osu");
    Console::execConfigFile("override");  // used for quickfixing live builds without redeploying/recompiling

    // Old value was invalid
    if(beatmap_mirror.getString() == UString("https://catboy.best/s/")) {
        beatmap_mirror.setValue("https://catboy.best/d/");
    }

    // Initialize sound here so we can load the preferred device from config
    // Avoids initializing the sound device twice, which can take a while depending on the driver
    auto sound_engine = engine->getSound();
    sound_engine->updateOutputDevices(true);
    sound_engine->initializeOutputDevice(sound_engine->getWantedDevice());
    convar->getConVarByName("snd_output_device")->setValue(sound_engine->getOutputDeviceName());
    convar->getConVarByName("snd_freq")
        ->setCallback(fastdelegate::MakeDelegate(sound_engine, &SoundEngine::onFreqChanged));
    convar->getConVarByName("snd_restart")
        ->setCallback(fastdelegate::MakeDelegate(sound_engine, &SoundEngine::restart));
    convar->getConVarByName("win_snd_wasapi_exclusive")->setCallback(_RESTART_SOUND_ENGINE_ON_CHANGE);
    convar->getConVarByName("win_snd_wasapi_buffer_size")->setCallback(_RESTART_SOUND_ENGINE_ON_CHANGE);
    convar->getConVarByName("win_snd_wasapi_period_size")->setCallback(_RESTART_SOUND_ENGINE_ON_CHANGE);
    convar->getConVarByName("asio_buffer_size")->setCallback(_RESTART_SOUND_ENGINE_ON_CHANGE);

    // Initialize skin after sound engine has started, or else sounds won't load properly
    osu_skin.setCallback(fastdelegate::MakeDelegate(this, &Osu::onSkinChange));
    osu_skin_reload.setCallback(fastdelegate::MakeDelegate(this, &Osu::onSkinReload));
    onSkinChange("", osu_skin.getString());

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
        UString skinFolder = m_osu_folder_ref->getString();
        skinFolder.append(m_osu_folder_sub_skins_ref->getString());
        skinFolder.append(osu_skin.getString());
        skinFolder.append("/");
        if(m_skin == NULL)  // the skin may already be loaded by Console::execConfigFile() above
            onSkinChange("", osu_skin.getString());

        // enable async skin loading for user-action skin changes (but not during startup)
        Skin::m_osu_skin_async->setValue(1.0f);
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

    // update mod settings
    updateMods();

    // Init online functionality (multiplayer/leaderboards/etc)
    init_networking_thread();
    if(mp_autologin.getBool()) {
        reconnect();
    }

    m_mainMenu->setVisible(true);
    m_updateHandler->checkForUpdates();

    // memory/performance optimization; if osu_mod_mafham is not enabled, reduce the two rendertarget sizes to 64x64,
    m_osu_mod_mafham_ref->setCallback(fastdelegate::MakeDelegate(this, &Osu::onModMafhamChange));
    m_osu_mod_fposu_ref->setCallback(fastdelegate::MakeDelegate(this, &Osu::onModFPoSuChange));

    // Not the type of shader you want players to tweak or delete, so loading from string
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
    engine->getResourceManager()->loadResource(flashlight_shader);
}

Osu::~Osu() {
    osu = NULL;

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
}

void Osu::draw(Graphics *g) {
    if(m_skin == NULL)  // sanity check
    {
        g->setColor(0xffff0000);
        g->fillRect(0, 0, getScreenWidth(), getScreenHeight());
        return;
    }

    // if we are not using the native window resolution, draw into the buffer
    const bool isBufferedDraw = osu_resolution_enabled.getBool();

    if(isBufferedDraw) m_backBuffer->enable();

    // draw everything in the correct order
    if(isInPlayMode()) {  // if we are playing a beatmap
        Beatmap *beatmap = getSelectedBeatmap();
        const bool isFPoSu = (m_osu_mod_fposu_ref->getBool());

        if(isFPoSu) m_playfieldBuffer->enable();

        getSelectedBeatmap()->draw(g);

        if(m_bModFlashlight) {
            // Dim screen when holding a slider
            float max_opacity = 1.f;
            if(holding_slider && !avoid_flashes.getBool()) {
                max_opacity = 0.2f;
            }

            // Convert screen mouse -> osu mouse pos
            Vector2 cursorPos = beatmap->getCursorPos();
            Vector2 mouse_position = cursorPos - GameRules::getPlayfieldOffset();
            mouse_position /= GameRules::getPlayfieldScaleFactor();

            // Update flashlight position
            double follow_delay = flashlight_follow_delay.getFloat();
            double frame_time = min(engine->getFrameTime(), follow_delay);
            double t = frame_time / follow_delay;
            t = t * (2.f - t);
            flashlight_position += t * (mouse_position - flashlight_position);
            Vector2 flashlightPos =
                flashlight_position * GameRules::getPlayfieldScaleFactor() + GameRules::getPlayfieldOffset();

            float fl_radius = flashlight_radius.getFloat() * GameRules::getPlayfieldScaleFactor();
            if(getScore()->getCombo() >= 200 || convar->getConVarByName("flashlight_always_hard")->getBool()) {
                fl_radius *= 0.625f;
            } else if(getScore()->getCombo() >= 100) {
                fl_radius *= 0.8125f;
            }

            flashlight_shader->enable();
            flashlight_shader->setUniform1f("max_opacity", max_opacity);
            flashlight_shader->setUniform1f("flashlight_radius", fl_radius);
            flashlight_shader->setUniform2f("flashlight_center", flashlightPos.x, getScreenSize().y - flashlightPos.y);
            g->setColor(COLOR(255, 0, 0, 0));
            g->fillRect(0, 0, getScreenWidth(), getScreenHeight());
            flashlight_shader->disable();
        }

        if(!isFPoSu) m_hud->draw(g);

        // quick retry fadeout overlay
        if(m_fQuickRetryTime != 0.0f && m_bQuickRetryDown) {
            float alphaPercent = 1.0f - (m_fQuickRetryTime - engine->getTime()) / osu_quick_retry_delay.getFloat();
            if(engine->getTime() > m_fQuickRetryTime) alphaPercent = 1.0f;

            g->setColor(COLOR((int)(255 * alphaPercent), 0, 0, 0));
            g->fillRect(0, 0, getScreenWidth(), getScreenHeight());
        }

        m_pauseMenu->draw(g);
        m_modSelector->draw(g);
        m_chat->draw(g);
        m_optionsMenu->draw(g);

        if(osu_draw_fps.getBool() && (!isFPoSu)) m_hud->drawFps(g);

        m_windowManager->draw(g);

        if(isFPoSu && m_osu_draw_cursor_ripples_ref->getBool()) m_hud->drawCursorRipples(g);

        // draw FPoSu cursor trail
        float fadingCursorAlpha =
            1.0f - clamp<float>((float)m_score->getCombo() / osu_mod_fadingcursor_combo.getFloat(), 0.0f, 1.0f);
        if(m_pauseMenu->isVisible() || getSelectedBeatmap()->isContinueScheduled() || !osu_mod_fadingcursor.getBool())
            fadingCursorAlpha = 1.0f;
        if(isFPoSu && m_fposu_draw_cursor_trail_ref->getBool())
            m_hud->drawCursorTrail(g, beatmap->getCursorPos(), fadingCursorAlpha);

        if(isFPoSu) {
            m_playfieldBuffer->disable();
            m_fposu->draw(g);
            m_hud->draw(g);

            if(osu_draw_fps.getBool()) m_hud->drawFps(g);
        }

        // draw player cursor
        Vector2 cursorPos = beatmap->getCursorPos();
        bool drawSecondTrail = (m_bModAuto || m_bModAutopilot || beatmap->is_watching || beatmap->is_spectating);
        bool updateAndDrawTrail = true;
        if(isFPoSu) {
            cursorPos = getScreenSize() / 2.0f;
            updateAndDrawTrail = false;
        }
        m_hud->drawCursor(g, cursorPos, fadingCursorAlpha, drawSecondTrail, updateAndDrawTrail);
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

        if(osu_draw_fps.getBool()) m_hud->drawFps(g);

        m_windowManager->draw(g);
        m_hud->drawCursor(g, engine->getMouse()->getPos());
    }

    m_tooltipOverlay->draw(g);
    m_notificationOverlay->draw(g);
    m_volumeOverlay->draw(g);

    // loading spinner for some async tasks
    if((m_bSkinLoadScheduled && m_skin != m_skinScheduledToLoad)) {
        m_hud->drawLoadingSmall(g, "");
    }

    // if we are not using the native window resolution
    if(isBufferedDraw) {
        // draw a scaled version from the buffer to the screen
        m_backBuffer->disable();

        Vector2 offset = Vector2(engine->getGraphics()->getResolution().x / 2 - g_vInternalResolution.x / 2,
                                 engine->getGraphics()->getResolution().y / 2 - g_vInternalResolution.y / 2);
        g->setBlending(false);
        if(osu_letterboxing.getBool()) {
            m_backBuffer->draw(g, offset.x * (1.0f + osu_letterboxing_offset_x.getFloat()),
                               offset.y * (1.0f + osu_letterboxing_offset_y.getFloat()), g_vInternalResolution.x,
                               g_vInternalResolution.y);
        } else {
            if(osu_resolution_keep_aspect_ratio.getBool()) {
                const float scale =
                    getImageScaleToFitResolution(m_backBuffer->getSize(), engine->getGraphics()->getResolution());
                const float scaledWidth = m_backBuffer->getWidth() * scale;
                const float scaledHeight = m_backBuffer->getHeight() * scale;
                m_backBuffer->draw(g,
                                   max(0.0f, engine->getGraphics()->getResolution().x / 2.0f - scaledWidth / 2.0f) *
                                       (1.0f + osu_letterboxing_offset_x.getFloat()),
                                   max(0.0f, engine->getGraphics()->getResolution().y / 2.0f - scaledHeight / 2.0f) *
                                       (1.0f + osu_letterboxing_offset_y.getFloat()),
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

    if(isInPlayMode() && m_osu_mod_fposu_ref->getBool()) m_fposu->update();

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
                    (osu_skip_intro_enabled.getBool() && getSelectedBeatmap()->getHitObjectIndexForCurrentTime() < 1);
                bool can_skip_break =
                    (osu_skip_breaks_enabled.getBool() && getSelectedBeatmap()->getHitObjectIndexForCurrentTime() > 0);
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
                if(mousePosX != m_fPrevSeekMousePosX || !osu_scrubbing_smooth.getBool()) {
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
    if(osu_resolution_enabled.getBool()) {
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

UString getModsStringForConVar(int mods) {
    UString modsString = "  ";  // double space to reset if emtpy

    // NOTE: the order here is different on purpose, to avoid name collisions during parsing (see Osu::updateMods())
    // order is the same as in ModSelector::updateModConVar()
    if(mods & ModFlags::Easy) modsString.append("ez");
    if(mods & ModFlags::HardRock) modsString.append("hr");
    if(mods & ModFlags::Relax) modsString.append("relax");
    if(mods & ModFlags::NoFail) modsString.append("nf");
    if(mods & ModFlags::SuddenDeath) modsString.append("sd");
    if(mods & ModFlags::Perfect) modsString.append("ss,");
    if(mods & ModFlags::Autopilot) modsString.append("autopilot");
    if(mods & ModFlags::HalfTime) modsString.append("ht");
    if(mods & ModFlags::DoubleTime) modsString.append("dt");
    if(mods & ModFlags::Nightcore) modsString.append("nc");
    if(mods & ModFlags::SpunOut) modsString.append("spunout");
    if(mods & ModFlags::Hidden) modsString.append("hd");
    if(mods & ModFlags::Autoplay) modsString.append("auto");
    if(mods & ModFlags::Nightmare) modsString.append("nightmare");
    if(mods & ModFlags::Target) modsString.append("practicetarget");
    if(mods & ModFlags::TouchDevice) modsString.append("nerftd");
    if(mods & ModFlags::ScoreV2) modsString.append("v2");

    return modsString;
}

bool Osu::useMods(FinishedScore *score) {
    bool nomod = (score->modsLegacy == 0);

    // legacy mods (common to all scores)
    getModSelector()->resetMods();
    osu_mods.setValue(getModsStringForConVar(score->modsLegacy));

    if(score->modsLegacy & ModFlags::FPoSu) {
        convar->getConVarByName("osu_mod_fposu")->setValue(true);
    }

    // NOTE: We don't know whether the original score was only horizontal, only vertical, or both
    if(score->isLegacyScore && score->modsLegacy & ModFlags::Mirror) {
        convar->getConVarByName("osu_playfield_mirror_horizontal")->setValue(true);
        convar->getConVarByName("osu_playfield_mirror_vertical")->setValue(true);
    }

    if(!score->isLegacyScore && !score->isImportedLegacyScore) {
        // neosu score, custom values for everything possible, have to calculate and check whether to apply any
        // overrides (or leave default)
        // reason being that just because the speedMultiplier stored in the score = 1.5x doesn't mean that we should
        // move the override slider to 1.5x especially for CS/AR/OD/HP, because those get stored in the score as
        // directly coming from Beatmap::getAR() (so with pre-applied difficultyMultiplier etc.)

        // overrides

        // NOTE: if the beatmap is loaded (in db), then use the raw base values from there, otherwise trust potentially
        // incorrect stored values from score (see explanation above)
        float tempAR = score->AR;
        float tempCS = score->CS;
        float tempOD = score->OD;
        float tempHP = score->HP;
        const DatabaseBeatmap *diff2 = getSongBrowser()->getDatabase()->getBeatmapDifficulty(score->md5hash);
        if(diff2 != NULL) {
            tempAR = diff2->getAR();
            tempCS = diff2->getCS();
            tempOD = diff2->getOD();
            tempHP = diff2->getHP();
        }

        const Replay::BEATMAP_VALUES legacyValues =
            Replay::getBeatmapValuesForModsLegacy(score->modsLegacy, tempAR, tempCS, tempOD, tempHP);

        // beatmap values
        {
            const float beatmapValueComparisonEpsilon = 0.0001f;
            if(std::abs(legacyValues.AR - score->AR) >= beatmapValueComparisonEpsilon) {
                convar->getConVarByName("osu_ar_override")->setValue(score->AR);
                nomod = false;
            }
            if(std::abs(legacyValues.CS - score->CS) >= beatmapValueComparisonEpsilon) {
                convar->getConVarByName("osu_cs_override")->setValue(score->CS);
                nomod = false;
            }
            if(std::abs(legacyValues.OD - score->OD) >= beatmapValueComparisonEpsilon) {
                convar->getConVarByName("osu_od_override")->setValue(score->OD);
                nomod = false;
            }
            if(std::abs(legacyValues.HP - score->HP) >= beatmapValueComparisonEpsilon) {
                convar->getConVarByName("osu_hp_override")->setValue(score->HP);
                nomod = false;
            }
        }

        // speed multiplier
        {
            const float speedMultiplierComparisonEpsilon = 0.0001f;
            if(std::abs(legacyValues.speedMultiplier - score->speedMultiplier) >= speedMultiplierComparisonEpsilon) {
                osu_speed_override.setValue(score->speedMultiplier);
                nomod = false;
            }
        }

        // experimental mods
        {
            auto cv = UString(score->experimentalModsConVars.c_str());
            const std::vector<UString> experimentalMods = cv.split(";");
            for(size_t i = 0; i < experimentalMods.size(); i++) {
                if(experimentalMods[i] == UString("")) continue;

                ConVar *cvar = convar->getConVarByName(experimentalMods[i], false);
                if(cvar != NULL) {
                    cvar->setValue(1.0f);  // enable experimental mod (true, 1.0f)
                    nomod = false;
                } else {
                    debugLog("couldn't find \"%s\"\n", experimentalMods[i].toUtf8());
                }
            }
        }
    }

    return nomod;
}

void Osu::updateMods() {
    if(bancho.is_in_a_multi_room()) {
        m_bModNC = bancho.room.mods & ModFlags::Nightcore;
        if(m_bModNC) {
            m_bModDT = false;
        } else if(bancho.room.mods & ModFlags::DoubleTime) {
            m_bModDT = true;
        }

        m_bModHT = (bancho.room.mods & ModFlags::HalfTime);
        m_bModDC = false;
        if(m_bModHT && bancho.prefer_daycore) {
            m_bModHT = false;
            m_bModDC = true;
        }

        m_bModNF = bancho.room.mods & ModFlags::NoFail;
        m_bModEZ = bancho.room.mods & ModFlags::Easy;
        m_bModTD = bancho.room.mods & ModFlags::TouchDevice;
        m_bModHD = bancho.room.mods & ModFlags::Hidden;
        m_bModHR = bancho.room.mods & ModFlags::HardRock;
        m_bModSD = bancho.room.mods & ModFlags::SuddenDeath;
        m_bModRelax = bancho.room.mods & ModFlags::Relax;
        m_bModAuto = bancho.room.mods & ModFlags::Autoplay;
        m_bModSpunout = bancho.room.mods & ModFlags::SpunOut;
        m_bModAutopilot = bancho.room.mods & ModFlags::Autopilot;
        m_bModSS = bancho.room.mods & ModFlags::Perfect;
        m_bModTarget = bancho.room.mods & ModFlags::Target;
        m_bModScorev2 = bancho.room.win_condition == SCOREV2;
        m_bModFlashlight = bancho.room.mods & ModFlags::Flashlight;
        m_bModNightmare = false;

        if(bancho.room.freemods) {
            for(int i = 0; i < 16; i++) {
                if(bancho.room.slots[i].player_id != bancho.user_id) continue;

                m_bModNF = bancho.room.slots[i].mods & ModFlags::NoFail;
                m_bModEZ = bancho.room.slots[i].mods & ModFlags::Easy;
                m_bModTD = bancho.room.slots[i].mods & ModFlags::TouchDevice;
                m_bModHD = bancho.room.slots[i].mods & ModFlags::Hidden;
                m_bModHR = bancho.room.slots[i].mods & ModFlags::HardRock;
                m_bModSD = bancho.room.slots[i].mods & ModFlags::SuddenDeath;
                m_bModRelax = bancho.room.slots[i].mods & ModFlags::Relax;
                m_bModAuto = bancho.room.slots[i].mods & ModFlags::Autoplay;
                m_bModSpunout = bancho.room.slots[i].mods & ModFlags::SpunOut;
                m_bModAutopilot = bancho.room.slots[i].mods & ModFlags::Autopilot;
                m_bModSS = bancho.room.slots[i].mods & ModFlags::Perfect;
                m_bModTarget = bancho.room.slots[i].mods & ModFlags::Target;
            }
        }
    } else {
        m_bModAuto = osu_mods.getString().find("auto") != -1;
        m_bModAutopilot = osu_mods.getString().find("autopilot") != -1;
        m_bModRelax = osu_mods.getString().find("relax") != -1;
        m_bModSpunout = osu_mods.getString().find("spunout") != -1;
        m_bModTarget = osu_mods.getString().find("practicetarget") != -1;
        m_bModScorev2 = osu_mods.getString().find("v2") != -1;
        m_bModFlashlight = osu_mods.getString().find("fl") != -1;
        m_bModDT = osu_mods.getString().find("dt") != -1;
        m_bModNC = osu_mods.getString().find("nc") != -1;
        m_bModNF = osu_mods.getString().find("nf") != -1;
        m_bModHT = osu_mods.getString().find("ht") != -1;
        m_bModDC = osu_mods.getString().find("dc") != -1;
        m_bModHD = osu_mods.getString().find("hd") != -1;
        m_bModHR = osu_mods.getString().find("hr") != -1;
        m_bModEZ = osu_mods.getString().find("ez") != -1;
        m_bModSD = osu_mods.getString().find("sd") != -1;
        m_bModSS = osu_mods.getString().find("ss") != -1;
        m_bModNightmare = osu_mods.getString().find("nightmare") != -1;
        m_bModTD = osu_mods.getString().find("nerftd") != -1;
    }

    // static overrides
    onSpeedChange("", osu_speed_override.getString());

    // autopilot overrides auto
    if(m_bModAutopilot) m_bModAuto = false;

    // handle auto/pilot cursor visibility
    if(isInPlayMode()) {
        m_bShouldCursorBeVisible =
            m_bModAuto || m_bModAutopilot || getSelectedBeatmap()->is_watching || getSelectedBeatmap()->is_spectating;
        env->setCursorVisible(m_bShouldCursorBeVisible);
    }

    // handle windows key disable/enable
    updateWindowsKeyDisable();

    // notify the possibly running beatmap of mod changes, for e.g. recalculating stacks dynamically if HR is toggled
    {
        getSelectedBeatmap()->onModUpdate();
        getSelectedBeatmap()->vanilla = false;  // user just cheated, prevent score submission

        if(m_songBrowser2 != NULL) {
            m_songBrowser2->recalculateStarsForSelectedBeatmap(true);
        }
    }
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

    // disable mouse buttons hotkey
    if(key == (KEYCODE)KeyBindings::DISABLE_MOUSE_BUTTONS.getInt()) {
        if(osu_disable_mousebuttons.getBool()) {
            osu_disable_mousebuttons.setValue(0.0f);
            m_notificationOverlay->addNotification("Mouse buttons are enabled.");
        } else {
            osu_disable_mousebuttons.setValue(1.0f);
            m_notificationOverlay->addNotification("Mouse buttons are disabled.");
        }
    }

    // F8 toggle chat
    if(key == (KEYCODE)KeyBindings::TOGGLE_CHAT.getInt()) {
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
    if(key == (KEYCODE)KeyBindings::SAVE_SCREENSHOT.getInt()) saveScreenshot();

    // boss key (minimize + mute)
    if(key == (KEYCODE)KeyBindings::BOSS_KEY.getInt()) {
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
            if(!key.isConsumed() && key == (KEYCODE)KeyBindings::INSTANT_REPLAY.getInt()) {
                if(!beatmap->is_watching && !beatmap->is_spectating) {
                    FinishedScore score;
                    score.isLegacyScore = false;
                    score.isImportedLegacyScore = false;
                    score.replay = beatmap->live_replay;
                    score.md5hash = beatmap->getSelectedDifficulty2()->getMD5Hash();
                    score.modsLegacy = getScore()->getModsLegacy();
                    score.speedMultiplier = getSpeedMultiplier();
                    score.CS = beatmap->getCS();
                    score.AR = beatmap->getAR();
                    score.OD = beatmap->getOD();
                    score.HP = beatmap->getHP();

                    std::vector<ConVar *> allExperimentalMods = getExperimentalMods();
                    for(int i = 0; i < allExperimentalMods.size(); i++) {
                        if(allExperimentalMods[i]->getBool()) {
                            score.experimentalModsConVars.append(allExperimentalMods[i]->getName());
                            score.experimentalModsConVars.append(";");
                        }
                    }

                    if(bancho.is_online()) {
                        score.player_id = bancho.user_id;
                        score.playerName = bancho.username.toUtf8();
                    } else {
                        auto local_name = convar->getConVarByName("name")->getString();
                        score.player_id = 0;
                        score.playerName = local_name.toUtf8();
                    }

                    double percentFinished = beatmap->getPercentFinished();
                    double duration = convar->getConVarByName("instant_replay_duration")->getFloat() * 1000.f;
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
                const bool isKeyLeftClick = (key == (KEYCODE)KeyBindings::LEFT_CLICK.getInt());
                const bool isKeyLeftClick2 = (key == (KEYCODE)KeyBindings::LEFT_CLICK_2.getInt());
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
                const bool isKeyRightClick = (key == (KEYCODE)KeyBindings::RIGHT_CLICK.getInt());
                const bool isKeyRightClick2 = (key == (KEYCODE)KeyBindings::RIGHT_CLICK_2.getInt());
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
            if(key == KEY_ENTER || key == (KEYCODE)KeyBindings::SKIP_CUTSCENE.getInt()) m_bSkipScheduled = true;

            // toggle ui
            if(!key.isConsumed() && key == (KEYCODE)KeyBindings::TOGGLE_SCOREBOARD.getInt() &&
               !m_bScoreboardToggleCheck) {
                m_bScoreboardToggleCheck = true;

                if(engine->getKeyboard()->isShiftDown()) {
                    if(!m_bUIToggleCheck) {
                        m_bUIToggleCheck = true;
                        m_osu_draw_hud_ref->setValue(!m_osu_draw_hud_ref->getBool());
                        m_notificationOverlay->addNotification(m_osu_draw_hud_ref->getBool()
                                                                   ? "In-game interface has been enabled."
                                                                   : "In-game interface has been disabled.",
                                                               0xffffffff, false, 0.1f);

                        key.consume();
                    }
                } else {
                    if(bancho.is_playing_a_multi_map()) {
                        m_osu_draw_scoreboard_mp->setValue(!m_osu_draw_scoreboard_mp->getBool());
                        m_notificationOverlay->addNotification(
                            m_osu_draw_scoreboard_mp->getBool() ? "Scoreboard is shown." : "Scoreboard is hidden.",
                            0xffffffff, false, 0.1f);
                    } else {
                        m_osu_draw_scoreboard->setValue(!m_osu_draw_scoreboard->getBool());
                        m_notificationOverlay->addNotification(
                            m_osu_draw_scoreboard->getBool() ? "Scoreboard is shown." : "Scoreboard is hidden.",
                            0xffffffff, false, 0.1f);
                    }

                    key.consume();
                }
            }

            // allow live mod changing while playing
            if(!key.isConsumed() && (key == KEY_F1 || key == (KEYCODE)KeyBindings::TOGGLE_MODSELECT.getInt()) &&
               ((KEY_F1 != (KEYCODE)KeyBindings::LEFT_CLICK.getInt() &&
                 KEY_F1 != (KEYCODE)KeyBindings::LEFT_CLICK_2.getInt()) ||
                (!m_bKeyboardKey1Down && !m_bKeyboardKey12Down)) &&
               ((KEY_F1 != (KEYCODE)KeyBindings::RIGHT_CLICK.getInt() &&
                 KEY_F1 != (KEYCODE)KeyBindings::RIGHT_CLICK_2.getInt()) ||
                (!m_bKeyboardKey2Down && !m_bKeyboardKey22Down)) &&
               !m_bF1 && !getSelectedBeatmap()->hasFailed() &&
               !bancho.is_playing_a_multi_map())  // only if not failed though
            {
                m_bF1 = true;
                toggleModSelection(true);
            }

            // quick save/load
            if(!bancho.is_playing_a_multi_map()) {
                if(key == (KEYCODE)KeyBindings::QUICK_SAVE.getInt())
                    m_fQuickSaveTime = getSelectedBeatmap()->getPercentFinished();

                if(key == (KEYCODE)KeyBindings::QUICK_LOAD.getInt()) {
                    // special case: allow cancelling the failing animation here
                    if(getSelectedBeatmap()->hasFailed()) getSelectedBeatmap()->cancelFailing();

                    getSelectedBeatmap()->seekPercent(m_fQuickSaveTime);
                }
            }

            // quick seek
            if(!bancho.is_playing_a_multi_map()) {
                const bool backward = (key == (KEYCODE)KeyBindings::SEEK_TIME_BACKWARD.getInt());
                const bool forward = (key == (KEYCODE)KeyBindings::SEEK_TIME_FORWARD.getInt());

                if(backward || forward) {
                    const unsigned long lengthMS = getSelectedBeatmap()->getLength();
                    const float percentFinished = getSelectedBeatmap()->getPercentFinished();

                    if(lengthMS > 0) {
                        double seekedPercent = 0.0;
                        if(backward)
                            seekedPercent -= (double)osu_seek_delta.getInt() * (1.0 / (double)lengthMS) * 1000.0;
                        else if(forward)
                            seekedPercent += (double)osu_seek_delta.getInt() * (1.0 / (double)lengthMS) * 1000.0;

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
        if(((key == (KEYCODE)KeyBindings::QUICK_RETRY.getInt() ||
             (engine->getKeyboard()->isControlDown() && !engine->getKeyboard()->isAltDown() && key == KEY_R)) &&
            !m_bQuickRetryDown)) {
            m_bQuickRetryDown = true;
            m_fQuickRetryTime = engine->getTime() + osu_quick_retry_delay.getFloat();
        }

        // handle seeking
        if(key == (KEYCODE)KeyBindings::SEEK_TIME.getInt()) m_bSeekKey = true;

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
            if((key == (KEYCODE)KeyBindings::GAME_PAUSE.getInt() || key == KEY_ESCAPE) && !m_bEscape) {
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
            if(key == (KEYCODE)KeyBindings::INCREASE_LOCAL_OFFSET.getInt()) {
                long offsetAdd = engine->getKeyboard()->isAltDown() ? 1 : 5;
                getSelectedBeatmap()->getSelectedDifficulty2()->setLocalOffset(
                    getSelectedBeatmap()->getSelectedDifficulty2()->getLocalOffset() + offsetAdd);
                m_notificationOverlay->addNotification(
                    UString::format("Local beatmap offset set to %ld ms",
                                    getSelectedBeatmap()->getSelectedDifficulty2()->getLocalOffset()));
            }
            if(key == (KEYCODE)KeyBindings::DECREASE_LOCAL_OFFSET.getInt()) {
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
            const bool isKeyLeftClick = (key == (KEYCODE)KeyBindings::LEFT_CLICK.getInt());
            const bool isKeyLeftClick2 = (key == (KEYCODE)KeyBindings::LEFT_CLICK_2.getInt());
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
            const bool isKeyRightClick = (key == (KEYCODE)KeyBindings::RIGHT_CLICK.getInt());
            const bool isKeyRightClick2 = (key == (KEYCODE)KeyBindings::RIGHT_CLICK_2.getInt());
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
    if(key == KEY_F1 || key == (KEYCODE)KeyBindings::TOGGLE_MODSELECT.getInt()) m_bF1 = false;
    if(key == (KEYCODE)KeyBindings::GAME_PAUSE.getInt() || key == KEY_ESCAPE) m_bEscape = false;
    if(key == KEY_SHIFT) m_bUIToggleCheck = false;
    if(key == (KEYCODE)KeyBindings::TOGGLE_SCOREBOARD.getInt()) {
        m_bScoreboardToggleCheck = false;
        m_bUIToggleCheck = false;
    }
    if(key == (KEYCODE)KeyBindings::QUICK_RETRY.getInt() || key == KEY_R) m_bQuickRetryDown = false;
    if(key == (KEYCODE)KeyBindings::SEEK_TIME.getInt()) m_bSeekKey = false;

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
    if(isInPlayMode() && !getSelectedBeatmap()->isPaused() && osu_disable_mousebuttons.getBool()) return;

    if(!m_bMouseKey1Down && down) {
        m_bMouseKey1Down = true;
        onKey1Change(true, true);
    } else if(m_bMouseKey1Down) {
        m_bMouseKey1Down = false;
        onKey1Change(false, true);
    }
}

void Osu::onRightChange(bool down) {
    if(isInPlayMode() && !getSelectedBeatmap()->isPaused() && osu_disable_mousebuttons.getBool()) return;

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

void Osu::onPlayEnd(bool quit, bool aborted) {
    m_snd_change_check_interval_ref->setValue(m_snd_change_check_interval_ref->getDefaultFloat());

    if(!quit) {
        if(!osu_mod_endless.getBool()) {
            m_rankingScreen->setScore(m_score);
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

    if(m_bModHR) difficultyMultiplier = 1.4f;
    if(m_bModEZ) difficultyMultiplier = 0.5f;

    return difficultyMultiplier;
}

float Osu::getCSDifficultyMultiplier() {
    float difficultyMultiplier = 1.0f;

    if(m_bModHR) difficultyMultiplier = 1.3f;  // different!
    if(m_bModEZ) difficultyMultiplier = 0.5f;

    return difficultyMultiplier;
}

float Osu::getScoreMultiplier() {
    float multiplier = 1.0f;

    if(m_bModEZ || (m_bModNF && !m_bModScorev2)) multiplier *= 0.50f;
    if(m_bModHT || m_bModDC) multiplier *= 0.30f;
    if(m_bModHR) {
        if(m_bModScorev2)
            multiplier *= 1.10f;
        else
            multiplier *= 1.06f;
    }
    if(m_bModDT || m_bModNC) {
        if(m_bModScorev2)
            multiplier *= 1.20f;
        else
            multiplier *= 1.12f;
    }
    if(m_bModFlashlight) multiplier *= 1.12f;
    if(m_bModHD) multiplier *= 1.06f;
    if(m_bModSpunout) multiplier *= 0.90f;

    if(m_bModRelax || m_bModAutopilot) multiplier *= 0.f;

    return multiplier;
}

float Osu::getRawSpeedMultiplier() {
    float speedMultiplier = 1.0f;

    if(m_bModDT || m_bModNC || m_bModHT || m_bModDC) {
        if(m_bModDT || m_bModNC)
            speedMultiplier = 1.5f;
        else
            speedMultiplier = 0.75f;
    }

    return speedMultiplier;
}

float Osu::getSpeedMultiplier() {
    float speedMultiplier = getRawSpeedMultiplier();

    if(osu_speed_override.getFloat() >= 0.0f) return max(osu_speed_override.getFloat(), 0.05f);

    return speedMultiplier;
}

float Osu::getAnimationSpeedMultiplier() {
    float animationSpeedMultiplier = getSpeedMultiplier();

    if(osu_animation_speed_override.getFloat() >= 0.0f) return max(osu_animation_speed_override.getFloat(), 0.05f);

    return animationSpeedMultiplier;
}

bool Osu::isInPlayMode() { return (m_songBrowser2 != NULL && m_songBrowser2->hasSelectedAndIsPlaying()); }

bool Osu::isNotInPlayModeOrPaused() { return !isInPlayMode() || getSelectedBeatmap()->isPaused(); }

bool Osu::shouldFallBackToLegacySliderRenderer() {
    return osu_force_legacy_slider_renderer.getBool() || m_osu_mod_wobble_ref->getBool() ||
           m_osu_mod_wobble2_ref->getBool() || m_osu_mod_minimize_ref->getBool() ||
           m_modSelector->isCSOverrideSliderActive()
        /* || (m_osu_playfield_rotation->getFloat() < -0.01f || m_osu_playfield_rotation->getFloat() > 0.01f)*/;
}

void Osu::onResolutionChanged(Vector2 newResolution) {
    debugLog("Osu::onResolutionChanged(%i, %i), minimized = %i\n", (int)newResolution.x, (int)newResolution.y,
             (int)engine->isMinimized());

    if(engine->isMinimized()) return;  // ignore if minimized

    const float prevUIScale = getUIScale();

    if(!osu_resolution_enabled.getBool()) {
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
            osu_resolution_enabled.setValue(0.0f);
            g_vInternalResolution = engine->getScreenSize();
        }
    }

    // update dpi specific engine globals
    m_ui_scrollview_scrollbarwidth_ref->setValue(15.0f * Osu::getUIScale());  // not happy with this as a convar

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

    if(m_osu_mod_fposu_ref->getBool())
        m_playfieldBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y);
    else
        m_playfieldBuffer->rebuild(0, 0, 64, 64);

    m_sliderFrameBuffer->rebuild(0, 0, g_vInternalResolution.x, g_vInternalResolution.y,
                                 Graphics::MULTISAMPLE_TYPE::MULTISAMPLE_0X);

    if(m_osu_mod_mafham_ref->getBool()) {
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
    if(osu_resolution_enabled.getBool()) {
        if(osu_letterboxing.getBool()) {
            // special case for osu: since letterboxed raw input absolute to window should mean the 'game' window, and
            // not the 'engine' window, no offset scaling is necessary
            if(m_mouse_raw_input_absolute_to_window_ref->getBool())
                offset = -Vector2((engine->getScreenWidth() / 2 - g_vInternalResolution.x / 2),
                                  (engine->getScreenHeight() / 2 - g_vInternalResolution.y / 2));
            else
                offset = -Vector2((engine->getScreenWidth() / 2 - g_vInternalResolution.x / 2) *
                                      (1.0f + osu_letterboxing_offset_x.getFloat()),
                                  (engine->getScreenHeight() / 2 - g_vInternalResolution.y / 2) *
                                      (1.0f + osu_letterboxing_offset_y.getFloat()));

            scale = Vector2(g_vInternalResolution.x / engine->getScreenWidth(),
                            g_vInternalResolution.y / engine->getScreenHeight());
        }
    }

    engine->getMouse()->setOffset(offset);
    engine->getMouse()->setScale(scale);
}

void Osu::updateWindowsKeyDisable() {
    if(debug->getBool()) debugLog("Osu::updateWindowsKeyDisable()\n");

    if(osu_win_disable_windows_key_while_playing.getBool()) {
        const bool isPlayerPlaying =
            engine->hasFocus() && isInPlayMode() &&
            (!getSelectedBeatmap()->isPaused() || getSelectedBeatmap()->isRestartScheduled()) && !m_bModAuto;
        m_win_disable_windows_key_ref->setValue(isPlayerPlaying ? 1.0f : 0.0f);
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
            osu_resolution_enabled.setValue(1.0f);
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
    if(isInPlayMode() && !getSelectedBeatmap()->isPaused() && osu_pause_on_focus_loss.getBool()) {
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

    if(!osu_alt_f4_quits_even_while_playing.getBool() && isInPlayMode()) {
        getSelectedBeatmap()->stop();
        return false;
    }

    // save everything
    m_optionsMenu->save();
    m_songBrowser2->getDatabase()->save();

    disconnect();

    // the only time where a shutdown could be problematic is while an update is being installed, so we block it here
    return m_updateHandler == NULL || m_updateHandler->getStatus() != UpdateHandler::STATUS::STATUS_INSTALLING_UPDATE;
}

void Osu::onSkinReload() {
    m_bSkinLoadWasReload = true;
    onSkinChange("", osu_skin.getString());
}

void Osu::onSkinChange(UString oldValue, UString newValue) {
    if(m_skin != NULL) {
        if(m_bSkinLoadScheduled || m_skinScheduledToLoad != NULL) return;
        if(newValue.length() < 1) return;
    }

    std::string neosuSkinFolder = MCENGINE_DATA_DIR "skins/";
    neosuSkinFolder.append(newValue.toUtf8());
    neosuSkinFolder.append("/");
    if(env->directoryExists(neosuSkinFolder)) {
        m_skinScheduledToLoad = new Skin(newValue, neosuSkinFolder, (newValue == UString("default")));
    } else {
        UString ppySkinFolder = m_osu_folder_ref->getString();
        ppySkinFolder.append(m_osu_folder_sub_skins_ref->getString());
        ppySkinFolder.append(newValue);
        ppySkinFolder.append("/");
        std::string sf = ppySkinFolder.toUtf8();
        m_skinScheduledToLoad = new Skin(newValue, sf, (newValue == UString("default")));
    }

    // initial load
    if(m_skin == NULL) m_skin = m_skinScheduledToLoad;

    m_bSkinLoadScheduled = true;
}

void Osu::updateAnimationSpeed() {
    if(getSkin() != NULL) {
        float speed = getAnimationSpeedMultiplier() / getSpeedMultiplier();
        getSkin()->setAnimationSpeed(speed >= 0.0f ? speed : 0.0f);
    }
}

void Osu::onAnimationSpeedChange(UString oldValue, UString newValue) { updateAnimationSpeed(); }

void Osu::onSpeedChange(UString oldValue, UString newValue) {
    float speed = newValue.toFloat();
    getSelectedBeatmap()->setSpeed(speed >= 0.0f ? speed : getSpeedMultiplier());
    updateAnimationSpeed();

    if(m_modSelector != NULL) {
        int btn_state = (getModNC() || getModDC() || bancho.prefer_daycore) ? 1 : 0;

        // nightcore_enjoyer reverses button order so NC appears first
        if(convar->getConVarByName("nightcore_enjoyer")->getBool()) {
            btn_state = (getModDT() || getModHT()) ? 1 : 0;
        }

        // Why 0.0001f you ask? See ModSelector::resetMods()
        if(speed > 0.0001f && speed < 1.0) {
            m_modSelector->m_modButtonDoubletime->setOn(false, true);
            m_modSelector->m_modButtonDoubletime->setState(btn_state, false);
            m_modSelector->m_modButtonHalftime->setOn(true, true);
            m_modSelector->m_modButtonHalftime->setState(btn_state, false);
            m_bModDT = false;
            m_bModNC = false;
            m_bModHT = m_modSelector->m_modButtonHalftime->getActiveModName() == UString("ht");
            m_bModDC = m_modSelector->m_modButtonHalftime->getActiveModName() == UString("dc");
            bancho.prefer_daycore = m_modSelector->m_modButtonHalftime->getActiveModName() == UString("dc");
        } else if(speed > 1.0) {
            m_modSelector->m_modButtonDoubletime->setOn(true, true);
            m_modSelector->m_modButtonDoubletime->setState(btn_state, false);
            m_modSelector->m_modButtonHalftime->setOn(false, true);
            m_modSelector->m_modButtonHalftime->setState(btn_state, false);
            m_bModHT = false;
            m_bModDC = false;
            m_bModDT = m_modSelector->m_modButtonDoubletime->getActiveModName() == UString("dt");
            m_bModNC = m_modSelector->m_modButtonDoubletime->getActiveModName() == UString("nc");
            bancho.prefer_daycore = m_modSelector->m_modButtonDoubletime->getActiveModName() == UString("nc");
        }
    }
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
    if(osu_resolution_enabled.getBool()) {
        bool oldVal = oldValue.toFloat() > 0.0f;
        bool newVal = newValue.toFloat() > 0.0f;

        if(oldVal != newVal) m_bFireResolutionChangedScheduled = true;  // delay
    }
}

void Osu::updateConfineCursor() {
    if(debug->getBool()) debugLog("Osu::updateConfineCursor()\n");

    if((osu_confine_cursor_fullscreen.getBool() && env->isFullscreen()) ||
       (osu_confine_cursor_windowed.getBool() && !env->isFullscreen()) ||
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
        if(!(mouse && osu_disable_mousebuttons.getBool())) {
            // quickfix
            if(osu_disable_mousebuttons.getBool()) m_bMouseKey1Down = false;

            if(pressed && isKeyPressed1Allowed && !getSelectedBeatmap()->isPaused())  // see above note
                getSelectedBeatmap()->keyPressed1(mouse);
            else if(!m_bKeyboardKey1Down && !m_bKeyboardKey12Down && !m_bMouseKey1Down)
                getSelectedBeatmap()->keyReleased1(mouse);
        }
    }

    // cursor anim + ripples
    const bool doAnimate =
        !(isInPlayMode() && !getSelectedBeatmap()->isPaused() && mouse && osu_disable_mousebuttons.getBool());
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
        if(!(mouse && osu_disable_mousebuttons.getBool())) {
            // quickfix
            if(osu_disable_mousebuttons.getBool()) m_bMouseKey2Down = false;

            if(pressed && isKeyPressed2Allowed && !getSelectedBeatmap()->isPaused())  // see above note
                getSelectedBeatmap()->keyPressed2(mouse);
            else if(!m_bKeyboardKey2Down && !m_bKeyboardKey22Down && !m_bMouseKey2Down)
                getSelectedBeatmap()->keyReleased2(mouse);
        }
    }

    // cursor anim + ripples
    const bool doAnimate =
        !(isInPlayMode() && !getSelectedBeatmap()->isPaused() && mouse && osu_disable_mousebuttons.getBool());
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

void Osu::onNotification(UString args) {
    m_notificationOverlay->addNotification(
        args, COLOR(255, osu_notification_color_r.getInt(), osu_notification_color_g.getInt(),
                    osu_notification_color_b.getInt()));
}

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
        if(osu->getScreenWidth() < osu_ui_scale_to_dpi_minimum_width.getInt() ||
           osu->getScreenHeight() < osu_ui_scale_to_dpi_minimum_height.getInt())
            return osu_ui_scale.getFloat();
    } else if(engine->getScreenWidth() < osu_ui_scale_to_dpi_minimum_width.getInt() ||
              engine->getScreenHeight() < osu_ui_scale_to_dpi_minimum_height.getInt())
        return osu_ui_scale.getFloat();

    return ((osu_ui_scale_to_dpi.getBool() ? env->getDPIScale() : 1.0f) * osu_ui_scale.getFloat());
}

bool Osu::findIgnoreCase(const std::string &haystack, const std::string &needle) {
    auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
                          [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); });

    return (it != haystack.end());
}
