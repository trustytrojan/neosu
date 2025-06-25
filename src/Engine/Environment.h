#pragma once
#include "ConVar.h"
#include "Cursors.h"
#include "Graphics.h"
#include "KeyboardEvent.h"
#include "Rect.h"
#include "cbase.h"

#ifdef MCENGINE_PLATFORM_WINDOWS // temp, for isatty
#include <io.h>
#endif

class ContextMenu;

class Environment {
   public:
    enum class OS { NONE, WINDOWS, LINUX, MACOS };

   public:
    Environment();
    virtual ~Environment() { ; }

    virtual void update() { ; }

    // engine/factory
    virtual Graphics *createRenderer() = 0;
    virtual ContextMenu *createContextMenu() = 0;

    // system
    virtual OS getOS() = 0;
    virtual void shutdown() = 0;
    virtual void restart() = 0;
    virtual void sleep(unsigned int us) = 0;
    virtual std::string getExecutablePath() = 0;
    virtual void openURLInDefaultBrowser(UString url) = 0;
    virtual void openDirectory(std::string path) = 0;

    // user
    virtual UString getUsername() = 0;
    virtual std::string getUserDataPath() = 0;

    // file IO
    virtual bool fileExists(std::string fileName) = 0;
    virtual bool directoryExists(std::string directoryName) = 0;
    virtual bool createDirectory(std::string directoryName) = 0;
    virtual bool renameFile(std::string oldFileName, std::string newFileName) = 0;
    virtual bool deleteFile(std::string filePath) = 0;
    virtual std::vector<std::string> getFilesInFolder(std::string folder) = 0;
    virtual std::vector<std::string> getFoldersInFolder(std::string folder) = 0;
    virtual std::vector<UString> getLogicalDrives() = 0;
    virtual std::string getFolderFromFilePath(std::string filepath) = 0;
    virtual std::string getFileExtensionFromFilePath(std::string filepath, bool includeDot = false) = 0;
    virtual std::string getFileNameFromFilePath(std::string filePath) = 0;

    static inline bool isaTTY() { return ::isatty(fileno(stdout)) != 0; }

    // clipboard
    virtual UString getClipBoardText() = 0;
    virtual void setClipBoardText(UString text) = 0;

    // dialogs & message boxes
    virtual void showMessageInfo(UString title, UString message) = 0;
    virtual void showMessageWarning(UString title, UString message) = 0;
    virtual void showMessageError(UString title, UString message) = 0;
    virtual void showMessageErrorFatal(UString title, UString message) = 0;
    virtual UString openFileWindow(const char *filetypefilters, UString title, UString initialpath) = 0;
    virtual UString openFolderWindow(UString title, UString initialpath) = 0;

    // window
    virtual void focus() = 0;
    virtual void center() = 0;
    virtual void minimize() = 0;
    virtual void maximize() = 0;
    virtual void enableFullscreen() = 0;
    virtual void disableFullscreen() = 0;
    virtual void setWindowTitle(UString title) = 0;
    virtual void setWindowPos(int x, int y) = 0;
    virtual void setWindowSize(int width, int height) = 0;
    virtual void setWindowResizable(bool resizable) = 0;
    virtual void setMonitor(int monitor) = 0;
    virtual Vector2 getWindowPos() = 0;
    virtual Vector2 getWindowSize() = 0;
    virtual int getMonitor() = 0;
    virtual std::vector<McRect> getMonitors() = 0;
    virtual Vector2 getNativeScreenSize() = 0;
    virtual McRect getVirtualScreenRect() = 0;
    virtual McRect getDesktopRect() = 0;
    virtual int getDPI() = 0;
    virtual bool isFullscreen() = 0;
    virtual bool isWindowResizable() = 0;

    // mouse
    virtual bool isCursorInWindow() = 0;
    virtual bool isCursorVisible() = 0;
    virtual bool isCursorClipped() = 0;
    virtual Vector2 getMousePos() = 0;
    virtual McRect getCursorClip() = 0;
    virtual CURSORTYPE getCursor() = 0;
    virtual void setCursor(CURSORTYPE cur) = 0;
    virtual void setCursorVisible(bool visible) = 0;
    virtual void setMousePos(int x, int y) = 0;
    virtual void setCursorClip(bool clip, McRect rect) = 0;

    // keyboard
    virtual UString keyCodeToString(KEYCODE keyCode) = 0;

   public:
    // built-in convenience
    i32 get_nb_cpu_cores();

    // window
    virtual void setFullscreenWindowedBorderless(bool fullscreenWindowedBorderless);
    virtual bool isFullscreenWindowedBorderless() { return this->bFullscreenWindowedBorderless; }
    virtual float getDPIScale() { return (float)this->getDPI() / 96.0f; }

   protected:
    bool bFullscreenWindowedBorderless;
};

extern Environment *env;
