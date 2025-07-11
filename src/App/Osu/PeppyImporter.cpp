#include "ConVar.h"
#include "Engine.h"
#include "File.h"
#include "Keyboard.h"

void try_set_key(const char* str, ConVar* cvar) {
    UString key{UString(str).trim()};

    if(key == UString("None")) {
        cvar->setValue(0);
    } else if(key == UString("A")) {
        cvar->setValue((int)KEY_A);
    } else if(key == UString("B")) {
        cvar->setValue((int)KEY_B);
    } else if(key == UString("C")) {
        cvar->setValue((int)KEY_C);
    } else if(key == UString("D")) {
        cvar->setValue((int)KEY_D);
    } else if(key == UString("E")) {
        cvar->setValue((int)KEY_E);
    } else if(key == UString("F")) {
        cvar->setValue((int)KEY_F);
    } else if(key == UString("G")) {
        cvar->setValue((int)KEY_G);
    } else if(key == UString("H")) {
        cvar->setValue((int)KEY_H);
    } else if(key == UString("I")) {
        cvar->setValue((int)KEY_I);
    } else if(key == UString("J")) {
        cvar->setValue((int)KEY_J);
    } else if(key == UString("K")) {
        cvar->setValue((int)KEY_K);
    } else if(key == UString("L")) {
        cvar->setValue((int)KEY_L);
    } else if(key == UString("M")) {
        cvar->setValue((int)KEY_M);
    } else if(key == UString("N")) {
        cvar->setValue((int)KEY_N);
    } else if(key == UString("O")) {
        cvar->setValue((int)KEY_O);
    } else if(key == UString("P")) {
        cvar->setValue((int)KEY_P);
    } else if(key == UString("Q")) {
        cvar->setValue((int)KEY_Q);
    } else if(key == UString("R")) {
        cvar->setValue((int)KEY_R);
    } else if(key == UString("S")) {
        cvar->setValue((int)KEY_S);
    } else if(key == UString("T")) {
        cvar->setValue((int)KEY_T);
    } else if(key == UString("U")) {
        cvar->setValue((int)KEY_U);
    } else if(key == UString("V")) {
        cvar->setValue((int)KEY_V);
    } else if(key == UString("W")) {
        cvar->setValue((int)KEY_W);
    } else if(key == UString("X")) {
        cvar->setValue((int)KEY_X);
    } else if(key == UString("Y")) {
        cvar->setValue((int)KEY_Y);
    } else if(key == UString("Z")) {
        cvar->setValue((int)KEY_Z);
    } else if(key == UString("0")) {
        cvar->setValue((int)KEY_0);
    } else if(key == UString("1")) {
        cvar->setValue((int)KEY_1);
    } else if(key == UString("2")) {
        cvar->setValue((int)KEY_2);
    } else if(key == UString("3")) {
        cvar->setValue((int)KEY_3);
    } else if(key == UString("4")) {
        cvar->setValue((int)KEY_4);
    } else if(key == UString("5")) {
        cvar->setValue((int)KEY_5);
    } else if(key == UString("6")) {
        cvar->setValue((int)KEY_6);
    } else if(key == UString("7")) {
        cvar->setValue((int)KEY_7);
    } else if(key == UString("8")) {
        cvar->setValue((int)KEY_8);
    } else if(key == UString("9")) {
        cvar->setValue((int)KEY_9);
    } else if(key == UString("F1")) {
        cvar->setValue((int)KEY_F1);
    } else if(key == UString("F2")) {
        cvar->setValue((int)KEY_F2);
    } else if(key == UString("F3")) {
        cvar->setValue((int)KEY_F3);
    } else if(key == UString("F4")) {
        cvar->setValue((int)KEY_F4);
    } else if(key == UString("F5")) {
        cvar->setValue((int)KEY_F5);
    } else if(key == UString("F6")) {
        cvar->setValue((int)KEY_F6);
    } else if(key == UString("F7")) {
        cvar->setValue((int)KEY_F7);
    } else if(key == UString("F8")) {
        cvar->setValue((int)KEY_F8);
    } else if(key == UString("F9")) {
        cvar->setValue((int)KEY_F9);
    } else if(key == UString("F10")) {
        cvar->setValue((int)KEY_F10);
    } else if(key == UString("F11")) {
        cvar->setValue((int)KEY_F11);
    } else if(key == UString("F12")) {
        cvar->setValue((int)KEY_F12);
    } else if(key == UString("Left")) {
        cvar->setValue((int)KEY_LEFT);
    } else if(key == UString("Right")) {
        cvar->setValue((int)KEY_RIGHT);
    } else if(key == UString("Up")) {
        cvar->setValue((int)KEY_UP);
    } else if(key == UString("Down")) {
        cvar->setValue((int)KEY_DOWN);
    } else if(key == UString("Tab")) {
        cvar->setValue((int)KEY_TAB);
    } else if(key == UString("Return") || key == UString("Enter")) {
        cvar->setValue((int)KEY_ENTER);
    } else if(key == UString("Shift")) {
        cvar->setValue((int)KEY_LSHIFT);
    } else if(key == UString("Control")) {
        cvar->setValue((int)KEY_LCONTROL);
    } else if(key == UString("LeftAlt")) {
        cvar->setValue((int)KEY_LALT);
    } else if(key == UString("RightAlt")) {
        cvar->setValue((int)KEY_RALT);
    } else if(key == UString("Escape")) {
        cvar->setValue((int)KEY_ESCAPE);
    } else if(key == UString("Space")) {
        cvar->setValue((int)KEY_SPACE);
    } else if(key == UString("Back")) {
        cvar->setValue((int)KEY_BACKSPACE);
    } else if(key == UString("End")) {
        cvar->setValue((int)KEY_END);
    } else if(key == UString("Insert")) {
        cvar->setValue((int)KEY_INSERT);
    } else if(key == UString("Delete")) {
        cvar->setValue((int)KEY_DELETE);
    } else if(key == UString("Help")) {
        cvar->setValue((int)KEY_HELP);
    } else if(key == UString("Home")) {
        cvar->setValue((int)KEY_HOME);
    } else if(key == UString("Escape")) {
        cvar->setValue((int)KEY_ESCAPE);
    } else if(key == UString("PageUp")) {
        cvar->setValue((int)KEY_PAGEUP);
    } else if(key == UString("PageDown")) {
        cvar->setValue((int)KEY_PAGEDOWN);
    } else {
        debugLog("No key code found for '%s'!\n", key.toUtf8());
    }
}

