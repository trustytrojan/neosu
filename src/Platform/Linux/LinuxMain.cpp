#ifdef __linux__

#include <cstdio>
#include <cstdlib>

#include "LinuxMain.h"

#include "ConVar.h"
#include "Engine.h"
#include "LinuxEnvironment.h"
#include "LinuxGLLegacyInterface.h"
#include "XI2Handler.h"
#include "Profiler.h"
#include "Timing.h"
#include "FPSLimiter.h"

#include "Keyboard.h"
#include "KeyboardKeys.h"

#define XLIB_ILLEGAL_ACCESS
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput2.h>
#include <X11/Xutil.h>
#include <unistd.h>

#include "OpenGLHeaders.h"

#include <SDL3/SDL_events.h>  // for SDL_PumpEvents, needed for SDL_ShowOpenFileDialog/SDL_ShowOpenFolderDialog

#define WINDOW_TITLE "neosu"

// TODO: this is incorrect, the size here doesn't take the decorations into account
#define WINDOW_WIDTH (1280)
#define WINDOW_HEIGHT (720)

#define WINDOW_WIDTH_MIN 100
#define WINDOW_HEIGHT_MIN 100

//********************//
//	Main entry point  //
//********************//

LinuxMain::LinuxMain(int argc, char *argv[], const std::vector<UString> &argCmdline,
                     const std::unordered_map<UString, std::optional<UString>> &argMap) {
    // XInitThreads();
    this->dpy = XOpenDisplay(nullptr);
    if(this->dpy == nullptr) {
        printf("FATAL ERROR: XOpenDisplay() can't connect to X server!\n\n");
        return;
    }

    // before we do anything, check if XInput is available (for raw mouse input, smooth horizontal & vertical mouse
    // wheel etc.)
    int xi2firstEvent = -1, xi2error = -1;
    if(!XQueryExtension(this->dpy, "XInputExtension", &this->xi2opcode, &xi2firstEvent, &xi2error)) {
        printf("FATAL ERROR: XQueryExtension() XInput extension not available!\n\n");
        return;
    }

    // want version 2 at least
    int ximajor = 2, ximinor = 0;
    if(XIQueryVersion(this->dpy, &ximajor, &ximinor) == BadRequest) {
        printf("FATAL ERROR: XIQueryVersion() XInput2 not available, server supports only %d.%d!\n\n", ximajor,
               ximinor);
        return;
    }

    // debugLogF("XI2 version: {:d}.{:d}, opcode: {:d}, XI_RawMotion: {:d}\n", ximajor, ximinor, this->xi2opcode,
    // XI_RawMotion);

    this->rootWindow = DefaultRootWindow(this->dpy);

    // resolve GLX functions
    int screen = DefaultScreen(this->dpy);
    if(!gladLoadGLX(this->dpy, screen)) {
        printf("FATAL ERROR: Couldn't resolve GLX functions (gladLoadGLX() failed)!\n\n");
        return;
    }

    int major = 0, minor = 0;
    glXQueryVersion(this->dpy, &major, &minor);
    debugLogF("GLX Version: {}.{}\n", major, minor);

    XVisualInfo *vi = LinuxGLLegacyInterface::getVisualInfo(this->dpy);
    if(!vi) {
        printf("FATAL ERROR: Couldn't glXChooseVisual!\n\n");
        return;
    }

    this->cmap = XCreateColormap(this->dpy, this->rootWindow, vi->visual, AllocNone);

    // set window attributes
    this->swa.colormap = this->cmap;
    this->swa.event_mask =
        ExposureMask | FocusChangeMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | KeymapStateMask;

    Screen *defaultScreen = DefaultScreenOfDisplay(this->dpy);
    this->clientWindow = XCreateWindow(this->dpy, this->rootWindow, defaultScreen->width / 2 - WINDOW_WIDTH / 2,
                                       defaultScreen->height / 2 - WINDOW_HEIGHT / 2, WINDOW_WIDTH, WINDOW_HEIGHT, 0,
                                       vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &this->swa);

    XFree(vi);

    // window resize limit
    XSizeHints *winSizeHints = XAllocSizeHints();
    if(!winSizeHints) {
        printf("FATAL ERROR: XAllocSizeHints() out of memory!\n\n");
        return;
    }
    winSizeHints->flags = PMinSize;
    winSizeHints->min_width = WINDOW_WIDTH_MIN;
    winSizeHints->min_height = WINDOW_HEIGHT_MIN;
    XSetWMNormalHints(this->dpy, this->clientWindow, winSizeHints);
    XFree(winSizeHints);

    // register custom delete message for quitting the engine (received as ClientMessage in the main loop)
    Atom wm_delete_window = XInternAtom(this->dpy, "WM_DELETE_WINDOW", false);
    XSetWMProtocols(this->dpy, this->clientWindow, &wm_delete_window, 1);

    // hint that compositing should be disabled (disable forced vsync)
    constexpr const unsigned char shouldBypassCompositor = 1;
    Atom _net_wm_bypass_compositor = XInternAtom(this->dpy, "_NET_WM_BYPASS_COMPOSITOR", false);
    XChangeProperty(this->dpy, this->clientWindow, _net_wm_bypass_compositor, XA_CARDINAL, 32, PropModeReplace,
                    &shouldBypassCompositor, 1);

    // why are there 4 different properties to set for the title...
    XTextProperty wm_name_prop;
    char *title = (char *)WINDOW_TITLE;
    int rc = XStringListToTextProperty(&title, 1, &wm_name_prop);
    if(rc) {
        // one...
        XSetWMName(this->dpy, this->clientWindow, &wm_name_prop);
        XFree(wm_name_prop.value);  // insanity
    }

    XClassHint *wm_class_hint = XAllocClassHint();
    if(wm_class_hint) {
        wm_class_hint->res_class = (char *)WINDOW_TITLE;  // two...
        wm_class_hint->res_name = (char *)WINDOW_TITLE;   // three...
        XSetClassHint(this->dpy, this->clientWindow, wm_class_hint);
        XFree(wm_class_hint);
    }

    // make window visible & set title
    XMapWindow(this->dpy, this->clientWindow);
    XStoreName(this->dpy, this->clientWindow, WINDOW_TITLE);  // four.

    // after the window is visible, center it again (if the window manager ignored the position of the window in
    // XCreateWindow(), because fuck you)
    // debugLogF("moving to {} {}\n",  defaultScreen->width / 2 - WINDOW_WIDTH / 2,
    //             defaultScreen->height / 2 - WINDOW_HEIGHT / 2);
    XMoveWindow(this->dpy, this->clientWindow, defaultScreen->width / 2 - WINDOW_WIDTH / 2,
                defaultScreen->height / 2 - WINDOW_HEIGHT / 2);

    // get input method
    this->im = XOpenIM(this->dpy, nullptr, nullptr, nullptr);
    if(this->im == nullptr) {
        printf("FATAL ERROR: XOpenIM() couldn't open input method!\n\n");
        return;
    }

    // get input context
    this->ic = XCreateIC(this->im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow,
                         this->clientWindow, NULL);
    if(this->ic == nullptr) {
        printf("FATAL ERROR: XCreateIC() couldn't create input context!\n\n");
        return;
    }

    XSync(this->dpy, False);

    XI2Handler::clientPointerDevID = XI2Handler::getClientPointer(this->dpy);
    XI2Handler::updateDeviceCache(this->dpy);

    // select xi2 events
    XI2Handler::selectEvents(this->dpy, this->clientWindow, DefaultRootWindow(this->dpy));

    // get keyboard focus
    XSetICFocus(this->ic);

    XSync(this->dpy, False);

    // create timers
    auto *deltaTimer = new Timer();

    // initialize engine
    baseEnv = new LinuxEnvironment(this->dpy, this->clientWindow, argCmdline, argMap);
    engine = new Engine(argc, argv);

    deltaTimer->start();
    deltaTimer->update();

    engine->loadApp();

    // main loop
    while(this->bRunning) {
        VPROF_MAIN();

        // handle window message queue
        {
            VPROF_BUDGET("Events", VPROF_BUDGETGROUP_EVENTS);

            if(Environment::s_sdl_dialog_opened > 0) {
                /* From SDL_dialog.h:
                 * On Linux, dialogs may require XDG Portals, which requires DBus, which
                 * requires an event-handling loop. Apps that do not use SDL to handle events
                 * should add a call to SDL_PumpEvents in their main loop.
                 */
                SDL_PumpEvents();
            }

            while(XPending(this->dpy)) {
                XNextEvent(this->dpy, &this->xev);

                if(XFilterEvent(&this->xev, this->clientWindow))  // for keyboard chars
                    continue;

                WndProc();
            }
        }

        // update
        {
            deltaTimer->update();
            engine->setFrameTime(deltaTimer->getDelta());

            if(this->bUpdate) engine->onUpdate();
        }

        // draw
        if(this->bDraw) {
            engine->onPaint();
        }

        // delay the next frame
        {
            VPROF_BUDGET("FPSLimiter", VPROF_BUDGETGROUP_SLEEP);

            // delay the next frame
            const int target_fps = !this->bHasFocus ? cv::fps_max_background.getInt()
                                                    : osu->isInPlayMode() ? cv::fps_max.getInt() : cv::fps_max_menu.getInt());
            FPSLimiter::limit_frames(target_fps);
        }
    }

    // release the engine
    SAFE_DELETE(engine);

    // clean up environment
    SAFE_DELETE(baseEnv);

    // release timer
    SAFE_DELETE(deltaTimer);

    // destroy the window
    XDestroyWindow(this->dpy, this->clientWindow);
    XCloseDisplay(this->dpy);

    // unload GLX
    gladUnloadGLX();

    ret = 0;
}

