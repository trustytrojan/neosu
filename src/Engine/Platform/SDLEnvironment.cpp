#include "SDLEnvironment.h"

#ifdef MCENGINE_FEATURE_SDL

#include "ConVar.h"
#include "Engine.h"
#include "NullContextMenu.h"
#include "NullGraphicsInterface.h"
#include "SDLGLES2Interface.h"
#include "SDLGLLegacyInterface.h"
#include "WinSDLGLES2Interface.h"

SDLEnvironment::SDLEnvironment(SDL_Window *window) : Environment() {
    m_window = window;

    m_bResizable = true;
    m_bFullscreen = false;

    m_bIsCursorInsideWindow = false;
    m_bCursorVisible = true;
    m_bCursorClipped = false;
    m_cursorType = CURSORTYPE::CURSOR_NORMAL;

    m_sPrevClipboardTextSDL = NULL;

    // TODO: init monitors
    if(this->vMonitors.size() < 1) {
        /// debugLog("WARNING: No monitors found! Adding default monitor ...\n");

        const Vector2 windowSize = getWindowSize();
        m_vMonitors.push_back(McRect(0, 0, windowSize.x, windowSize.y));
    }
}

void SDLEnvironment::update() {
    Environment::update();

    m_bIsCursorInsideWindow = McRect(0, 0, engine->getScreenWidth(), engine->getScreenHeight()).contains(getMousePos());
}

Graphics *SDLEnvironment::createRenderer() {
    // return new NullGraphicsInterface();
    return new SDLGLLegacyInterface(this->window);

#ifdef MCENGINE_FEATURE_OPENGLES

#ifdef _WIN32

    return new WinSDLGLES2Interface(this->window);

#else

    return new SDLGLES2Interface(this->window);

#endif

#endif
}

ContextMenu *SDLEnvironment::createContextMenu() { return new NullContextMenu(); }

Environment::OS SDLEnvironment::getOS() { return Environment::OS::NONE; }

void SDLEnvironment::shutdown() {
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}

void SDLEnvironment::restart() {
    // TODO:
    shutdown();
}

void SDLEnvironment::sleep(unsigned int us) { SDL_Delay(us / 1000); }

UString SDLEnvironment::getExecutablePath() {
    const char *path = SDL_GetBasePath();
    if(path != NULL) {
        UString uPath = UString(path);
        SDL_free((void *)path);
        return uPath;
    } else
        return UString("");
}

void SDLEnvironment::openURLInDefaultBrowser(UString url) {
    debugLog("WARNING: SDLEnvironment::openURLInDefaultBrowser() not available in SDL!\n");
}

UString SDLEnvironment::getUsername() { return UString(""); }

UString SDLEnvironment::getUserDataPath() {
    const char *path = SDL_GetPrefPath("McEngine", "McEngine");
    if(path != NULL) {
        UString uPath = UString(path);
        SDL_free((void *)path);
        return uPath;
    } else
        return UString("");
}

bool SDLEnvironment::fileExists(std::string filename) {
    SDL_RWops *file = SDL_RWFromFile(filename.c_str(), "r");
    if(file != NULL) {
        SDL_RWclose(file);
        return true;
    } else
        return false;
}

bool SDLEnvironment::directoryExists(std::string directoryName) {
    debugLog("WARNING: SDLEnvironment::directoryExists() not available in SDL!\n");
    return false;
}

bool SDLEnvironment::createDirectory(std::string directoryName) {
    debugLog("WARNING: SDLEnvironment::createDirectory() not available in SDL!\n");
    return false;
}

bool SDLEnvironment::renameFile(std::string oldFileName, std::string newFileName) {
    return std::rename(oldFileName.c_str(), newFileName.c_str());
}

bool SDLEnvironment::deleteFile(std::string filePath) { return std::remove(filePath.c_str()) == 0; }

std::vector<UString> SDLEnvironment::getFilesInFolder(UString folder) {
    debugLog("WARNING: SDLEnvironment::getFilesInFolder() not available in SDL!\n");
    return std::vector<UString>();
}

