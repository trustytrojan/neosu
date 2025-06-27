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
    ~NullEnvironment() override { ; }

    // engine/factory
    Graphics *createRenderer() override;
    ContextMenu *createContextMenu() override;

    // system
    OS getOS() override { return Environment::OS::NONE; }
    void shutdown() override;
    void restart() override;
    std::string getExecutablePath() override { return ""; }
    void openURLInDefaultBrowser(UString url) override { (void)url; }

    // user
    UString getUsername() override { return "<NULL>"; }
    std::string getUserDataPath() override { return "<NULL>"; }

    // file IO
    bool fileExists(std::string filename) override {
        (void)filename;
        return false;
    }
    bool directoryExists(std::string directoryName) override {
        (void)directoryName;
        return false;
    }
    bool createDirectory(std::string directoryName) override {
        (void)directoryName;
        return false;
    }
    bool renameFile(std::string oldFileName, std::string newFileName) override {
        (void)oldFileName;
        (void)newFileName;
        return false;
    }
    bool deleteFile(std::string filePath) override {
        (void)filePath;
        return false;
    }
    std::vector<std::string> getFilesInFolder(std::string folder) override {
        (void)folder;
        return std::vector<std::string>();
    }
    std::vector<std::string> getFoldersInFolder(std::string folder) override {
        (void)folder;
        return std::vector<std::string>();
    }
    std::vector<UString> getLogicalDrives() override { return std::vector<UString>(); }
    std::string getFolderFromFilePath(std::string filepath) override {
        (void)filepath;
        return "";
    }
    std::string getFileExtensionFromFilePath(std::string filepath, bool includeDot = false) override {
        (void)filepath;
        (void)includeDot;
        return "";
    }
    std::string getFileNameFromFilePath(std::string filePath) override {
        (void)filePath;
        return "";
    }

    // clipboard
    UString getClipBoardText() override { return ""; }
    void setClipBoardText(UString text) override { (void)text; }

    // dialogs & message boxes
    void showMessageInfo(UString title, UString message) override {
        (void)title;
        (void)message;
    }
    void showMessageWarning(UString title, UString message) override {
        (void)title;
        (void)message;
    }
    void showMessageError(UString title, UString message) override {
        (void)title;
        (void)message;
    }
    void showMessageErrorFatal(UString title, UString message) override {
        (void)title;
        (void)message;
    }
    UString openFileWindow(const char *filetypefilters, UString title, UString initialpath) override {
        (void)filetypefilters;
        (void)title;
        (void)initialpath;
        return "";
    }
    UString openFolderWindow(UString title, UString initialpath) override {
        (void)title;
        (void)initialpath;
        return "";
    }

    // window
    void focus() override { ; }
    void center() override { ; }
    void minimize() override { ; }
    void maximize() override { ; }
    void enableFullscreen() override { ; }
    void disableFullscreen() override { ; }
    void setWindowTitle(UString title) override { (void)title; }
    void setWindowPos(int x, int y) override {
        (void)x;
        (void)y;
    }
    void setWindowSize(int width, int height) override {
        (void)width;
        (void)height;
    }
    void setWindowResizable(bool resizable) override { (void)resizable; }
    void setMonitor(int monitor) override { (void)monitor; }
    Vector2 getWindowPos() override { return Vector2(0, 0); }
    Vector2 getWindowSize() override { return Vector2(1280, 720); }
    int getMonitor() override { return 0; }
    std::vector<McRect> getMonitors() override { return std::vector<McRect>(); }
    Vector2 getNativeScreenSize() override { return Vector2(1920, 1080); }
    McRect getVirtualScreenRect() override { return McRect(0, 0, 1920, 1080); }
    McRect getDesktopRect() override { return McRect(0, 0, 1920, 1080); }
    int getDPI() override { return 96; }
    bool isFullscreen() override { return false; }
    bool isWindowResizable() override { return true; }

    // mouse
    bool isCursorInWindow() override { return true; }
    bool isCursorVisible() override { return true; }
    bool isCursorClipped() override { return false; }
    Vector2 getMousePos() override { return Vector2(0, 0); }
    McRect getCursorClip() override { return McRect(0, 0, 0, 0); }
    CURSORTYPE getCursor() override { return CURSORTYPE::CURSOR_NORMAL; }
    void setCursor(CURSORTYPE cur) override { (void)cur; }
    void setCursorVisible(bool visible) override { (void)visible; }
    void setMousePos(int x, int y) override {
        (void)x;
        (void)y;
    }
    void setCursorClip(bool clip, McRect rect) override {
        (void)clip;
        (void)rect;
    }

    // keyboard
    UString keyCodeToString(KEYCODE keyCode) override { return UString::format("%lu", keyCode); }
};

#endif