//****************//
//	Message loop  //
//****************//

KEYCODE LinuxMain::normalizeKeysym(KEYCODE keysym) {
    switch(keysym) {
        case XK_ISO_Left_Tab:  // shift+tab variant
            return KEY_TAB;
        case XK_KP_Enter:  // numpad enter
            return KEY_ENTER;
        // need to add more on a case-by-case basis... annoying
        default:
            return keysym;
    }
}

void LinuxMain::WndProc() {
    switch(this->xev.type) {
        case GenericEvent:
            if(baseEnv && this->xev.xcookie.extension == this->xi2opcode) {
                XI2Handler::handleGenericEvent(this->dpy, this->xev);
            }
            break;

        case ConfigureNotify:
            if(engine != NULL) {
                const Vector2 size{this->xev.xconfigure.width, this->xev.xconfigure.height};
                if(size != Vector2{0}) {  // ignore 0,0 size
                    engine->requestResolutionChange(size);
                }
            }
            break;

        case Expose:
        case MapNotify:
            if(engine != NULL && engine->isMinimized()) {
                engine->onRestored();
            }
            break;

        case KeymapNotify:
            XRefreshKeyboardMapping(&this->xev.xmapping);
            break;

        case FocusIn:
            this->bHasFocus = true;
            if(baseEnv != NULL) {
                baseEnv->invalidateMousePos();  // force refresh on focus
            }
            if(this->bRunning && engine != NULL) {
                engine->onFocusGained();
            }
            break;

        case FocusOut:
            this->bHasFocus = false;
            if(baseEnv != NULL) {
                baseEnv->invalidateMousePos();  // force refresh on unfocus
            }
            if(this->bRunning && engine != NULL) {
                engine->onFocusLost();
            }
            break;

        // clipboard
        case SelectionRequest:
            if(baseEnv != NULL) baseEnv->handleSelectionRequest(this->xev.xselectionrequest);
            break;

        // keyboard (TODO: move to xinput2)
        case KeyPress: {
            XKeyEvent *ke = &this->xev.xkey;
            // always use shifted keysym for consistency, needs to be same as in KeyRelease
            unsigned long keysym = XLookupKeysym(ke, 1);

            if(keyboard != NULL) {
                // normalize shifted/unshifted keysym
                KEYCODE normalizedKey = normalizeKeysym(keysym);
                keyboard->onKeyDown(normalizedKey);
            }

            // handle character input separately (this should still use modified lookup)
            constexpr int buffSize = 20;
            static std::array<char, buffSize> buf{};
            Status status = 0;
            KeySym textKeysym = 0;
            int length = Xutf8LookupString(this->ic, (XKeyPressedEvent *)&this->xev.xkey, buf.data(), buffSize,
                                           &textKeysym, &status);

            if(length > 0) {
                buf[buffSize - 1] = 0;
                if(keyboard != NULL) {
                    keyboard->onChar(buf[0]);
                }
            }
        } break;

        case KeyRelease: {
            XKeyEvent *ke = &this->xev.xkey;
            // always use shifted keysym for consistency, needs to be same as in KeyPress
            unsigned long keysym = XLookupKeysym(ke, 1);

            if(keyboard != NULL) {
                // LINUX: fuck X11 key repeats with release events inbetween
                if(XEventsQueued(this->dpy, QueuedAfterReading)) {
                    XEvent nextEvent;
                    XPeekEvent(this->dpy, &nextEvent);
                    if(nextEvent.type == KeyPress && nextEvent.xkey.time == ke->time &&
                       nextEvent.xkey.keycode == ke->keycode)
                        break;  // throw the event away
                }

                // normalize shifted/unshifted keysym
                KEYCODE normalizedKey = normalizeKeysym(keysym);
                keyboard->onKeyUp(normalizedKey);
            }
        } break;

        case ClientMessage:
            if(engine != NULL) engine->onShutdown();
            this->bRunning = false;
            break;

        default:
            if(cv::debug_env.getBool()) {
                debugLogF("unhandled X11 event type: {}\n", this->xev.type);
            }
            break;
    }
}

#endif
