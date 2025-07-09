#ifdef __linux__

#include "LinuxEnvironment.h"

#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/cursorfont.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput2.h>
#include <dirent.h>
#include <libgen.h>
#include <math.h>
#include <math.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "Engine.h"
#include "LinuxGLLegacyInterface.h"
#include "XI2Handler.h"

typedef struct {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long inputMode;
    unsigned long status;
} Hints;

bool LinuxEnvironment::bResizable = true;
std::vector<McRect> LinuxEnvironment::vMonitors;

LinuxEnvironment::LinuxEnvironment(Display *display, Window window) : Environment() {
    env = this;
    this->display = display;
    this->window = window;

    this->bCursorClipped = false;
    this->bCursorRequest = false;
    this->bCursorReset = false;
    this->bCursorVisible = true;
    this->bIsCursorInsideWindow = false;
    this->mouseCursor = XCreateFontCursor(this->display, XC_left_ptr);
    this->invisibleCursor = this->makeBlankCursor();
    this->cursorType = CURSORTYPE::CURSOR_NORMAL;

    this->vCachedMousePos = Vector2(0, 0);
    this->bMousePosValid = false;

    this->atom_UTF8_STRING = XInternAtom(this->display, "UTF8_STRING", False);
    this->atom_CLIPBOARD = XInternAtom(this->display, "CLIPBOARD", False);
    this->atom_TARGETS = XInternAtom(this->display, "TARGETS", False);

    this->bFullScreen = false;
    this->iDPI = 96;

    this->bIsRestartScheduled = false;
    this->bResizeDelayHack = false;
    this->bPrevCursorHack = false;
    this->bFullscreenWasResizable = true;

    // init dpi
    {
        XrmInitialize();

        char *resourceString = XResourceManagerString(this->display);

        if(resourceString) {
            XrmDatabase db = XrmGetStringDatabase(resourceString);

            char *type = nullptr;
            XrmValue value;

            if(XrmGetResource(db, "Xft.dpi", "String", &type, &value) == True) {
                if(value.addr) {
                    this->iDPI = (int)std::atof(value.addr);
                    /// debugLog("m_iDPI = %i\n", this->iDPI);
                }
            }
        }
    }

    // TODO: init monitors
    if(LinuxEnvironment::vMonitors.size() < 1) {
        /// debugLog("WARNING: No monitors found! Adding default monitor ...\n");

        const Vector2 windowSize = this->getWindowSize();
        LinuxEnvironment::vMonitors.emplace_back(0, 0, windowSize.x, windowSize.y);
    }
}

LinuxEnvironment::~LinuxEnvironment() { XFreeCursor(this->display, this->invisibleCursor); }

void LinuxEnvironment::update() {
    if(!this->bCursorRequest) {
        if(this->bCursorReset) {
            this->bCursorReset = false;
            this->setCursor(CURSORTYPE::CURSOR_NORMAL);
        }
    }
    this->bCursorRequest = false;

    this->bIsCursorInsideWindow =
        McRect(0, 0, engine->getScreenWidth(), engine->getScreenHeight()).contains(this->getMousePos());
}

Graphics *LinuxEnvironment::createRenderer() { return new LinuxGLLegacyInterface(this->display, this->window); }

void LinuxEnvironment::shutdown() {
    XEvent ev;
    memset(&ev, 0, sizeof(ev));

    ev.type = ClientMessage;
    ev.xclient.type = ClientMessage;
    ev.xclient.window = this->window;
    ev.xclient.message_type = XInternAtom(this->display, "WM_PROTOCOLS", True);
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = static_cast<long>(XInternAtom(this->display, "WM_DELETE_WINDOW", False));
    ev.xclient.data.l[1] = CurrentTime;

    XSendEvent(this->display, this->window, false, NoEventMask, &ev);
}

void LinuxEnvironment::restart() {
    this->bIsRestartScheduled = true;
    this->shutdown();
}