static UString get_osu_folder_from_registry() {
#ifdef _WIN32
    i32 err;
    HKEY key;

    auto key_path = L"Software\\Classes\\osustable.File.osk\\Shell\\Open\\Command";
    err = RegOpenKeyExW(HKEY_CURRENT_USER, key_path, 0, KEY_READ, &key);
    if(err != ERROR_SUCCESS) {
        // Older registry key, in case the user isn't on latest osu!stable version
        // See https://osu.ppy.sh/home/changelog/cuttingedge/20250111
        err = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"osu\\shell\\open\\command", 0, KEY_READ, &key);
    }
    if(err != ERROR_SUCCESS) {
        debugLog("osu!stable not found in registry!\n");
        return "";
    }

    DWORD dwType = REG_SZ;
    WCHAR szPath[MAX_PATH];
    DWORD dwSize = sizeof(szPath);
    err = RegQueryValueExW(key, NULL, NULL, &dwType, (LPBYTE)szPath, &dwSize);
    RegCloseKey(key);
    if(err != ERROR_SUCCESS) {
        debugLog("Failed to get path of osu:// protocol handler.\n");
        return "";
    }

    // The path is in format: "C:\Path\To\osu!.exe" "%1"
    WCHAR* endQuote = wcschr(szPath + 1, L'"');
    if(endQuote) {
        *endQuote = L'\0';
    }
    WCHAR* path = szPath + 1;
    WCHAR* lastBackslash = wcsrchr(path, L'\\');
    if(lastBackslash) {
        *lastBackslash = L'\0';
        return path;
    }
