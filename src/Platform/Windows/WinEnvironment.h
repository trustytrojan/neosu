#pragma once

#ifdef _WIN32

#include "Environment.h"
#include "cbase.h"

class WinEnvironment : public Environment {
   public:
    static long getWindowStyleWindowed();

   public:
    WinEnvironment(HWND hwnd, HINSTANCE hinstance);
    virtual ~WinEnvironment();

    void update();

    // engine/factory
    Graphics *createRenderer();

    // system
    void shutdown();
    void restart();
    void openURLInDefaultBrowser(UString url);
    void openDirectory(std::string path);

    // user
    UString getUsername();

    // file IO
    std::vector<std::string> getFilesInFolder(const std::string &folder) noexcept override;
    std::vector<std::string> getFoldersInFolder(const std::string &folder) noexcept override;
    std::vector<UString> getLogicalDrives();

    // clipboard
    UString getClipBoardText();
    void setClipBoardText(UString text);

    // dialogs & message boxes
    void showMessageInfo(UString title, UString message);
    void showMessageWarning(UString title, UString message);
    void showMessageError(UString title, UString message);
    void showMessageErrorFatal(UString title, UString message);
    UString openFileWindow(const char *filetypefilters, UString title, UString initialpath);
    UString openFolderWindow(UString title, UString initialpath);

    // window
    void focus();
    void center();
    void minimize();
    void maximize();
    void enableFullscreen();
    void disableFullscreen();
    void setWindowTitle(UString title);
    void setWindowPos(int x, int y);
    void setWindowSize(int width, int height);
    void setWindowResizable(bool resizable);
    void setMonitor(int monitor);
    Vector2 getWindowPos();
    Vector2 getWindowSize();
    int getMonitor();
    std::vector<McRect> getMonitors();
    Vector2 getNativeScreenSize();
    McRect getVirtualScreenRect();
    McRect getDesktopRect();
    int getDPI();
    bool isFullscreen() { return this->bFullScreen; }
    bool isWindowResizable() { return this->bResizable; }

    // mouse
    bool isCursorInWindow();
    bool isCursorVisible();
    bool isCursorClipped();
    Vector2d getMousePos();
    McRect getCursorClip();
    CURSORTYPE getCursor();
    void setCursor(CURSORTYPE cur);
    void setCursorVisible(bool visible);
    void setMousePos(double x, double y);
    void setCursorClip(bool clip, McRect rect);

    // keyboard
    UString keyCodeToString(KEYCODE keyCode);

    // ILLEGAL:
    // (also custom)
    void setDPIOverride(int newForcedDPI) { this->iDPIOverride = newForcedDPI; }
    bool setProcessPriority(int priority);  // 0 = normal, 1 = high
    bool setProcessAffinity(int affinity);  // -1 = reset (all cores), 0 = first core, 1 = last core
    void disableWindowsKey();
    void enableWindowsKey();
    inline HWND getHwnd() const { return this->hwnd; }
    inline HINSTANCE getHInstance() const { return this->hInstance; }
    inline bool isRestartScheduled() const { return this->bIsRestartScheduled; }

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