std::string LinuxEnvironment::getExecutablePath() {
    char buf[4096];
    memset(buf, '\0', 4096);
    if(readlink("/proc/self/exe", buf, 4095) != -1)
        return std::string(buf);
    else
        return std::string("");
}

void LinuxEnvironment::openURLInDefaultBrowser(UString url) {
    if(fork() == 0) exit(execl("/usr/bin/xdg-open", "xdg-open", url.toUtf8(), (char *)nullptr));
}

void LinuxEnvironment::openDirectory(std::string path) {
    std::string cmd("xdg-open ");
    cmd.append(path);

    system(cmd.c_str());
}

UString LinuxEnvironment::getUsername() {
    passwd *pwd = getpwuid(getuid());
    if(pwd != nullptr && pwd->pw_name != nullptr)
        return UString(pwd->pw_name);
    else
        return UString("");
}

std::string LinuxEnvironment::getUserDataPath() {
    passwd *pwd = getpwuid(getuid());
    if(pwd != nullptr && pwd->pw_dir != nullptr)
        return std::string(pwd->pw_dir);
    else
        return std::string("");
}

bool LinuxEnvironment::createDirectory(std::string directoryName) { return mkdir(directoryName.c_str(), 0755) != -1; }

bool LinuxEnvironment::renameFile(std::string oldFileName, std::string newFileName) {
    return rename(oldFileName.c_str(), newFileName.c_str()) != -1;
}

bool LinuxEnvironment::deleteFile(std::string filePath) { return remove(filePath.c_str()) == 0; }

std::vector<std::string> LinuxEnvironment::getFilesInFolder(std::string folder) {
    std::vector<std::string> files;

    struct dirent **namelist{};
    int n = scandir(folder.c_str(), &namelist, getFilesInFolderFilter, alphasort);
    if(n < 0) {
        /// debugLog("error, scandir() returned %i!\n", n);
        return files;
    }

    for(int i = 0; i < n; i++) {
        std::string uName = &(namelist[i]->d_name)[0];
        std::string fullName = folder;
        fullName.append(uName);
        free(namelist[i]);

        struct stat stDirInfo{};
        int lstatret = lstat(fullName.c_str(), &stDirInfo);
        if(lstatret < 0) {
            // perror (name);
            // debugLog("error, lstat() returned %i!\n", lstatret);
            continue;
        }

        if(!S_ISDIR(stDirInfo.st_mode)) files.push_back(uName);
    }
    free(static_cast<void *>(namelist));

    return files;
}

std::vector<std::string> LinuxEnvironment::getFoldersInFolder(std::string folder) {
    std::vector<std::string> folders;

    struct dirent **namelist{};
    int n = scandir(folder.c_str(), &namelist, getFoldersInFolderFilter, alphasort);
    if(n < 0) {
        /// debugLog("error, scandir() returned %i!\n", n);
        return folders;
    }

    for(int i = 0; i < n; i++) {
        std::string uName = &(namelist[i]->d_name)[0];
        std::string fullName = folder;
        fullName.append(uName);
        free(namelist[i]);

        struct stat stDirInfo{};
        int lstatret = lstat(fullName.c_str(), &stDirInfo);
        if(lstatret < 0) {
            /// perror (name);
            /// debugLog("error, lstat() returned %i!\n", lstatret);
            continue;
        }

        if(S_ISDIR(stDirInfo.st_mode)) folders.push_back(uName);
    }
    free(static_cast<void *>(namelist));

    return folders;
}

std::vector<UString> LinuxEnvironment::getLogicalDrives() {
    std::vector<UString> drives;
    drives.emplace_back("/");
    return drives;
}

std::string LinuxEnvironment::getFolderFromFilePath(std::string filepath) {
    if(Environment::directoryExists(filepath))  // indirect check if this is already a valid directory (and not a file)
        return filepath;
    else
        return std::string(dirname((char *)filepath.c_str()));
}

std::string LinuxEnvironment::getFileExtensionFromFilePath(std::string filepath, bool includeDot) {
    const int idx = filepath.find_last_of('.');
    if(idx != -1)
        return filepath.substr(idx + 1);
    else
        return std::string("");
}

