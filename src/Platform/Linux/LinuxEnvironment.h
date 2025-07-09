#pragma once

#ifdef __linux__

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>

#include "Environment.h"

class LinuxEnvironment : public Environment {
   public:
    LinuxEnvironment(Display *display, Window window);
    ~LinuxEnvironment() override;

    void update() override;

    // engine/factory
    Graphics *createRenderer() override;

    // system
    void shutdown() override;
    void restart() override;
    std::string getExecutablePath() override;
    void openURLInDefaultBrowser(UString url) override;
    void openDirectory(std::string path) override;

    // user
    UString getUsername() override;
    std::string getUserDataPath() override;

    // file IO
    bool createDirectory(std::string directoryName) override;
    bool renameFile(std::string oldFileName, std::string newFileName) override;
    bool deleteFile(std::string filePath) override;
    std::vector<std::string> getFilesInFolder(std::string folder) override;
    std::vector<std::string> getFoldersInFolder(std::string folder) override;
    std::vector<UString> getLogicalDrives() override;
    std::string getFolderFromFilePath(std::string filepath) override;
    std::string getFileExtensionFromFilePath(std::string filepath, bool includeDot = false) override;
    std::string getFileNameFromFilePath(std::string filePath) override;

    // clipboard
    UString getClipBoardText() override;
    void setClipBoardText(UString text) override;

    // dialogs & message boxes
    void showMessageInfo(UString title, UString message) override;
    void showMessageWarning(UString title, UString message) override;
    void showMessageError(UString title, UString message) override;
    void showMessageErrorFatal(UString title, UString message) override;
    UString openFileWindow(const char *filetypefilters, UString title, UString initialpath) override;
    UString openFolderWindow(UString title, UString initialpath) override;

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
    [[nodiscard]] inline Display *getDisplay() const { return this->display; }
    [[nodiscard]] inline Window getWindow() const { return this->window; }
    [[nodiscard]] inline bool isRestartScheduled() const { return this->bIsRestartScheduled; }

    inline void updateMousePos(double x, double y) {
        this->vCachedMousePos = Vector2d(x, y);
        this->bMousePosValid = true;
    }
    inline void invalidateMousePos() { this->bMousePosValid = false; }

    void handleSelectionRequest(XSelectionRequestEvent &evt);

   private:
    static int getFilesInFolderFilter(const struct dirent *entry);
    static int getFoldersInFolderFilter(const struct dirent *entry);

    void setWindowResizableInt(bool resizable, Vector2 windowSize);
    Vector2 getWindowSizeServer();

    Cursor makeBlankCursor();
    void setCursorInt(Cursor cursor);

    UString readWindowProperty(Window window, Atom prop, Atom fmt /* XA_STRING or UTF8_STRING */,
                               bool deleteAfterReading);
    bool requestSelectionContent(UString &selection_content, Atom selection, Atom requested_format);
    void setClipBoardTextInt(UString clipText);
    UString getClipboardTextInt();

    Display *display;
    Window window;

    // monitors
    static std::vector<McRect> vMonitors;

    // window
    static bool bResizable;
    bool bFullScreen;
    Vector2 vLastWindowPos;
    Vector2 vLastWindowSize;
    int iDPI;

    // mouse
    bool bCursorClipped;
    McRect cursorClip;
    bool bCursorRequest;
    bool bCursorReset;
    bool bCursorVisible;
    bool bIsCursorInsideWindow;
    Cursor mouseCursor;
    Cursor invisibleCursor;
    CURSORTYPE cursorType;
    Vector2d vCachedMousePos;
    bool bMousePosValid;

    // clipboard
    UString sLocalClipboardContent;
    Atom atom_UTF8_STRING;
    Atom atom_CLIPBOARD;
    Atom atom_TARGETS;

    // custom
    bool bIsRestartScheduled;
    bool bResizeDelayHack;
    Vector2 vResizeHackSize;
    bool bPrevCursorHack;
    bool bFullscreenWasResizable;
    Vector2 vPrevDisableFullscreenWindowSize;
};

using EnvironmentImpl = LinuxEnvironment;

#endif

std::string fix_filename_casing(const std::string& directory, std::string filename);
