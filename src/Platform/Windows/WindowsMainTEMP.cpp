// these methods need to be factored out somehow to not be 100% tied to the windows main loop, putting them here to
// avoid bloating WindowsMain.cpp
#ifdef _WIN32
#include "WindowsMain.h"

#include "WinDebloatDefs.h"
#include <objbase.h>

#include "Engine.h"
#include "Database.h"
#include "DatabaseBeatmap.h"
#include "Downloader.h"  // for extract_beatmapset
#include "File.h"
#include "MainMenu.h"
#include "OptionsMenu.h"
#include "Osu.h"
#include "Skin.h"
#include "SongBrowser/SongBrowser.h"

void WindowsMain::handle_osk(const char *osk_path) {
    Skin::unpack(osk_path);

    auto folder_name = env->getFileNameFromFilePath(osk_path);
    folder_name.erase(folder_name.size() - 4);  // remove .osk extension

    cv::skin.setValue(env->getFileNameFromFilePath(folder_name).c_str());
    osu->optionsMenu->updateSkinNameLabel();
}

void WindowsMain::handle_osz(const char *osz_path) {
    File osz(osz_path);
    i32 set_id = Downloader::extract_beatmapset_id(reinterpret_cast<const u8 *>(osz.readFile()), osz.getFileSize());
    if(set_id < 0) {
        // special case: legacy fallback behavior for invalid beatmapSetID, try to parse the ID from the
        // path
        auto mapset_name = UString(env->getFileNameFromFilePath(osz_path).c_str());
        const std::vector<UString> tokens = mapset_name.split(" ");
        for(auto token : tokens) {
            i32 id = token.toInt();
            if(id > 0) set_id = id;
        }
    }
    if(set_id == -1) {
        osu->getNotificationOverlay()->addToast("Beatmapset doesn't have a valid ID.");
        return;
    }

    std::string mapset_dir = MCENGINE_DATA_DIR "maps\\";
    mapset_dir.append(std::to_string(set_id));
    mapset_dir.append("\\");
    if(!Environment::directoryExists(mapset_dir)) {
        env->createDirectory(mapset_dir);
    }
    if(!Downloader::extract_beatmapset(reinterpret_cast<const u8 *>(osz.readFile()), osz.getFileSize(), mapset_dir)) {
        osu->getNotificationOverlay()->addToast("Failed to extract beatmapset");
        return;
    }

    db->addBeatmapSet(mapset_dir);
    if(!osu->getSongBrowser()->selectBeatmapset(set_id)) {
        osu->getNotificationOverlay()->addToast("Failed to import beatmapset");
        return;
    }

    // prevent song browser from picking main menu song after database loads
    // (we just loaded and selected another song, so previous no longer applies)
    SAFE_DELETE(osu->mainMenu->preloaded_beatmapset);
}

void WindowsMain::handle_neosu_url(const char *url) {
    if(strstr(url, "neosu://login/") == url) {
        // Disable autologin, in case there's an error while logging in
        // Will be reenabled after the login succeeds
        cv::mp_autologin.setValue(false);

        osu->getOptionsMenu()->setLoginLoadingState(true);

        // TODO @kiwec: make request to https://neosu.net/oauth/btoken with bancho.oauth_verifier + the code in
        //              neosu://login/<code> to get access token and refresh token
        //              then use c.neosu.net with access token
        //              OR, replace existing c. login packet?

        // TODO @kiwec: set bancho.username, cv_name (cv_name accessed in many places!)

        return;
    }

    if(!strcmp(url, "neosu://run")) {
        // nothing to do
        return;
    }

    if(strstr(url, "neosu://join_lobby/") == url) {
        // TODO @kiwec: lobby id
        return;
    }

    if(strstr(url, "neosu://select_map/") == url) {
        // TODO @kiwec: beatmapset + md5 combo
        return;
    }

    if(strstr(url, "neosu://spectate/") == url) {
        // TODO @kiwec: user id
        return;
    }

    if(strstr(url, "neosu://watch_replay/") == url) {
        // TODO @kiwec: replay md5
        return;
    }
}