#endif

    return "";
}

void import_settings_from_osu_stable() {
    UString osu_folder{cv_osu_folder.getString()};
    if(osu_folder.isWhitespaceOnly()) {
        osu_folder = get_osu_folder_from_registry();
        if(!osu_folder.isWhitespaceOnly()) {
            debugLog("Found osu! folder from registry: %s\n", osu_folder.toUtf8());
            cv_osu_folder.setValue(osu_folder);
        }
    }

    auto username = env->getUsername();
    if(username.length() == 0) {
        debugLog("Failed to get username; not going to import settings from osu!stable.\n");
        return;
    }

    std::string cfg_path = osu_folder.toUtf8();
    cfg_path.append("/osu!.");
    cfg_path.append(username.toUtf8());
    cfg_path.append(".cfg");
    File file(cfg_path);

    char str[1024];
    int num;
    f32 flt;

    while(file.canRead()) {
        std::string curLine = file.readLine();
        memset(str, '\0', 1024);
        num = 0;

        if(sscanf(curLine.c_str(), " BeatmapDirectory = %1023[^\n]", str) == 1) {
            UString BeatmapDirectory{UString(str).trim()};
            if(BeatmapDirectory.length() > 2) {
                cv_songs_folder.setValue(BeatmapDirectory);
            }
        } else if(sscanf(curLine.c_str(), " VolumeUniversal = %i[^\n]", &num) == 1) {
            cv_volume_master.setValue((f32)num / 100.f);
        } else if(sscanf(curLine.c_str(), " VolumeEffect = %i[^\n]", &num) == 1) {
            cv_volume_effects.setValue((f32)num / 100.f);
        } else if(sscanf(curLine.c_str(), " VolumeMusic = %i[^\n]", &num) == 1) {
            cv_volume_music.setValue((f32)num / 100.f);
        } else if(sscanf(curLine.c_str(), " AllowPublicInvites = %i[^\n]", &num) == 1) {
            cv_allow_mp_invites.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " AutoChatHide = %i[^\n]", &num) == 1) {
            cv_chat_auto_hide.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " BlockNonFriendPM = %i[^\n]", &num) == 1) {
            cv_allow_stranger_dms.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " ChatAudibleHighlight = %i[^\n]", &num) == 1) {
            cv_chat_ping_on_mention.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " ChatHighlightName = %i[^\n]", &num) == 1) {
            cv_chat_notify_on_mention.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " CursorSize = %f[^\n]", &flt) == 1) {
            cv_cursor_scale.setValue(flt);
        } else if(sscanf(curLine.c_str(), " AutomaticCursorSizing = %i[^\n]", &num) == 1) {
            cv_automatic_cursor_size.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " DimLevel = %i[^\n]", &num) == 1) {
            cv_background_dim.setValue((f32)num / 100.f);
        } else if(sscanf(curLine.c_str(), " IHateHavingFun = %i[^\n]", &num) == 1) {
            cv_background_dont_fade_during_breaks.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " DiscordRichPresence = %i[^\n]", &num) == 1) {
            cv_rich_presence.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " FpsCounter = %i[^\n]", &num) == 1) {
            cv_draw_fps.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " CursorRipple = %i[^\n]", &num) == 1) {
            cv_draw_cursor_ripples.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " HighlightWords = %1023[^\n]", str) == 1) {
            UString HighlightWords{UString(str).trim()};
            cv_chat_highlight_words.setValue(HighlightWords);
        } else if(sscanf(curLine.c_str(), " HighResolution = %i[^\n]", &num) == 1) {
            if(num == 1) {
                cv_skin_hd.setValue(true);
            }
        } else if(sscanf(curLine.c_str(), " IgnoreBeatmapSamples = %i[^\n]", &num) == 1) {
            cv_ignore_beatmap_samples.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " IgnoreBeatmapSkins = %i[^\n]", &num) == 1) {
            cv_ignore_beatmap_skins.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " IgnoreList = %1023[^\n]", str) == 1) {
            UString IgnoreList{UString(str).trim()};
            cv_chat_ignore_list.setValue(IgnoreList);
        } else if(sscanf(curLine.c_str(), " KeyOverlay = %i[^\n]", &num) == 1) {
            cv_draw_inputoverlay.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " Language = %1023[^\n]", str) == 1) {
            cv_language.setValue(str);
        } else if(sscanf(curLine.c_str(), " ShowInterface = %i[^\n]", &num) == 1) {
            cv_draw_hud.setValue(num == 0);
        } else if(sscanf(curLine.c_str(), " LowResolution = %i[^\n]", &num) == 1) {
            if(num == 1) {
                cv_skin_hd.setValue(false);
            }
        } else if(sscanf(curLine.c_str(), " MouseDisableButtons = %i[^\n]", &num) == 1) {
            cv_disable_mousebuttons.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " MouseDisableWheel = %i[^\n]", &num) == 1) {
            cv_disable_mousewheel.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " MouseSpeed = %f[^\n]", &flt) == 1) {
            cv_mouse_sensitivity.setValue(flt);
        } else if(sscanf(curLine.c_str(), " Offset = %i[^\n]", &num) == 1) {
            cv_universal_offset.setValue((f32)num);
        } else if(sscanf(curLine.c_str(), " ScoreMeterScale = %f[^\n]", &flt) == 1) {
            cv_hud_hiterrorbar_scale.setValue(flt);
        } else if(sscanf(curLine.c_str(), " NotifyFriends = %i[^\n]", &num) == 1) {
            cv_notify_friends.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " PopupDuringGameplay = %i[^\n]", &num) == 1) {
            cv_notify_during_gameplay.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " ScoreboardVisible = %i[^\n]", &num) == 1) {
            cv_draw_scoreboard.setValue(num == 1);
            cv_draw_scoreboard_mp.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " SongSelectThumbnails = %i[^\n]", &num) == 1) {
            cv_draw_songbrowser_thumbnails.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " ShowSpectators = %i[^\n]", &num) == 1) {
            cv_draw_spectator_list.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " ShowStoryboard = %i[^\n]", &num) == 1) {
            cv_draw_storyboard.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " Skin = %1023[^\n]", str) == 1) {
            UString Skin{UString(str).trim()};
            cv_skin.setValue(Skin);
        } else if(sscanf(curLine.c_str(), " SkinSamples = %i[^\n]", &num) == 1) {
            cv_skin_use_skin_hitsounds.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " SnakingSliders = %i[^\n]", &num) == 1) {
            cv_snaking_sliders.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " Video = %i[^\n]", &num) == 1) {
            cv_draw_video.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " RawInput = %i[^\n]", &num) == 1) {
            cv_mouse_raw_input.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " AbsoluteToOsuWindow = %i[^\n]", &num) == 1) {
            cv_mouse_raw_input_absolute_to_window.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " ConfineMouse = %1023[^\n]", str) == 1) {
            if(!strcmp(str, "Never")) {
                cv_confine_cursor_fullscreen.setValue(false);
                cv_confine_cursor_windowed.setValue(false);
            } else if(!strcmp(str, "Fullscreen")) {
                cv_confine_cursor_fullscreen.setValue(true);
                cv_confine_cursor_windowed.setValue(false);
            } else if(!strcmp(str, "Always")) {
                cv_confine_cursor_fullscreen.setValue(true);
                cv_confine_cursor_windowed.setValue(true);
            }
        } else if(sscanf(curLine.c_str(), " HiddenShowFirstApproach = %i[^\n]", &num) == 1) {
            cv_show_approach_circle_on_first_hidden_object.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " ComboColourSliderBall = %i[^\n]", &num) == 1) {
            cv_slider_ball_tint_combo_color.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " Username = %1023[^\n]", str) == 1) {
            UString Username{UString(str).trim()};
            cv_name.setValue(Username);
        } else if(sscanf(curLine.c_str(), " Letterboxing = %i[^\n]", &num) == 1) {
            cv_letterboxing.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " LetterboxPositionX = %i[^\n]", &num) == 1) {
            cv_letterboxing_offset_x.setValue((f32)num / 100.f);
        } else if(sscanf(curLine.c_str(), " LetterboxPositionY = %i[^\n]", &num) == 1) {
            cv_letterboxing_offset_y.setValue((f32)num / 100.f);
        } else if(sscanf(curLine.c_str(), " ShowUnicode = %i[^\n]", &num) == 1) {
            cv_prefer_cjk.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " Ticker = %i[^\n]", &num) == 1) {
            cv_chat_ticker.setValue(num == 1);
        } else if(sscanf(curLine.c_str(), " keyOsuLeft = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_LEFT_CLICK);
        } else if(sscanf(curLine.c_str(), " keyOsuRight = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_RIGHT_CLICK);
        } else if(sscanf(curLine.c_str(), " keyOsuSmoke = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_SMOKE);
        } else if(sscanf(curLine.c_str(), " keyPause = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_GAME_PAUSE);
        } else if(sscanf(curLine.c_str(), " keySkip = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_SKIP_CUTSCENE);
        } else if(sscanf(curLine.c_str(), " keyToggleScoreboard = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_TOGGLE_SCOREBOARD);
        } else if(sscanf(curLine.c_str(), " keyToggleChat = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_TOGGLE_CHAT);
        } else if(sscanf(curLine.c_str(), " keyToggleExtendedChat = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_TOGGLE_EXTENDED_CHAT);
        } else if(sscanf(curLine.c_str(), " keyScreenshot = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_SAVE_SCREENSHOT);
        } else if(sscanf(curLine.c_str(), " keyIncreaseAudioOffset = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_INCREASE_LOCAL_OFFSET);
        } else if(sscanf(curLine.c_str(), " keyDecreaseAudioOffset = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_DECREASE_LOCAL_OFFSET);
        } else if(sscanf(curLine.c_str(), " keyQuickRetry = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_QUICK_RETRY);
        } else if(sscanf(curLine.c_str(), " keyVolumeIncrease = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_INCREASE_VOLUME);
        } else if(sscanf(curLine.c_str(), " keyVolumeDecrease = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_DECREASE_VOLUME);
        } else if(sscanf(curLine.c_str(), " keyDisableMouseButtons = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_DISABLE_MOUSE_BUTTONS);
        } else if(sscanf(curLine.c_str(), " keyBossKey = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_BOSS_KEY);
        } else if(sscanf(curLine.c_str(), " keyEasy = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_MOD_EASY);
        } else if(sscanf(curLine.c_str(), " keyNoFail = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_MOD_NOFAIL);
        } else if(sscanf(curLine.c_str(), " keyHalfTime = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_MOD_HALFTIME);
        } else if(sscanf(curLine.c_str(), " keyHardRock = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_MOD_HARDROCK);
        } else if(sscanf(curLine.c_str(), " keySuddenDeath = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_MOD_SUDDENDEATH);
        } else if(sscanf(curLine.c_str(), " keyDoubleTime = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_MOD_DOUBLETIME);
        } else if(sscanf(curLine.c_str(), " keyHidden = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_MOD_HIDDEN);
        } else if(sscanf(curLine.c_str(), " keyFlashlight = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_MOD_FLASHLIGHT);
        } else if(sscanf(curLine.c_str(), " keyRelax = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_MOD_RELAX);
        } else if(sscanf(curLine.c_str(), " keyAutopilot = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_MOD_AUTOPILOT);
        } else if(sscanf(curLine.c_str(), " keySpunOut = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_MOD_SPUNOUT);
        } else if(sscanf(curLine.c_str(), " keyAuto = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_MOD_AUTO);
        } else if(sscanf(curLine.c_str(), " keyScoreV2 = %1023[^\n]", str) == 1) {
            try_set_key(str, &cv_MOD_SCOREV2);
        }
    }
}
