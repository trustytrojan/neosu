//================ Copyright (c) 2015, PG, All rights reserved. =================//
//
// Purpose:		linux
//
// $NoKeywords: $linuxenv
//===============================================================================//

#ifdef __linux__

#ifndef LINUXENVIRONMENT_H
#define LINUXENVIRONMENT_H

#include <X11/X.h>
#include <X11/Xlib.h>

#include "Environment.h"

class LinuxEnvironment : public Environment {
   public:
    LinuxEnvironment(Display *display, Window window);
    virtual ~LinuxEnvironment();

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
    void setWindowGhostCorporeal(bool corporeal);
    void setMonitor(int monitor);
    Vector2 getWindowPos();
    Vector2 getWindowSize();
    int getMonitor();
    std::vector<McRect> getMonitors();
    Vector2 getNativeScreenSize();
    McRect getVirtualScreenRect();
    McRect getDesktopRect();
    int getDPI();
    bool isFullscreen() { return m_bFullScreen; }
    bool isWindowResizable() { return m_bResizable; }

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
    inline Display *getDisplay() const { return m_display; }
    inline Window getWindow() const { return m_window; }
    inline bool isRestartScheduled() const { return m_bIsRestartScheduled; }

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

    Display *m_display;
    Window m_window;

    // monitors
    static std::vector<McRect> m_vMonitors;

    // window
    static bool m_bResizable;
    bool m_bFullScreen;
    Vector2 m_vLastWindowPos;
    Vector2 m_vLastWindowSize;
    int m_iDPI;

    // mouse
    bool m_bCursorClipped;
    McRect m_cursorClip;
    bool m_bCursorRequest;
    bool m_bCursorReset;
    bool m_bCursorVisible;
    bool m_bIsCursorInsideWindow;
    Cursor m_mouseCursor;
    Cursor m_invisibleCursor;
    CURSORTYPE m_cursorType;

    // clipboard
    UString m_sLocalClipboardContent;
    Atom m_atom_UTF8_STRING;
    Atom m_atom_CLIPBOARD;
    Atom m_atom_TARGETS;

    // custom
    bool m_bIsRestartScheduled;
    bool m_bResizeDelayHack;
    Vector2 m_vResizeHackSize;
    bool m_bPrevCursorHack;
    bool m_bFullscreenWasResizable;
    Vector2 m_vPrevDisableFullscreenWindowSize;
};

#endif

#endif