std::vector<UString> SDLEnvironment::getFoldersInFolder(UString folder) {
    debugLog("WARNING: SDLEnvironment::getFoldersInFolder() not available in SDL!\n");
    return std::vector<UString>();
}

std::vector<UString> SDLEnvironment::getLogicalDrives() {
    debugLog("WARNING: SDLEnvironment::getLogicalDrives() not available in SDL!\n");
    return std::vector<UString>();
}

std::string SDLEnvironment::getFolderFromFilePath(std::string filepath) {
    debugLog("WARNING: SDLEnvironment::getFolderFromFilePath() not available in SDL!\n");
    return filepath;
}

std::string SDLEnvironment::getFileExtensionFromFilePath(std::string filepath, bool includeDot) {
    const int idx = filepath.findLast(".");
    if(idx != -1)
        return filepath.substr(idx + 1);
    else
        return UString("");
}

std::string SDLEnvironment::getFileNameFromFilePath(std::string filePath) {
    if(filePath.length() < 1) return filePath;

    const size_t lastSlashIndex = filePath.findLast("/");
    if(lastSlashIndex != std::string::npos) return filePath.substr(lastSlashIndex + 1);

    return filePath;
}

UString SDLEnvironment::getClipBoardText() {
    if(this->sPrevClipboardTextSDL != NULL) SDL_free((void *)this->sPrevClipboardTextSDL);

    m_sPrevClipboardTextSDL = SDL_GetClipboardText();

    return (this->sPrevClipboardTextSDL != NULL ? UString(this->sPrevClipboardTextSDL) : UString(""));
}

void SDLEnvironment::setClipBoardText(UString text) { SDL_SetClipboardText(text.toUtf8()); }

void SDLEnvironment::showMessageInfo(UString title, UString message) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, title.toUtf8(), message.toUtf8(), this->window);
}

void SDLEnvironment::showMessageWarning(UString title, UString message) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, title.toUtf8(), message.toUtf8(), this->window);
}

void SDLEnvironment::showMessageError(UString title, UString message) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.toUtf8(), message.toUtf8(), this->window);
}

void SDLEnvironment::showMessageErrorFatal(UString title, UString message) { showMessageError(title, message); }

UString SDLEnvironment::openFileWindow(const char *filetypefilters, UString title, UString initialpath) {
    debugLog("WARNING: SDLEnvironment::openFileWindow() not available in SDL!\n");
    return UString("");
}

UString SDLEnvironment::openFolderWindow(UString title, UString initialpath) {
    debugLog("WARNING: SDLEnvironment::openFolderWindow() not available in SDL!\n");
    return UString("");
}

void SDLEnvironment::focus() { SDL_RaiseWindow(this->window); }

void SDLEnvironment::center() {
    // TODO: use nearest monitor
    Vector2 screenSize = getNativeScreenSize();
    Vector2 windowSize = getWindowSize();
    setWindowPos(screenSize.x / 2 - windowSize.x / 2, screenSize.y / 2 - windowSize.y / 2);
}

void SDLEnvironment::minimize() { SDL_MinimizeWindow(this->window); }

void SDLEnvironment::maximize() { SDL_MaximizeWindow(this->window); }

void SDLEnvironment::enableFullscreen() {
    if(this->bFullscreen) return;

    if(SDL_SetWindowFullscreen(this->window, SDL_WINDOW_FULLSCREEN_DESKTOP) ==
       0)  // NOTE: "fake" fullscreen since we don't want a videomode change
        m_bFullscreen = true;
}

void SDLEnvironment::disableFullscreen() {
    if(!m_bFullscreen) return;

    if(SDL_SetWindowFullscreen(this->window, 0) == 0) m_bFullscreen = false;
}

void SDLEnvironment::setWindowTitle(UString title) { SDL_SetWindowTitle(this->window, title.toUtf8()); }