std::string LinuxEnvironment::getFileNameFromFilePath(std::string filePath) {
    if(filePath.length() < 1) return filePath;

    const size_t lastSlashIndex = filePath.find_last_of('/');
    if(lastSlashIndex != std::string::npos) return filePath.substr(lastSlashIndex + 1);

    return filePath;
}

UString LinuxEnvironment::getClipBoardText() { return this->getClipboardTextInt(); }

void LinuxEnvironment::setClipBoardText(UString text) { this->setClipBoardTextInt(text); }

void LinuxEnvironment::showMessageInfo(UString title, UString message) {
    // TODO:
}

void LinuxEnvironment::showMessageWarning(UString title, UString message) {
    // TODO:
}

void LinuxEnvironment::showMessageError(UString title, UString message) {
    // TODO:
}

void LinuxEnvironment::showMessageErrorFatal(UString title, UString message) {
    // TODO:
}

UString LinuxEnvironment::openFileWindow(const char *filetypefilters, UString title, UString initialpath) {
    // TODO:
    return UString("");
}

UString LinuxEnvironment::openFolderWindow(UString title, UString initialpath) {
    // TODO:
    return UString("");
}

void LinuxEnvironment::focus() {
    XRaiseWindow(this->display, this->window);
    XMapRaised(this->display, this->window);
}

void LinuxEnvironment::center() {
    Vector2 windowSize = this->getWindowSize();
    if(this->bResizeDelayHack) windowSize = this->vResizeHackSize;
    this->bResizeDelayHack = false;

    Screen *defaultScreen = XDefaultScreenOfDisplay(this->display);
    XMoveResizeWindow(this->display, this->window, WidthOfScreen(defaultScreen) / 2 - (unsigned int)(windowSize.x / 2),
                      HeightOfScreen(defaultScreen) / 2 - (unsigned int)(windowSize.y / 2), (unsigned int)windowSize.x,
                      (unsigned int)windowSize.y);
}

void LinuxEnvironment::minimize() { XIconifyWindow(this->display, this->window, 0); }

void LinuxEnvironment::maximize() {
    XMapWindow(this->display, this->window);

    // set size to fill entire screen (also fill borders)
    // the "x" and "y" members of "attributes" are the window's coordinates relative to its parent, i.e. to the
    // decoration window
    XWindowAttributes attributes;
    XGetWindowAttributes(this->display, this->window, &attributes);
    XMoveResizeWindow(this->display, this->window, -attributes.x, -attributes.y,
                      (unsigned int)this->getNativeScreenSize().x, (unsigned int)this->getNativeScreenSize().y);
}

void LinuxEnvironment::enableFullscreen() {
    if(this->bFullScreen) return;

    // backup
    if(this->vPrevDisableFullscreenWindowSize != this->getWindowSize()) {
        this->vLastWindowPos = this->getWindowPos();
        this->vLastWindowSize = this->getWindowSize();
    }

    // handle resizability (force enable while fullscreen)
    this->bFullscreenWasResizable = LinuxEnvironment::bResizable;
    this->setWindowResizable(true);

    // disable window decorations
    Hints hints;
    Atom property;
    hints.flags = 2;        // specify that we're changing the window decorations
    hints.decorations = 0;  // 0 (false) = disable decorations
    property = XInternAtom(this->display, "_MOTIF_WM_HINTS", True);
    XChangeProperty(this->display, this->window, property, property, 32, PropModeReplace, (unsigned char *)&hints, 5);

    // set size to fill entire screen (also fill borders)
    // the "x" and "y" members of "attributes" are the window's coordinates relative to its parent, i.e. to the
    // decoration window
    XWindowAttributes attributes;
    XGetWindowAttributes(this->display, this->window, &attributes);
    XMoveResizeWindow(this->display, this->window, -attributes.x, -attributes.y,
                      (unsigned int)this->getNativeScreenSize().x, (unsigned int)this->getNativeScreenSize().y);

    // suggest fullscreen mode
    Atom atom = XInternAtom(this->display, "_NET_WM_STATE_FULLSCREEN", True);
    XChangeProperty(this->display, this->window, XInternAtom(this->display, "_NET_WM_STATE", True), XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)&atom, 1);

    // get identifiers for the provided atom name strings
    Atom wm_state = XInternAtom(this->display, "_NET_WM_STATE", False);
    Atom fullscreen = XInternAtom(this->display, "_NET_WM_STATE_FULLSCREEN", False);

    XEvent xev;
    memset(&xev, 0, sizeof(xev));

    xev.type = ClientMessage;
    xev.xclient.window = this->window;
    xev.xclient.message_type = wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 1;  // enable fullscreen (1 == true)
    xev.xclient.data.l[1] = fullscreen;

    // send an event mask to the X-server
    XSendEvent(this->display, DefaultRootWindow(this->display), False, SubstructureNotifyMask, &xev);

    // force top window
    this->focus();

    this->bFullScreen = true;
}

