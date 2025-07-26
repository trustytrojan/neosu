#pragma once

#ifdef _WIN32

#include "Environment.h"
#include "cbase.h"

class WinEnvironment final : public Environment {
   public:
    static long getWindowStyleWindowed();

   public:
    WinEnvironment(HWND hwnd, HINSTANCE hinstance, const std::vector<UString> &argCmdline,
                     const std::unordered_map<UString, std::optional<UString>> &argMap);
    ~WinEnvironment() override;

    void update() override;

    // engine/factory
    Graphics *createRenderer() override;

    // system
    void shutdown() override;
    void restart() override;

    // user
    UString getUsername() override;

    // file IO
    std::vector<std::string> getFilesInFolder(const std::string &folder) noexcept override;
    std::vector<std::string> getFoldersInFolder(const std::string &folder) noexcept override;
    std::vector<UString> getLogicalDrives() override;

    // clipboard
    UString getClipBoardText() override;
    void setClipBoardText(UString text) override;

    // dialogs & message boxes
    void showMessageInfo(const UString &title, const UString &message) override;
    void showMessageWarning(const UString &title, const UString &message) override;
    void showMessageError(const UString &title, const UString &message) override;
    void showMessageErrorFatal(const UString &title, const UString &message) override;

    // window
    void focus() override;
    void center() override;
    void minimize() override;
    void maximize() override;
    void enableFullscreen() override;
    void disableFullscreen() override;
    void setWindowTitle(UString title) override;
    void setWindowPos(int x, int y) override;
    void setWindowSize(int width, int height) override;
    void setWindowResizable(bool resizable) override;
    void setMonitor(int monitor) override;
    Vector2 getWindowPos() override;
    Vector2 getWindowSize() override;
    int getMonitor() override;
    std::vector<McRect> getMonitors() override;
    Vector2 getNativeScreenSize() override;
    McRect getVirtualScreenRect() override;
    McRect getDesktopRect() override;
    int getDPI() override;
    bool isFullscreen() override { return this->bFullScreen; }
    bool isWindowResizable() override { return this->bResizable; }

    // mouse
    bool isCursorInWindow() override;
    bool isCursorVisible() override;
    bool isCursorClipped() override;
    Vector2d getMousePos() override;
    McRect getCursorClip() override;
    CURSORTYPE getCursor() override;
    void setCursor(CURSORTYPE cur) override;
    void setCursorVisible(bool visible) override;
    void setMousePos(double x, double y) override;
    void setCursorClip(bool clip, McRect rect) override;

    // keyboard
    UString keyCodeToString(KEYCODE keyCode) override;

    // ILLEGAL:
    // (also custom)
    void setDPIOverride(int newForcedDPI) { this->iDPIOverride = newForcedDPI; }
    bool setProcessPriority(int priority);  // 0 = normal, 1 = high
    bool setProcessAffinity(int affinity);  // -1 = reset (all cores), 0 = first core, 1 = last core
    void disableWindowsKey();
    void enableWindowsKey();
    [[nodiscard]] inline HWND getHwnd() const { return this->hwnd; }
    [[nodiscard]] inline HINSTANCE getHInstance() const { return this->hInstance; }
    [[nodiscard]] inline bool isRestartScheduled() const { return this->bIsRestartScheduled; }

   private:
    static BOOL CALLBACK monitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
    static LRESULT CALLBACK lowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    void path_strip_filename(TCHAR *Path);
    void handleShowMessageFullscreen();
    void enumerateMonitors();
    void onProcessPriorityChange(const UString &oldValue, const UString &newValue);
    void onDisableWindowsKeyChange(const UString &oldValue, const UString &newValue);

    HWND hwnd;
    HINSTANCE hInstance;

    // monitors
    static std::vector<McRect> vMonitors;

    // window
    static bool bResizable;
    bool bFullScreen;
    Vector2 vWindowSize;
    Vector2 vLastWindowPos;
    Vector2 vLastWindowSize;

    // mouse
    bool bCursorClipped;
    McRect cursorClip;
    bool bIsCursorInsideWindow;
    bool bHasCursorTypeChanged;
    HCURSOR mouseCursor;
    CURSORTYPE cursorType;

    // custom
    int iDPIOverride;
    bool bIsRestartScheduled;
    static int iNumCoresForProcessAffinity;
    static HHOOK hKeyboardHook;
};

using EnvironmentImpl = WinEnvironment;

#endif
