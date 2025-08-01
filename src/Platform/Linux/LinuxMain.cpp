
#ifdef __linux__

#include <cstdio>
#include <cstdlib>
#include <utility>

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

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_events.h>  // for SDL_PumpEvents, needed for SDL_ShowOpenFileDialog/SDL_ShowOpenFolderDialog
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_system.h>
#include <SDL3/SDL_log.h>

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
    SDL_SetHintWithPriority(SDL_HINT_VIDEO_DOUBLE_BUFFER, "1", SDL_HINT_OVERRIDE);
    SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "x11", SDL_HINT_OVERRIDE);
    SDL_SetHintWithPriority(SDL_HINT_MOUSE_AUTO_CAPTURE, "0", SDL_HINT_OVERRIDE);
    SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER, "0", SDL_HINT_OVERRIDE);
    SDL_SetHintWithPriority(SDL_HINT_TOUCH_MOUSE_EVENTS, "0", SDL_HINT_OVERRIDE);
    SDL_SetHintWithPriority(SDL_HINT_MOUSE_EMULATE_WARP_WITH_RELATIVE, "0", SDL_HINT_OVERRIDE);
    SDL_SetHintWithPriority(SDL_HINT_VIDEO_X11_EXTERNAL_WINDOW_INPUT, "0", SDL_HINT_OVERRIDE);

    if(!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "FATAL ERROR: Couldn't SDL_Init(): %s\n", SDL_GetError());
        return;
    }
    // don't want em
    SDL_QuitSubSystem(SDL_INIT_EVENTS);

    if constexpr(Env::cfg((REND::GL | REND::GLES32), !REND::DX11)) {
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            Env::cfg(REND::GL) ? SDL_GL_CONTEXT_PROFILE_COMPATIBILITY : SDL_GL_CONTEXT_PROFILE_ES);
        if constexpr(!Env::cfg(REND::GL)) {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        }
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    }

    // get default monitor resolution and create the window with that as the starting size
    long windowCreateWidth = WINDOW_WIDTH;
    long windowCreateHeight = WINDOW_HEIGHT;
    {
        SDL_DisplayID di = SDL_GetPrimaryDisplay();
        const SDL_DisplayMode *dm = nullptr;
        if(di && (dm = SDL_GetDesktopDisplayMode(di))) {
            windowCreateWidth = dm->w;
            windowCreateHeight = dm->h;
        }
    }

    // set vulkan for linux dxvk-native, opengl otherwise
    constexpr auto windowFlags =
        SDL_WINDOW_HIDDEN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS |
        (Env::cfg((REND::GL | REND::GLES32)) ? SDL_WINDOW_OPENGL : (Env::cfg(REND::DX11) ? SDL_WINDOW_VULKAN : 0UL));

    SDL_PropertiesID props = SDL_CreateProperties();
    // if constexpr (Env::cfg(REND::DX11))
    // 	SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN, true);
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, WINDOW_TITLE);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, windowCreateWidth);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, windowCreateHeight);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_MAXIMIZED_BOOLEAN, false);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, windowFlags);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, true);

    // create window
    SDL_Window *sdlwnd = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);

    if(!sdlwnd) {
        fprintf(stderr, "FATAL ERROR: Couldn't SDL_CreateWindow(): %s\n", SDL_GetError());
        return;
    }

    SDL_GLContext sdlglctx{nullptr};
    if constexpr(Env::cfg((REND::GL | REND::GLES32), !REND::DX11)) {
        sdlglctx = SDL_GL_CreateContext(sdlwnd);
        if(!sdlglctx) {
            fprintf(stderr, "FATAL ERROR: Couldn't SDL_GL_CreateContext(): %s\n", SDL_GetError());
        }
        if(!SDL_GL_MakeCurrent(sdlwnd, sdlglctx)) {
            fprintf(stderr, "FATAL ERROR: Couldn't SDL_GL_MakeCurrent(): %s\n", SDL_GetError());
        }
    }

    this->dpy = (Display *)SDL_GetPointerProperty(SDL_GetWindowProperties(sdlwnd), SDL_PROP_WINDOW_X11_DISPLAY_POINTER,
                                                  nullptr);
    if(!this->dpy) {
        fprintf(stderr, "FATAL ERROR: Couldn't get X11 display: %s\n", SDL_GetError());
        return;
    }

    this->clientWindow =
        (Window)SDL_GetNumberProperty(SDL_GetWindowProperties(sdlwnd), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    if(!this->clientWindow) {
        fprintf(stderr, "FATAL ERROR: Couldn't create X11 window: %s\n", SDL_GetError());
        return;
    }

    int xi2firstEvent = -1, xi2error = -1;
    // get xi2opcode
    if(!XQueryExtension(this->dpy, "XInputExtension", &this->xi2opcode, &xi2firstEvent, &xi2error)) {
        fprintf(stderr, "FATAL ERROR: XQueryExtension() XInput extension not available!\n\n");
        return;
    }

    // don't let sdl handle events
    SDL_SetX11EventHook(LinuxMain::sdl_eventhook, this);

    this->screen_num =
        (int)SDL_GetNumberProperty(SDL_GetWindowProperties(sdlwnd), SDL_PROP_WINDOW_X11_SCREEN_NUMBER, 0);
    this->screen = ScreenOfDisplay(this->dpy, this->screen_num);

    XWindowAttributes xwa{};
    XGetWindowAttributes(this->dpy, this->clientWindow, &xwa);

    XSetWindowAttributes swa{};
    swa.colormap = xwa.colormap;
    swa.event_mask =
        ExposureMask | FocusChangeMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | KeymapStateMask;

    XChangeWindowAttributes(
        this->dpy, this->clientWindow,
        CWOverrideRedirect | CWBackPixmap | CWBorderPixel | CWBackingStore | CWColormap | CWEventMask, &swa);

    XSelectInput(
        this->dpy, this->clientWindow,
        ExposureMask | FocusChangeMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | KeymapStateMask);

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
    this->WM_DELETE_WINDOW_atom = XInternAtom(this->dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(this->dpy, this->clientWindow, &this->WM_DELETE_WINDOW_atom, 1);
    this->WM_PROTOCOLS_atom = XInternAtom(this->dpy, "WM_PROTOCOLS", False);

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
    SDL_ShowWindow(sdlwnd);
    SDL_RaiseWindow(sdlwnd);

    XStoreName(this->dpy, this->clientWindow, WINDOW_TITLE);  // four.

    // after the window is visible, center it again (if the window manager ignored the position of the window in
    // XCreateWindow(), because fuck you)
    // debugLogF("moving to {} {}\n",  defaultScreen->width / 2 - WINDOW_WIDTH / 2,
    //             defaultScreen->height / 2 - WINDOW_HEIGHT / 2);
    {
        const SDL_DisplayID di = SDL_GetDisplayForWindow(sdlwnd);
        SDL_SetWindowPosition(sdlwnd, SDL_WINDOWPOS_CENTERED_DISPLAY(di), SDL_WINDOWPOS_CENTERED_DISPLAY(di));
    }

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

    // sets client pointer device and device cache
    XI2Handler::updateDeviceCache(this->dpy, this->clientWindow);

    // select xi2 events
    XI2Handler::selectEvents(this->dpy, this->clientWindow, DefaultRootWindow(this->dpy));

    // get keyboard focus
    XSetICFocus(this->ic);

    XSync(this->dpy, False);

    // create timers
    auto *deltaTimer = new Timer();

    // initialize engine
    baseEnv = new LinuxEnvironment(this->dpy, this->clientWindow, this->screen, sdlwnd, argCmdline, argMap);
    engine = new Engine(argc, argv);

    deltaTimer->start();
    deltaTimer->update();

    engine->loadApp();

    // sdl gets stuck somewhere unless we do this
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);

    XEvent xev;

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
                XNextEvent(this->dpy, &xev);

                if(XFilterEvent(&xev, this->clientWindow))  // for keyboard chars
                    continue;

                WndProc(&xev);
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
            const int target_fps = !this->bHasFocus      ? cv::fps_max_background.getInt()
                                   : osu->isInPlayMode() ? cv::fps_max.getInt()
                                                         : cv::fps_max_menu.getInt();
            FPSLimiter::limit_frames(target_fps);
        }
    }

    // release the engine
    SAFE_DELETE(engine);

    // clean up environment
    SAFE_DELETE(baseEnv);

    // release timer
    SAFE_DELETE(deltaTimer);

    // destroy the gl context and window
    if(sdlglctx) SDL_GL_DestroyContext(sdlglctx);
    SDL_DestroyWindow(sdlwnd);
    SDL_Quit();

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