void LinuxEnvironment::disableFullscreen() {
    if(!this->bFullScreen) return;

    // unsuggest fullscreen mode
    Atom atom = XInternAtom(this->display, "_NET_WM_STATE_FULLSCREEN", True);
    XChangeProperty(this->display, this->window, XInternAtom(this->display, "_NET_WM_STATE", True), XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)&atom, 1);

    // get identifiers for the provided atom name strings
    Atom wm_state = XInternAtom(this->display, "_NET_WM_STATE", False);
    Atom fullscreen = XInternAtom(this->display, "_NET_WM_STATE_FULLSCREEN", False);

    XEvent xev;
    memset(&xev, 0, sizeof(xev));

    xev.type = ClientMessage;
    xev.xclient.window = this->window;
    xev.xclient.message_type = wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 0;  // disable fullscreen (0 == false)
    xev.xclient.data.l[1] = fullscreen;

    // send an event mask to the X-server
    XSendEvent(this->display, DefaultRootWindow(this->display), False, SubstructureNotifyMask, &xev);

    // enable window decorations
    Hints hints;
    Atom property;
    hints.flags = 2;
    hints.decorations = 1;
    property = XInternAtom(this->display, "_MOTIF_WM_HINTS", True);
    XChangeProperty(this->display, this->window, property, property, 32, PropModeReplace, (unsigned char *)&hints, 5);

    // restore previous size and position
    // NOTE: the y-position is not consistent, the window keeps going down when toggling fullscreen (probably due to
    // decorations), force center() workaround
    XMoveResizeWindow(this->display, this->window, (int)this->vLastWindowPos.x, (int)this->vLastWindowPos.y,
                      (unsigned int)this->vLastWindowSize.x, (unsigned int)this->vLastWindowSize.y);
    this->vResizeHackSize = this->vLastWindowSize;
    this->bResizeDelayHack = true;

    // update resizability with new resolution
    this->setWindowResizableInt(this->bFullscreenWasResizable, this->vLastWindowSize);

    this->center();

    this->vPrevDisableFullscreenWindowSize = this->getWindowSize();
    this->bFullScreen = false;
}

void LinuxEnvironment::setWindowTitle(UString title) { XStoreName(this->display, this->window, title.toUtf8()); }

void LinuxEnvironment::setWindowPos(int x, int y) {
    XMapWindow(this->display, this->window);
    XMoveWindow(this->display, this->window, x, y);
}

void LinuxEnvironment::setWindowSize(int width, int height) {
    // due to the way resizability works, we have to temporarily disable it to be able to resize the window (because
    // min/max is fixed)
    const Vector2 windowPos = this->getWindowPos();
    const bool wasWindowResizable = LinuxEnvironment::bResizable;
    if(!wasWindowResizable) this->setWindowResizableInt(true, Vector2(width, height));

    this->vResizeHackSize = Vector2(width, height);
    this->bResizeDelayHack = true;

    // hack to force update the XSizeHints state
    XResizeWindow(this->display, this->window, (unsigned int)width, (unsigned int)height);
    XMoveWindow(this->display, this->window, (int)windowPos.x, (int)windowPos.y);
    XRaiseWindow(this->display, this->window);

    if(!wasWindowResizable) this->setWindowResizableInt(false, Vector2(width, height));

    XFlush(this->display);
}