void WindowsMain::handle_cmdline_args(const char *args) {
    if(args[0] == '-') return;
    if(strlen(args) < 4) return;

    if(strstr(args, "neosu://") == args) {
        handle_neosu_url(args);
    } else {
        auto extension = env->getFileExtensionFromFilePath(args);
        if(!extension.compare("osz")) {
            handle_osz(args);
        } else if(!extension.compare("osk") || !extension.compare("zip")) {
            handle_osk(args);
        }
    }
}

void WindowsMain::register_neosu_file_associations() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    // Register neosu as an application
    HKEY neosu_key;
    i32 err = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\neosu", 0, NULL, REG_OPTION_NON_VOLATILE,
                              KEY_WRITE, NULL, &neosu_key, NULL);
    if(err != ERROR_SUCCESS) {
        debugLog("Failed to register neosu as an application. Error: %d (root)\n", err);
        return;
    }
    RegSetValueExW(neosu_key, L"", 0, REG_SZ, (BYTE *)L"neosu", 12);
    RegSetValueExW(neosu_key, L"URL Protocol", 0, REG_SZ, (BYTE *)L"", 2);

    HKEY app_key;
    err = RegCreateKeyExW(neosu_key, L"Application", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &app_key, NULL);
    if(err != ERROR_SUCCESS) {
        debugLog("Failed to register neosu as an application. Error: %d (app)\n", err);
        RegCloseKey(neosu_key);
        return;
    }
    RegSetValueExW(app_key, L"ApplicationName", 0, REG_SZ, (BYTE *)L"neosu", 12);
    RegCloseKey(app_key);

    HKEY cmd_key;
    wchar_t command[MAX_PATH + 10];
    swprintf_s(command, _countof(command), L"\"%s\" \"%%1\"", exePath);
    err = RegCreateKeyExW(neosu_key, L"shell\\open\\command", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                          &cmd_key, NULL);
    if(err != ERROR_SUCCESS) {
        debugLog("Failed to register neosu as an application. Error: %d (command)\n", err);
        RegCloseKey(neosu_key);
        return;
    }
    RegSetValueExW(cmd_key, L"", 0, REG_SZ, (BYTE *)command, (wcslen(command) + 1) * sizeof(wchar_t));
    RegCloseKey(cmd_key);

    RegCloseKey(neosu_key);

    // Register neosu as .osk handler
    HKEY osk_key;
    err = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Classes\\.osk\\OpenWithProgids"), 0, NULL,
                         REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &osk_key, NULL);
    if(err != ERROR_SUCCESS) {
        debugLog("Failed to register neosu as .osk format handler. Error: %d\n", err);
        return;
    }
    RegSetValueEx(osk_key, TEXT("neosu"), 0, REG_SZ, (BYTE *)L"", 2);
    RegCloseKey(osk_key);

    // Register neosu as .osr handler
    HKEY osr_key;
    err = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Classes\\.osr\\OpenWithProgids"), 0, NULL,
                         REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &osr_key, NULL);
    if(err != ERROR_SUCCESS) {
        debugLog("Failed to register neosu as .osr format handler. Error: %d\n", err);
        return;
    }
    RegSetValueEx(osr_key, TEXT("neosu"), 0, REG_SZ, (BYTE *)L"", 2);
    RegCloseKey(osr_key);

    // Register neosu as .osz handler
    HKEY osz_key;
    err = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Classes\\.osz\\OpenWithProgids"), 0, NULL,
                         REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &osz_key, NULL);
    if(err != ERROR_SUCCESS) {
        debugLog("Failed to register neosu as .osz format handler. Error: %d\n", err);
        return;
    }
    RegSetValueEx(osz_key, TEXT("neosu"), 0, REG_SZ, (BYTE *)L"", 2);
    RegCloseKey(osz_key);
}

#endif
