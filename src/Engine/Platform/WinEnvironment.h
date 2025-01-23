#pragma once

#ifdef _WIN32

#include "Environment.h"
#include "cbase.h"

// defined in main_Windows.cpp
void handle_cmdline_args(const char *args);

class WinEnvironment : public Environment {
   public:
    static long getWindowStyleWindowed();

   public:
    WinEnvironment(HWND hwnd, HINSTANCE hinstance);
    virtual ~WinEnvironment();

    void update();

    // engine/factory
    Graphics *createRenderer();
    ContextMenu *createContextMenu();

    // system
    OS getOS();
    void shutdown();
    void restart();
    void sleep(unsigned int us);
    std::string getExecutablePath();
    void openURLInDefaultBrowser(UString url);
    void openDirectory(std::string path);

    // user
    UString getUsername();
    std::string getUserDataPath();

    // file IO
    bool fileExists(std::string filename);
    bool directoryExists(std::string directoryName);
    bool createDirectory(std::string directoryName);
    bool renameFile(std::string oldFileName, std::string newFileName);
    bool deleteFile(std::string filePath);
    std::vector<std::string> getFilesInFolder(std::string folder);
    std::vector<std::string> getFoldersInFolder(std::string folder);
    std::vector<UString> getLogicalDrives();
    std::string getFolderFromFilePath(std::string filepath);
    std::string getFileExtensionFromFilePath(std::string filepath, bool includeDot = false);
    std::string getFileNameFromFilePath(std::string filePath);

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
    Vector2 getMousePos();
    McRect getCursorClip();
    CURSORTYPE getCursor();
    void setCursor(CURSORTYPE cur);
    void setCursorVisible(bool visible);
    void setMousePos(int x, int y);
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
    void onProcessPriorityChange(UString oldValue, UString newValue);
    void onDisableWindowsKeyChange(UString oldValue, UString newValue);

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

#endif