void LinuxEnvironment::setWindowResizable(bool resizable) {
    this->setWindowResizableInt(resizable, this->getWindowSize());
}

void LinuxEnvironment::setWindowResizableInt(bool resizable, Vector2 windowSize) {
    LinuxEnvironment::bResizable = resizable;

    const Vector2 windowPos = this->getWindowPos();

    // window managers may ignore this completely, there is no way to force it
    XSizeHints wmsize;
    memset(&wmsize, 0, sizeof(XSizeHints));

    wmsize.flags = PMinSize | PMaxSize;
    wmsize.min_width = LinuxEnvironment::bResizable ? 100 : (int)windowSize.x;
    wmsize.min_height = LinuxEnvironment::bResizable ? 100 : (int)windowSize.y;
    wmsize.max_width = LinuxEnvironment::bResizable ? 16384 : (int)windowSize.x;
    wmsize.max_height = LinuxEnvironment::bResizable ? 16384 : (int)windowSize.y;

    XSetWMNormalHints(this->display, this->window, &wmsize);

    // hack to force update the XSizeHints state
    XResizeWindow(this->display, this->window, (unsigned int)windowSize.x, (unsigned int)windowSize.y);
    XMoveWindow(this->display, this->window, (int)windowPos.x, (int)windowPos.y);
    XRaiseWindow(this->display, this->window);

    XFlush(this->display);
}

void LinuxEnvironment::setMonitor(int monitor) {
    // TODO:
    this->center();
}

Vector2 LinuxEnvironment::getWindowPos() {
    // client coordinates
    Window rootRet;
    int x = 0;
    int y = 0;
    unsigned int borderWidth = 1;
    unsigned int depth = 0;
    unsigned int width = 1;
    unsigned int height = 1;

    XGetGeometry(this->display, this->window, &rootRet, &x, &y, &width, &height, &borderWidth, &depth);

    return Vector2(x, y);

    // server coordinates
    /*
    int x = 0;
    int y = 0;
    Window child;
    XWindowAttributes xwa;
    XTranslateCoordinates(this->display, this->window, DefaultRootWindow(this->display), 0, 0, &x, &y, &child );
    XGetWindowAttributes(this->display, this->window, &xwa);

    return Vector2(x - xwa.x, y - xwa.y);
    */
}

Vector2 LinuxEnvironment::getWindowSize() {
    // client size (engine coordinates)
    Window rootRet;
    int x = 0;
    int y = 0;
    unsigned int borderWidth = 1;
    unsigned int depth = 0;
    unsigned int width = 1;
    unsigned int height = 1;

    XGetGeometry(this->display, this->window, &rootRet, &x, &y, &width, &height, &borderWidth, &depth);

    return Vector2(width, height);
}

Vector2 LinuxEnvironment::getWindowSizeServer() {
    // server size
    XWindowAttributes xwa;
    XGetWindowAttributes(this->display, this->window, &xwa);

    return Vector2(xwa.width, xwa.height);
}

int LinuxEnvironment::getMonitor() {
    // TODO:
    return 0;
}

std::vector<McRect> LinuxEnvironment::getMonitors() { return LinuxEnvironment::vMonitors; }

Vector2 LinuxEnvironment::getNativeScreenSize() {
    return Vector2(WidthOfScreen(DefaultScreenOfDisplay(this->display)),
                   HeightOfScreen(DefaultScreenOfDisplay(this->display)));
}

McRect LinuxEnvironment::getVirtualScreenRect() {
    // TODO:
    return McRect(0, 0, 1, 1);
}

