//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		empty environment, for debugging and new OS implementations
//
// $NoKeywords: $ne
//===============================================================================//

#ifndef NULLENVIRONMENT_H
#define NULLENVIRONMENT_H

#include "Environment.h"

class NullEnvironment : public Environment {
   public:
    NullEnvironment();
    virtual ~NullEnvironment() { ; }

    // engine/factory
    Graphics *createRenderer();
    ContextMenu *createContextMenu();

    // system
    OS getOS() { return Environment::OS::NONE; }
    void shutdown();
    void restart();
    void sleep(unsigned int us) { (void)us; }
    std::string getExecutablePath() { return ""; }
    void openURLInDefaultBrowser(UString url) { (void)url; }

    // user
    UString getUsername() { return "<NULL>"; }
    std::string getUserDataPath() { return "<NULL>"; }

    // file IO
    bool fileExists(std::string filename) {
        (void)filename;
        return false;
    }
    bool directoryExists(std::string directoryName) {
        (void)directoryName;
        return false;
    }
    bool createDirectory(std::string directoryName) {
        (void)directoryName;
        return false;
    }
    bool renameFile(std::string oldFileName, std::string newFileName) {
        (void)oldFileName;
        (void)newFileName;
        return false;
    }
    bool deleteFile(std::string filePath) {
        (void)filePath;
        return false;
    }
    std::vector<std::string> getFilesInFolder(std::string folder) {
        (void)folder;
        return std::vector<std::string>();
    }
    std::vector<std::string> getFoldersInFolder(std::string folder) {
        (void)folder;
        return std::vector<std::string>();
    }
    std::vector<UString> getLogicalDrives() { return std::vector<UString>(); }
    std::string getFolderFromFilePath(std::string filepath) {
        (void)filepath;
        return "";
    }
    std::string getFileExtensionFromFilePath(std::string filepath, bool includeDot = false) {
        (void)filepath;
        (void)includeDot;
        return "";
    }
    std::string getFileNameFromFilePath(std::string filePath) {
        (void)filePath;
        return "";
    }

    // clipboard
    UString getClipBoardText() { return ""; }
    void setClipBoardText(UString text) { (void)text; }

    // dialogs & message boxes
    void showMessageInfo(UString title, UString message) {
        (void)title;
        (void)message;
    }
    void showMessageWarning(UString title, UString message) {
        (void)title;
        (void)message;
    }
    void showMessageError(UString title, UString message) {
        (void)title;
        (void)message;
    }
    void showMessageErrorFatal(UString title, UString message) {
        (void)title;
        (void)message;
    }
    UString openFileWindow(const char *filetypefilters, UString title, UString initialpath) {
        (void)filetypefilters;
        (void)title;
        (void)initialpath;
        return "";
    }
    UString openFolderWindow(UString title, UString initialpath) {
        (void)title;
        (void)initialpath;
        return "";
    }

    // window
    void focus() { ; }
    void center() { ; }
    void minimize() { ; }
    void maximize() { ; }
    void enableFullscreen() { ; }
    void disableFullscreen() { ; }
    void setWindowTitle(UString title) { (void)title; }
    void setWindowPos(int x, int y) {
        (void)x;
        (void)y;
    }
    void setWindowSize(int width, int height) {
        (void)width;
        (void)height;
    }
    void setWindowResizable(bool resizable) { (void)resizable; }
    void setWindowGhostCorporeal(bool corporeal) { (void)corporeal; }
    void setMonitor(int monitor) { (void)monitor; }
    Vector2 getWindowPos() { return Vector2(0, 0); }
    Vector2 getWindowSize() { return Vector2(1280, 720); }
    int getMonitor() { return 0; }
    std::vector<McRect> getMonitors() { return std::vector<McRect>(); }
    Vector2 getNativeScreenSize() { return Vector2(1920, 1080); }
    McRect getVirtualScreenRect() { return McRect(0, 0, 1920, 1080); }
    McRect getDesktopRect() { return McRect(0, 0, 1920, 1080); }
    int getDPI() { return 96; }
    bool isFullscreen() { return false; }
    bool isWindowResizable() { return true; }

    // mouse
    bool isCursorInWindow() { return true; }
    bool isCursorVisible() { return true; }
    bool isCursorClipped() { return false; }
    Vector2 getMousePos() { return Vector2(0, 0); }
    McRect getCursorClip() { return McRect(0, 0, 0, 0); }
    CURSORTYPE getCursor() { return CURSORTYPE::CURSOR_NORMAL; }
    void setCursor(CURSORTYPE cur) { (void)cur; }
    void setCursorVisible(bool visible) { (void)visible; }
    void setMousePos(int x, int y) {
        (void)x;
        (void)y;
    }
    void setCursorClip(bool clip, McRect rect) {
        (void)clip;
        (void)rect;
    }

    // keyboard
    UString keyCodeToString(KEYCODE keyCode) { return UString::format("%lu", keyCode); }
};

#endif
