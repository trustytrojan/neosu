#pragma once

#include "UString.h"
#include "Cursors.h"
#include "Graphics.h"
#include "KeyboardEvent.h"
#include "Rect.h"

#ifdef MCENGINE_PLATFORM_WINDOWS  // temp, for isatty
#include <io.h>
#else
#include <unistd.h>  // isatty libc++
#endif

#include <filesystem>
#include <unordered_map>
#include <functional>

class Environment;

extern Environment *env;

class LinuxMain;
class LinuxEnvironment;
class WinEnvironment;
class WindowsMain;

class Environment {
    friend class LinuxEnvironment;
    friend class LinuxMain;
    friend class WinEnvironment;
    friend class WindowsMain;

   public:
    Environment(const std::vector<UString> &argCmdline,
                const std::unordered_map<UString, std::optional<UString>> &argMap);
    virtual ~Environment() { env = NULL; }

    virtual void update() { ; }

    // engine/factory
    virtual Graphics *createRenderer() = 0;

    // system
    virtual void shutdown() = 0;
    virtual void restart() = 0;

    // resolved and cached at early startup with argv[0]
    // contains the full canonical path to the current exe
    static const std::string &getPathToSelf(const char *argv0 = nullptr);

    [[nodiscard]] inline const std::unordered_map<UString, std::optional<UString>> &getLaunchArgs() const {
        return mArgMap;
    }
    [[nodiscard]] inline const std::vector<UString> &getCommandLine() const { return vCmdLine; }

    // i.e. getenv()
    static std::string getEnvVariable(const std::string &varToQuery) noexcept;

    static const std::string &getExeFolder();

    // replace non-alphanum characters in a string to %-encoded ones viable for a URL
    [[nodiscard]] static std::string encodeStringToURL(const std::string &stringToConvert) noexcept;

    static void openURLInDefaultBrowser(const std::string &url) noexcept;
    static void openFileBrowser(const std::string &initialpath) noexcept;

    // user
    virtual UString getUsername() = 0;
    static const std::string &getUserDataPath();

    // file IO
    virtual std::vector<UString> getLogicalDrives() = 0;

    static inline bool isaTTY() { return isatty(fileno(stdout)) != 0; }

    // modifies the input filename! (checks case insensitively past the last slash)
    static bool fileExists(std::string &filename);
    // modifies the input directoryName! (checks case insensitively past the last slash)
    static bool directoryExists(std::string &directoryName);

    // same as the above, but for string literals (so we can't check insensitively and modify the input)
    static bool fileExists(const std::string &filename);
    static bool directoryExists(const std::string &directoryName);

    static bool createDirectory(const std::string &directoryName);
    static bool renameFile(const std::string &oldFileName, const std::string &newFileName);
    static bool deleteFile(const std::string &filePath);

    static std::string getFolderFromFilePath(const std::string &filepath) noexcept;
    static std::string getFileExtensionFromFilePath(const std::string &filepath, bool includeDot = false) noexcept;
    static std::string getFileNameFromFilePath(const std::string &filePath) noexcept;

    // i can't make these as fast as the win32 api version in a generic way
    virtual std::vector<std::string> getFilesInFolder(const std::string &folder) noexcept;
    virtual std::vector<std::string> getFoldersInFolder(const std::string &folder) noexcept;

    // clipboard
    virtual UString getClipBoardText() = 0;
    virtual void setClipBoardText(UString text) = 0;

    // dialogs & message boxes
    virtual void showMessageInfo(const UString &title, const UString &message);
    virtual void showMessageWarning(const UString &title, const UString &message);
    virtual void showMessageError(const UString &title, const UString &message);
    virtual void showMessageErrorFatal(const UString &title, const UString &message);

    using FileDialogCallback = std::function<void(const std::vector<UString> &paths)>;
    static void openFileWindow(FileDialogCallback callback, const char *filetypefilters, const UString &title,
                               const UString &initialpath = "");
    static void openFolderWindow(FileDialogCallback callback, const UString &initialpath = "");

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
    virtual Vector2d getMousePos() = 0;
    virtual McRect getCursorClip() = 0;
    virtual CURSORTYPE getCursor() = 0;
    virtual void setCursor(CURSORTYPE cur) = 0;
    virtual void setCursorVisible(bool visible) = 0;
    virtual void setMousePos(double x, double y) = 0;
    virtual void setCursorClip(bool clip, McRect rect) = 0;

    // keyboard
    virtual UString keyCodeToString(KEYCODE keyCode) = 0;

   public:
    // window
    virtual void setFullscreenWindowedBorderless(bool fullscreenWindowedBorderless);
    virtual bool isFullscreenWindowedBorderless() { return this->bFullscreenWindowedBorderless; }
    virtual float getDPIScale() { return (float)this->getDPI() / 96.0f; }

   protected:
    bool bFullscreenWindowedBorderless;
    std::unordered_map<UString, std::optional<UString>> mArgMap;
    std::vector<UString> vCmdLine;

   private:
    // static callbacks/helpers
    struct FileDialogCallbackData {
        FileDialogCallback callback;
    };
    static void sdlFileDialogCallback(void *userdata, const char *const *filelist, int filter);
    static int s_sdl_dialog_opened;

    // for getting files in folder/ folders in folder
    static std::vector<std::string> enumerateDirectory(const std::string &pathToEnum,
                                                       /* enum SDL_PathType */ unsigned int type) noexcept;
    static std::string getThingFromPathHelper(
        const std::string &path,
        bool folder) noexcept;  // code sharing for getFolderFromFilePath/getFileNameFromFilePath

    // internal path conversion helper, SDL_URLOpen needs a URL-encoded URI on Unix (because it goes to xdg-open)
    [[nodiscard]] static std::string filesystemPathToURI(const std::filesystem::path &path) noexcept;
};