McRect LinuxEnvironment::getDesktopRect() {
    // TODO:
    Vector2 screen = this->getNativeScreenSize();
    return McRect(0, 0, screen.x, screen.y);
}

int LinuxEnvironment::getDPI() {
    return std::clamp<int>(this->iDPI, 96, 96 * 2);  // sanity clamp
}

bool LinuxEnvironment::isCursorInWindow() { return this->bIsCursorInsideWindow; }

bool LinuxEnvironment::isCursorVisible() { return this->bCursorVisible; }

bool LinuxEnvironment::isCursorClipped() { return this->bCursorClipped; }

Vector2d LinuxEnvironment::getMousePos() {
    if(!this->bMousePosValid) {
        // fallback to query pointer if cached position is invalid
        Window rootRet = 0, childRet = 0;
        double childX = 0.0, childY = 0.0;
        double rootX = 0.0, rootY = 0.0;
        XIButtonState buttons;
        XIModifierState modifiers;
        XIGroupState group;

        Bool result = XIQueryPointer(this->display, XI2Handler::clientPointerDevID, this->window, &rootRet, &childRet,
                                     &rootX, &rootY, &childX, &childY, &buttons, &modifiers, &group);
        if(result) {
            this->vCachedMousePos = Vector2d(childX, childY);
            this->bMousePosValid = true;
        }  // else fallback to cached position, which could happen during window switching or multi-monitor setups

        if(buttons.mask) {
            XFree(buttons.mask);
        }
    }

    return this->vCachedMousePos;
}

McRect LinuxEnvironment::getCursorClip() { return this->cursorClip; }

CURSORTYPE LinuxEnvironment::getCursor() { return this->cursorType; }

void LinuxEnvironment::setCursor(CURSORTYPE cur) {
    this->cursorType = cur;

    if(!this->bCursorVisible) return;

    switch(cur) {
        case CURSORTYPE::CURSOR_NORMAL:
            this->mouseCursor = XCreateFontCursor(this->display, XC_left_ptr);
            break;
        case CURSORTYPE::CURSOR_WAIT:
            this->mouseCursor = XCreateFontCursor(this->display, XC_circle);
            break;
        case CURSORTYPE::CURSOR_SIZE_H:
            this->mouseCursor = XCreateFontCursor(this->display, XC_sb_h_double_arrow);
            break;
        case CURSORTYPE::CURSOR_SIZE_V:
            this->mouseCursor = XCreateFontCursor(this->display, XC_sb_v_double_arrow);
            break;
        case CURSORTYPE::CURSOR_SIZE_HV:
            this->mouseCursor = XCreateFontCursor(this->display, XC_bottom_left_corner);
            break;
        case CURSORTYPE::CURSOR_SIZE_VH:
            this->mouseCursor = XCreateFontCursor(this->display, XC_bottom_right_corner);
            break;
        case CURSORTYPE::CURSOR_TEXT:
            this->mouseCursor = XCreateFontCursor(this->display, XC_xterm);
            break;
        default:
            this->mouseCursor = XCreateFontCursor(this->display, XC_left_ptr);
            break;
    }

    this->setCursorInt(this->mouseCursor);

    this->bCursorReset = true;
    this->bCursorRequest = true;
}

void LinuxEnvironment::setCursorVisible(bool visible) {
    this->bCursorVisible = visible;
    this->setCursorInt(visible ? this->mouseCursor : this->invisibleCursor);
}

void LinuxEnvironment::setMousePos(double x, double y) {
    XIWarpPointer(this->display, XI2Handler::clientPointerDevID, None, this->window, 0, 0, 0, 0, x, y);
    // XFlush is usually sufficient, XSync forces unnecessary round-trip
    XFlush(this->display);

    // update cached position immediately
    this->vCachedMousePos = Vector2(x, y);
    this->bMousePosValid = true;
}