bool LinuxMain::WndProc(XEvent *xev) {
    switch(xev->type) {
        case GenericEvent:
            if(baseEnv && xev->xcookie.extension == this->xi2opcode) {
                XI2Handler::handleGenericEvent(this->dpy, xev);
            }
            return true;

        case ConfigureNotify:
            if(engine != NULL) {
                const Vector2 size{xev->xconfigure.width, xev->xconfigure.height};
                if(size != Vector2{0}) {  // ignore 0,0 size
                    engine->requestResolutionChange(size);
                }
            }
            return true;

        case Expose:
        case MapNotify:
            if(engine != NULL && engine->isMinimized()) {
                engine->onRestored();
            }
            return true;

        case KeymapNotify:
            XRefreshKeyboardMapping(&xev->xmapping);
            return true;

        case FocusIn:
            this->bHasFocus = true;
            if(baseEnv != NULL) {
                baseEnv->invalidateMousePos();  // force refresh on focus
            }
            if(this->bRunning && engine != NULL) {
                engine->onFocusGained();
            }
            return true;

        case FocusOut:
            this->bHasFocus = false;
            if(baseEnv != NULL) {
                baseEnv->invalidateMousePos();  // force refresh on unfocus
            }
            if(this->bRunning && engine != NULL) {
                engine->onFocusLost();
            }
            return true;

        // clipboard
        case SelectionRequest:
            if(baseEnv != NULL) baseEnv->handleSelectionRequest(xev->xselectionrequest);
            return true;

        // keyboard (TODO: move to xinput2)
        case KeyPress: {
            XKeyEvent *ke = &xev->xkey;
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
            int length =
                Xutf8LookupString(this->ic, (XKeyPressedEvent *)&xev->xkey, buf.data(), buffSize, &textKeysym, &status);

            if(length > 0) {
                buf[buffSize - 1] = 0;
                if(keyboard != NULL) {
                    keyboard->onChar(buf[0]);
                }
            }
        }
            return true;

        case KeyRelease: {
            XKeyEvent *ke = &xev->xkey;
            // always use shifted keysym for consistency, needs to be same as in KeyPress
            unsigned long keysym = XLookupKeysym(ke, 1);

            if(keyboard != NULL) {
                // LINUX: fuck X11 key repeats with release events inbetween
                if(XEventsQueued(this->dpy, QueuedAfterReading)) {
                    XEvent nextEvent;
                    XPeekEvent(this->dpy, &nextEvent);
                    if(nextEvent.type == KeyPress && nextEvent.xkey.time == ke->time &&
                       nextEvent.xkey.keycode == ke->keycode)
                        return true;  // throw the event away
                }

                // normalize shifted/unshifted keysym
                KEYCODE normalizedKey = normalizeKeysym(keysym);
                keyboard->onKeyUp(normalizedKey);
            }
        }
            return true;

        case ClientMessage:
            if((xev->xclient.message_type == this->WM_PROTOCOLS_atom) && (xev->xclient.format == 32) &&
               (std::cmp_equal(xev->xclient.data.l[0], this->WM_DELETE_WINDOW_atom))) {
                if(engine != NULL) engine->onShutdown();
                this->bRunning = false;
                return true;
            } else {
                return false;
            }
        default:
            if(cv::debug_env.getBool()) {
                debugLogF("unhandled X11 event type: {}\n", xev->type);
            }
            break;
    }
    return false;
}

#endif
