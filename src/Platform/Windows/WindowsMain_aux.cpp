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

#include "dynutils.h"

extern BOOL WINAPI pImmDisableIME(DWORD);
enum PROCESS_DPI_AWARENESS {};  // NOLINT
extern HRESULT WINAPI pSetProcessDpiAwareness(PROCESS_DPI_AWARENESS);
extern WINUSERAPI BOOL WINAPI pEnableNonClientDpiScaling(HWND);

using namespace dynutils;
namespace {  // static

bool s_pmdpia_supported;  // checked in wndproc on creation (WM_NCCREATE) for enabling
                          // non-client dpi scaling, by setup_dpi_late

lib_obj *combase_handle{nullptr};
bool combase_loaded{false};

bool setup_dpi_win81() {
    HRESULT result = E_FAIL;

    auto *shcore_handle =
        reinterpret_cast<lib_obj *>(LoadLibraryEx(TEXT("shcore.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));
    if(shcore_handle) {
        auto spdpi_awareness_func = load_func<pSetProcessDpiAwareness>(shcore_handle, "SetProcessDpiAwareness");
        if(spdpi_awareness_func) {
            result = spdpi_awareness_func((PROCESS_DPI_AWARENESS)2);  // 2 == PROCESS_PER_MONITOR_DPI_AWARE
        }
        unload_lib(shcore_handle);
    }

    s_pmdpia_supported = (result == S_OK || result == E_ACCESSDENIED);

    return s_pmdpia_supported;
}

void setup_dpi_vista() {
    auto *user32_handle = reinterpret_cast<lib_obj *>(GetModuleHandle(TEXT("user32.dll")));
    if(user32_handle) {
        auto spdpi_aware_func = load_func<SetProcessDPIAware>(user32_handle, "SetProcessDPIAware");
        if(spdpi_aware_func) {
            spdpi_aware_func();
        }
    }
}

}  // namespace

// these were pulled straight out of SDL3 (zlib licensed)
HRESULT WindowsMain::Setup::com_init() {
    HRESULT hr = E_FAIL;

    if(!combase_loaded) {
        combase_handle =
            reinterpret_cast<lib_obj *>(LoadLibraryEx(TEXT("combase.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));
        combase_loaded = true;
    }

    auto coinit_func = load_func<CoInitializeEx>(combase_handle, "CoInitializeEx");

    if(coinit_func) {
        hr = coinit_func(nullptr, COINIT_APARTMENTTHREADED);
        if(hr == RPC_E_CHANGED_MODE) {
            hr = coinit_func(nullptr, COINIT_MULTITHREADED);
        }
        if(hr == S_FALSE) {
            return S_OK;
        }
    }

    return hr;
}

void WindowsMain::Setup::com_uninit(void) {
    if(combase_loaded) {
        auto couninit_func = load_func<CoUninitialize>(combase_handle, "CoUninitialize");
        if(couninit_func) {
            couninit_func();
        }
        unload_lib(combase_handle);
    }
}

void WindowsMain::Setup::disable_ime() {
    auto *imm32_handle =
        reinterpret_cast<lib_obj *>(LoadLibraryEx(TEXT("imm32.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));
    if(imm32_handle) {
        auto disable_ime_func = load_func<pImmDisableIME>(imm32_handle, "ImmDisableIME");
        if(disable_ime_func) disable_ime_func(-1);
        unload_lib(imm32_handle);
    }
}

void WindowsMain::Setup::dpi_early() {
    // Windows 8.1+
    // per-monitor dpi scaling
    if(!setup_dpi_win81()) {
        // Windows Vista+
        // system-wide dpi scaling
        setup_dpi_vista();
    }
}

// done in WM_NCCREATE
void WindowsMain::Setup::dpi_late(HWND hwnd) {
    if(s_pmdpia_supported) {
        auto *user32_handle = reinterpret_cast<lib_obj *>(GetModuleHandle(TEXT("user32.dll")));
        if(user32_handle) {
            auto encdpiscale_func = load_func<pEnableNonClientDpiScaling>(user32_handle, "EnableNonClientDpiScaling");
            if(encdpiscale_func) {
                encdpiscale_func(hwnd);
            }
        }
    }
}

// drag-drop/file associations/registry stuff below
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
        for(const auto &token : tokens) {
            i32 id = token.toInt();
            if(id > 0) set_id = id;
        }
    }
    if(set_id == -1) {
        osu->notificationOverlay->addToast("Beatmapset doesn't have a valid ID.", ERROR_TOAST);
        return;
    }

    std::string mapset_dir = MCENGINE_DATA_DIR "maps\\";
    mapset_dir.append(std::to_string(set_id));
    mapset_dir.append("\\");
    if(!Environment::directoryExists(mapset_dir)) {
        env->createDirectory(mapset_dir);
    }
    if(!Downloader::extract_beatmapset(reinterpret_cast<const u8 *>(osz.readFile()), osz.getFileSize(), mapset_dir)) {
        osu->notificationOverlay->addToast("Failed to extract beatmapset", ERROR_TOAST);
        return;
    }

    db->addBeatmapSet(mapset_dir);
    if(!osu->getSongBrowser()->selectBeatmapset(set_id)) {
        osu->notificationOverlay->addToast("Failed to import beatmapset", ERROR_TOAST);
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

void WindowsMain::handle_cmdline_args(std::vector<std::string> args) {
    bool need_to_reload_database = false;

    for(auto &arg : args) {
        if(arg[0] == '-') return;
        if(arg.length() < 4) return;

        if(strstr(arg.c_str(), "neosu://") == arg.c_str()) {
            handle_neosu_url(arg.c_str());
        } else {
            auto extension = env->getFileExtensionFromFilePath(arg);
            if(!extension.compare("osz")) {
                // NOTE: we're assuming db is loaded here?
                handle_osz(arg.c_str());
            } else if(!extension.compare("osk") || !extension.compare("zip")) {
                handle_osk(arg.c_str());
            } else if(!extension.compare("db") && !db->isLoading()) {
                db->dbPathsToImport.push_back(arg);
                need_to_reload_database = true;
            }
        }
    }

    if(need_to_reload_database) {
        osu->getSongBrowser()->refreshBeatmaps();
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
        debugLog("Failed to register neosu as an application. Error: {:d} (root)\n", err);
        return;
    }
    RegSetValueExW(neosu_key, L"", 0, REG_SZ, (BYTE *)L"neosu", 12);
    RegSetValueExW(neosu_key, L"URL Protocol", 0, REG_SZ, (BYTE *)L"", 2);

    HKEY app_key;
    err = RegCreateKeyExW(neosu_key, L"Application", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &app_key, NULL);
    if(err != ERROR_SUCCESS) {
        debugLog("Failed to register neosu as an application. Error: {:d} (app)\n", err);
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
        debugLog("Failed to register neosu as an application. Error: {:d} (command)\n", err);
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
        debugLog("Failed to register neosu as .osk format handler. Error: {:d}\n", err);
        return;
    }
    RegSetValueEx(osk_key, TEXT("neosu"), 0, REG_SZ, (BYTE *)L"", 2);
    RegCloseKey(osk_key);

    // Register neosu as .osr handler
    HKEY osr_key;
    err = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Classes\\.osr\\OpenWithProgids"), 0, NULL,
                         REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &osr_key, NULL);
    if(err != ERROR_SUCCESS) {
        debugLog("Failed to register neosu as .osr format handler. Error: {:d}\n", err);
        return;
    }
    RegSetValueEx(osr_key, TEXT("neosu"), 0, REG_SZ, (BYTE *)L"", 2);
    RegCloseKey(osr_key);

    // Register neosu as .osz handler
    HKEY osz_key;
    err = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Classes\\.osz\\OpenWithProgids"), 0, NULL,
                         REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &osz_key, NULL);
    if(err != ERROR_SUCCESS) {
        debugLog("Failed to register neosu as .osz format handler. Error: {:d}\n", err);
        return;
    }
    RegSetValueEx(osz_key, TEXT("neosu"), 0, REG_SZ, (BYTE *)L"", 2);
    RegCloseKey(osz_key);
}

#endif