void LinuxEnvironment::setCursorClip(bool clip, McRect rect) {
    if(clip) {
        this->bCursorClipped = XI2Handler::grab(this->display, true, false);
    } else {
        this->bCursorClipped = XI2Handler::grab(this->display, false, false);
    }
    if(this->bCursorClipped) this->cursorClip = rect;
}

UString LinuxEnvironment::keyCodeToString(KEYCODE keyCode) {
    const char *name = XKeysymToString(keyCode);
    return name != nullptr ? UString(name) : UString("");
}

// helper functions

int LinuxEnvironment::getFilesInFolderFilter(const struct dirent *entry) { return 1; }

int LinuxEnvironment::getFoldersInFolderFilter(const struct dirent *entry) {
    if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) return 0;
    return 1;
}

Cursor LinuxEnvironment::makeBlankCursor() {
    static char data[1] = {0};
    Cursor cursor;
    Pixmap blank;
    XColor dummy;

    blank = XCreateBitmapFromData(this->display, this->window, data, 1, 1);
    if(blank == None) {
        debugLog("fatal error, XCreateBitmapFromData() out of memory!\n");
        return 0;
    }
    cursor = XCreatePixmapCursor(this->display, blank, blank, &dummy, &dummy, 0, 0);
    XFreePixmap(this->display, blank);

    return cursor;
}

void LinuxEnvironment::setCursorInt(Cursor cursor) {
    if(this->bPrevCursorHack) XUndefineCursor(this->display, this->window);
    this->bPrevCursorHack = true;

    XDefineCursor(this->display, this->window, cursor);
}

UString LinuxEnvironment::readWindowProperty(Window window, Atom prop, Atom fmt /* XA_STRING or UTF8_STRING */,
                                             bool deleteAfterReading) {
    UString returnData = UString("");
    unsigned char *clipData;
    Atom actualType;
    int actualFormat;
    unsigned long nitems, bytesLeft;
    if(XGetWindowProperty(this->display, this->window, prop, 0L /* offset */, 1000000 /* length (max) */, False,
                          AnyPropertyType /* format */, &actualType, &actualFormat, &nitems, &bytesLeft,
                          &clipData) == Success) {
        if(actualType == this->atom_UTF8_STRING && actualFormat == 8) {
            // very inefficient, but whatever
            std::string temp;
            for(size_t i = 0; i < nitems; i++) {
                temp += clipData[i];
            }
            returnData = UString(temp.c_str());
        } else if(actualType == XA_STRING && actualFormat == 8) {
            // very inefficient, but whatever
            std::string temp;
            for(size_t i = 0; i < nitems; i++) {
                temp += clipData[i];
            }
            returnData = UString(temp.c_str());
        }

        if(clipData != nullptr) XFree(clipData);
    }

    if(deleteAfterReading) XDeleteProperty(this->display, window, prop);

    return returnData;
}

bool LinuxEnvironment::requestSelectionContent(UString &selection_content, Atom selection, Atom requested_format) {
    // send a SelectionRequest to the window owning the selection and waits for its answer (with a timeout)
    // the selection owner will be asked to set the MCENGINE_SEL property on m_window with the selection content
    Atom property_name = XInternAtom(this->display, "MCENGINE_SEL", false);
    XConvertSelection(this->display, selection, requested_format, property_name, this->window, CurrentTime);
    bool gotReply = false;
    int timeoutMs = 200;  // will wait at most for 200 ms
    do {
        XEvent event;
        gotReply = XCheckTypedWindowEvent(this->display, this->window, SelectionNotify, &event);
        if(gotReply) {
            if(event.xselection.property == property_name) {
                selection_content = this->readWindowProperty(event.xselection.requestor, event.xselection.property,
                                                             requested_format, true);
                return true;
            } else  // the format we asked for was denied.. (event.xselection.property == None)
                return false;
        }

        // not very elegant.. we could do a select() or something like that... however clipboard content requesting
        // is inherently slow on x11, it often takes 50ms or more so...
        Timing::sleepMS(4);
        timeoutMs -= 4;
    } while(timeoutMs > 0);

    debugLog(": Timeout!\n");
    return false;
}