void SDLEnvironment::setWindowPos(int x, int y) { SDL_SetWindowPosition(this->window, x, y); }

void SDLEnvironment::setWindowSize(int width, int height) { SDL_SetWindowSize(this->window, width, height); }

void SDLEnvironment::setWindowResizable(bool resizable) {
    SDL_SetWindowResizable(this->window, resizable ? SDL_TRUE : SDL_FALSE);
    m_bResizable = resizable;
}

void SDLEnvironment::setMonitor(int monitor) {
    // TODO:
    center();
}

Vector2 SDLEnvironment::getWindowPos() {
    int x = 0;
    int y = 0;
    { SDL_GetWindowPosition(this->window, &x, &y); }
    return Vector2(x, y);
}

Vector2 SDLEnvironment::getWindowSize() {
    int width = 100;
    int height = 100;
    { SDL_GetWindowSize(this->window, &width, &height); }
    return Vector2(width, height);
}

int SDLEnvironment::getMonitor() {
    // TODO: needs to be implemented in combination with center() etc.
    return 0;
    // const int monitor = SDL_GetWindowDisplayIndex(this->window);
    // return (monitor != -1 ? monitor : 0);
}

Vector2 SDLEnvironment::getNativeScreenSize() {
    SDL_DisplayMode dm;
    { SDL_GetCurrentDisplayMode(getMonitor(), &dm); }
    return Vector2(dm.w, dm.h);
}

McRect SDLEnvironment::getVirtualScreenRect() {
    // TODO:
    return McRect(0, 0, 1, 1);
}

McRect SDLEnvironment::getDesktopRect() {
    // TODO:
    Vector2 screen = getNativeScreenSize();
    return McRect(0, 0, screen.x, screen.y);
}

int SDLEnvironment::getDPI() {
    // TODO: get dpi for current display the window is on?
    const int displayIndex = 0;

    float dpi = 96.0f;
    SDL_GetDisplayDPI(displayIndex, NULL, &dpi, NULL);

    return clamp<int>((int)dpi, 96, 96 * 2);  // sanity clamp
}

Vector2 SDLEnvironment::getMousePos() {
    int mouseX = 0;
    int mouseY = 0;

    // HACKHACK: workaround, don't change this
    if(cv_mouse_sensitivity.getFloat() == 1.0f) {
        SDL_GetMouseState(&mouseX, &mouseY);

        return Vector2(mouseX, mouseY);
    } else {
        int windowX = 0;
        int windowY = 0;

        SDL_GetGlobalMouseState(&mouseX, &mouseY);
        SDL_GetWindowPosition(this->window, &windowX, &windowY);

        return Vector2(mouseX - windowX, mouseY - windowY);
    }
}

void SDLEnvironment::setCursor(CURSORTYPE cur) {
    // (not properly supported in SDL)
}

void SDLEnvironment::setCursorVisible(bool visible) {
    SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
    m_bCursorVisible = visible;
}

void SDLEnvironment::setMousePos(int x, int y) { SDL_WarpMouseInWindow(this->window, x, y); }

void SDLEnvironment::setCursorClip(bool clip, McRect rect) {
    m_cursorClip = rect;

    if(clip) {
        if(rect.getWidth() == 0 && rect.getHeight() == 0) {
            m_cursorClip = McRect(0, 0, engine->getScreenWidth(), engine->getScreenHeight());
        }

        // TODO: custom rect (only fullscreen works atm)
    }

    SDL_SetWindowGrab(this->window, clip ? SDL_TRUE : SDL_FALSE);
}

UString SDLEnvironment::keyCodeToString(KEYCODE keyCode) {
    const char *name = SDL_GetScancodeName((SDL_Scancode)keyCode);
    if(name == NULL)
        return UString::format("%lu", keyCode);
    else {
        UString uName = UString(name);
        if(uName.length() < 1)
            return UString::format("%lu", keyCode);
        else
            return uName;
    }
}

#endif
