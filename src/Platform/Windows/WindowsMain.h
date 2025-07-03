#pragma once

#ifdef _WIN32

#include "WinEnvironment.h"

class Engine;

class WindowsMain {
   public:
    WindowsMain(int argc, char *argv[], const std::vector<UString> &argCmdline,
                const std::unordered_map<UString, std::optional<UString>> &argMap);
    static int ret;

    // I L L E G A L (sorry, that was the inner McKay in me)
    void handle_cmdline_args(const char *args);

   private:
    bool bRunning{true};
    bool bUpdate{true};
    bool bDraw{true};

    bool bMinimized{false};                       // for fps_max_background
    bool bHasFocus{false};                        // for fps_max_background
    bool bIsCursorVisible{true};                  // local variable
    bool bSupportsPerMonitorDpiAwareness{false};  // local variable

    HWND createWinWindow(HINSTANCE hInstance);

    static LRESULT CALLBACK wndProcWrapper(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK realWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // misc helpers
    void handle_osk(const char *osk_path);
    void handle_osz(const char *osz_path);
    void handle_neosu_url(const char *url);
    void register_neosu_file_associations();

    WPARAM mapLeftRightKeys(WPARAM wParam, LPARAM lParam);

    HRESULT doCoInitialize();
    void doCoUninitialize();
    FARPROC doLoadComBaseFunction(const char *name);
};

using Main = WindowsMain;

extern EnvironmentImpl *baseEnv;
extern Main *mainloopPtrHack;

#endif