void LinuxEnvironment::handleSelectionRequest(XSelectionRequestEvent &evt) {
    // called from the event loop in response to SelectionRequest events
    // the selection content is sent to the target window as a window property
    XSelectionEvent reply;
    reply.type = SelectionNotify;
    reply.display = evt.display;
    reply.requestor = evt.requestor;
    reply.selection = evt.selection;
    reply.target = evt.target;
    reply.property = None;  // == "fail"
    reply.time = evt.time;

    char *data = nullptr;
    int property_format = 0, data_nitems = 0;
    if(evt.selection == XA_PRIMARY || evt.selection == this->atom_CLIPBOARD) {
        if(evt.target == XA_STRING) {
            // format data according to system locale
            data = strdup((const char *)this->sLocalClipboardContent.toUtf8());
            data_nitems = strlen(data);
            property_format = 8;  // bits/item
        } else if(evt.target == this->atom_UTF8_STRING) {
            // translate to utf8
            data = strdup((const char *)this->sLocalClipboardContent.toUtf8());
            data_nitems = strlen(data);
            property_format = 8;  // bits/item
        } else if(evt.target == this->atom_TARGETS) {
            // another application wants to know what we are able to send
            data_nitems = 2;
            property_format = 32;  // atoms are 32-bit
            data = (char *)malloc(data_nitems * 4);
            ((Atom *)data)[0] = this->atom_UTF8_STRING;
            ((Atom *)data)[1] = XA_STRING;
        }
    } else
        debugLog(": Requested unsupported clipboard!\n");

    if(data) {
        const size_t MAX_REASONABLE_SELECTION_SIZE = 1000000;

        // for very big chunks of data, we should use the "INCR" protocol, which is a pain in the asshole
        if(evt.property != None && strlen(data) < MAX_REASONABLE_SELECTION_SIZE) {
            XChangeProperty(evt.display, evt.requestor, evt.property, evt.target, property_format /* 8 or 32 */,
                            PropModeReplace, (const unsigned char *)data, data_nitems);
            reply.property = evt.property;  // " == success"
        }
        free(data);
    }

    XSendEvent(evt.display, evt.requestor, 0, NoEventMask, (XEvent *)&reply);
}

void LinuxEnvironment::setClipBoardTextInt(UString clipText) {
    this->sLocalClipboardContent = std::move(clipText);
    XSetSelectionOwner(this->display, XA_PRIMARY, this->window, CurrentTime);
    XSetSelectionOwner(this->display, this->atom_CLIPBOARD, this->window, CurrentTime);
}

UString LinuxEnvironment::getClipboardTextInt() {
    UString content;

    // only work with the "CLIPBOARD" selection (ctrl-C/ctrl-V clipboard)
    // ignore PRIMARY selection (select-to-copy/middle-click-paste)

    Window selection_owner = XGetSelectionOwner(this->display, this->atom_CLIPBOARD);

    if(selection_owner != None) {
        if(selection_owner == this->window)          // ourself
            content = this->sLocalClipboardContent;  // just return the local clipboard
        else {
            // first try: we want an utf8 string
            bool ok = this->requestSelectionContent(content, this->atom_CLIPBOARD, this->atom_UTF8_STRING);
            if(!ok)  // second chance, ask for a good old locale-dependent string
                ok = this->requestSelectionContent(content, this->atom_CLIPBOARD, XA_STRING);
        }
    }

    return content;
}

#endif

#include <filesystem>
#include <utility>

std::string fix_filename_casing(const std::string& directory, std::string filename) {
#ifdef _WIN32
    return filename;
#else
    if(!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) return filename;

    std::filesystem::path dir = directory;
    for(auto &entry : std::filesystem::directory_iterator(dir)) {
        if(entry.is_regular_file() && !strcasecmp(entry.path().filename().string().c_str(), filename.c_str())) {
            return entry.path().filename().string();
        }
    }
    return filename;
#endif
}
