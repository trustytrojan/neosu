#ifdef __linux__

#include <cstdio>
#include <cstdlib>

#include "ConVar.h"
#include "Engine.h"
#include "LinuxEnvironment.h"
#include "LinuxGLLegacyInterface.h"
#include "XI2Handler.h"
#include "Profiler.h"
#include "Timing.h"
#include "cbase.h"

#include "Mouse.h"

#define XLIB_ILLEGAL_ACCESS
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput2.h>
#include <unistd.h>

#include "OpenGLHeaders.h"

#define WINDOW_TITLE "McEngine"

// TODO: this is incorrect, the size here doesn't take the decorations into account
#define WINDOW_WIDTH (1280)
#define WINDOW_HEIGHT (720)

#define WINDOW_WIDTH_MIN 100
#define WINDOW_HEIGHT_MIN 100

namespace {  // static

Engine *g_engine = NULL;
LinuxEnvironment *g_environment = NULL;

bool g_bRunning = true;
bool g_bUpdate = true;
bool g_bDraw = true;

bool g_bHasFocus = false;  // for fps_max_background

Display *dpy;
Window root;
// GLint                   att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
Colormap cmap;
XSetWindowAttributes swa;
Window win;
// GLXContext glc;
// XWindowAttributes gwa;
XEvent xev;

// input
XIM im;
XIC ic;
int xi2opcode;

//****************//
//	Message loop  //
//****************//

inline void WndProc() {
    switch(xev.type) {
        case GenericEvent:
            if(g_environment && mouse && xev.xcookie.extension == xi2opcode) {
                XI2Handler::handleGenericEvent(dpy, xev);
            }
            break;
        case MotionNotify:
            // update cached absolute position from regular motion events
            if(g_environment != NULL) {
                g_environment->updateMousePos(static_cast<float>(xev.xmotion.x), static_cast<float>(xev.xmotion.y));
            }
            break;

        case ConfigureNotify:
            if(g_engine != NULL) {
                g_engine->requestResolutionChange(Vector2(xev.xconfigure.width, xev.xconfigure.height));
                // update scaling factors for absolute devices when window size changes
                XI2Handler::updatePointerCache(dpy);
            }
            break;

        case KeymapNotify:
            XRefreshKeyboardMapping(&xev.xmapping);
            break;

        case FocusIn:
            g_bHasFocus = true;
            if(g_environment != NULL) {
                g_environment->invalidateMousePos();  // force refresh on focus
            }
            if(g_bRunning && g_engine != NULL) {
                g_engine->onFocusGained();
            }
            break;

        case FocusOut:
            g_bHasFocus = false;
            if(g_environment != NULL) {
                g_environment->invalidateMousePos();  // force refresh on unfocus
            }
            if(g_bRunning && g_engine != NULL) {
                g_engine->onFocusLost();
            }
            break;

        // clipboard
        case SelectionRequest:
            if(g_environment != NULL) g_environment->handleSelectionRequest(xev.xselectionrequest);
            break;

        // keyboard
        case KeyPress: {
            XKeyEvent *ke = &xev.xkey;
            unsigned long key = XLookupKeysym(
                ke, /*(ke->state&ShiftMask) ? 1 : 0*/ 1);  // WARNING: these two must be the same (see below)
            // printf("X: keyPress = %i\n", key);

            if(g_engine != NULL) {
                g_engine->onKeyboardKeyDown(key);
            }

            constexpr int buffSize = 20;
            std::array<char, buffSize> buf{};
            Status status = 0;
            KeySym keysym = 0;
            int length = Xutf8LookupString(ic, (XKeyPressedEvent *)&xev.xkey, buf.data(), buffSize, &keysym, &status);

            if(length > 0) {
                buf[buffSize - 1] = 0;
                // printf("buff = %s\n", buf);
                if(g_engine != NULL) g_engine->onKeyboardChar(buf[0]);
            }
        } break;

        case KeyRelease: {
            XKeyEvent *ke = &xev.xkey;
            unsigned long key = XLookupKeysym(
                ke, /*(ke->state & ShiftMask) ? 1 : 0*/ 1);  // WARNING: these two must be the same (see above)
            // printf("X: keyRelease = %i\n", key);

            if(g_engine != NULL) {
                // LINUX: fuck X11 key repeats with release events inbetween
                if(XEventsQueued(dpy, QueuedAfterReading)) {
                    XEvent nextEvent;
                    XPeekEvent(dpy, &nextEvent);
                    if(nextEvent.type == KeyPress && nextEvent.xkey.time == ke->time &&
                       nextEvent.xkey.keycode == ke->keycode)
                        break;  // throw the event away
                }

                g_engine->onKeyboardKeyUp(key);
            }
        } break;

        // mouse, also inject mouse 4 + 5 as keyboard keys
        case ButtonPress:
            if(g_engine != NULL && mouse != NULL) {
                switch(xev.xbutton.button) {
                    case Button1:
                        mouse->onLeftChange(true);
                        break;
                    case Button2:
                        mouse->onMiddleChange(true);
                        break;
                    case Button3:
                        mouse->onRightChange(true);
                        break;

                    case Button4:  // = mouse wheel up
                        mouse->onWheelVertical(120);
                        break;
                    case Button5:  // = mouse wheel down
                        mouse->onWheelVertical(-120);
                        break;

                    case 6:  // = mouse wheel left
                        mouse->onWheelHorizontal(-120);
                        break;
                    case 7:  // = mouse wheel right
                        mouse->onWheelHorizontal(120);
                        break;

                    case 8:                                               // mouse 4 (backwards)
                        g_engine->onKeyboardKeyDown(XK_Pointer_Button4);  // NOTE: abusing "dead vowels for universal
                                                                          // syllable entry", no idea what this key does
                        break;
                    case 9:                                               // mouse 5 (forwards)
                        g_engine->onKeyboardKeyDown(XK_Pointer_Button5);  // NOTE: abusing "dead vowels for universal
                                                                          // syllable entry", no idea what this key does
                        break;
                    default:
                        break;
                }
            }
            break;

        case ButtonRelease:
            if(g_engine != NULL && mouse != NULL) {
                switch(xev.xbutton.button) {
                    case Button1:
                        mouse->onLeftChange(false);
                        break;
                    case Button2:
                        mouse->onMiddleChange(false);
                        break;
                    case Button3:
                        mouse->onRightChange(false);
                        break;

                    case 8:                                             // mouse 4 (backwards)
                        g_engine->onKeyboardKeyUp(XK_Pointer_Button4);  // NOTE: abusing "dead vowels for universal
                                                                        // syllable entry", no idea what this key does
                        break;
                    case 9:                                             // mouse 5 (forwards)
                        g_engine->onKeyboardKeyUp(XK_Pointer_Button5);  // NOTE: abusing "dead vowels for universal
                                                                        // syllable entry", no idea what this key does
                        break;
                    default:
                        break;
                }
            }
            break;

        // destroy
        case ClientMessage:
            if(g_engine != NULL) g_engine->onShutdown();
            g_bRunning = false;
            break;
        default:
            break;
    }
}
}  // namespace

//********************//
//	Main entry point  //
//********************//

int main(int argc, char *argv[]) {
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
    // Fix path in case user is running it from the wrong folder.
    // We only do this if MCENGINE_DATA_DIR is set to its default value, since if it's changed,
    // the packager clearly wants the executable in a different location.
    if constexpr(ARRAY_SIZE(MCENGINE_DATA_DIR) == 3 && MCENGINE_DATA_DIR[0] == '.' && MCENGINE_DATA_DIR[1] == '/') {
        std::array<char, PATH_MAX> exe_path{};
        ssize_t exe_path_s = readlink("/proc/self/exe", exe_path.data(), exe_path.size() - 1);
        if(exe_path_s == -1) {
            perror("readlink");
            exit(1);
        }
        exe_path[exe_path_s] = '\0';

        char *last_slash = strrchr(exe_path.data(), '/');
        if(last_slash != NULL) {
            *last_slash = '\0';
        }

        if(chdir(exe_path.data()) != 0) {
            perror("chdir");
            exit(1);
        }
    }

    dpy = XOpenDisplay(NULL);
    if(dpy == NULL) {
        printf("FATAL ERROR: XOpenDisplay() can't connect to X server!\n\n");
        exit(1);
    }

    // before we do anything, check if XInput is available (for raw mouse input, smooth horizontal & vertical mouse
    // wheel etc.)
    int xi2firstEvent = -1, xi2error = -1;
    if(!XQueryExtension(dpy, "XInputExtension", &xi2opcode, &xi2firstEvent, &xi2error)) {
        printf("FATAL ERROR: XQueryExtension() XInput extension not available!\n\n");
        exit(1);
    }

    // want version 2 at least
    int ximajor = 2, ximinor = 0;
    if(XIQueryVersion(dpy, &ximajor, &ximinor) == BadRequest) {
        printf("FATAL ERROR: XIQueryVersion() XInput2 not available, server supports only %d.%d!\n\n", ximajor,
               ximinor);
        exit(1);
    }

    // debugLogF("XI2 version: {:d}.{:d}, opcode: {:d}, XI_RawMotion: {:d}\n", ximajor, ximinor, xi2opcode,
    // XI_RawMotion);

    root = DefaultRootWindow(dpy);

    // resolve GLX functions
    int screen = DefaultScreen(dpy);
    if(!gladLoadGLX(dpy, screen)) {
        printf("FATAL ERROR: Couldn't resolve GLX functions (gladLoadGLX() failed)!\n\n");
        exit(1);
    }

    int major = 0, minor = 0;
    glXQueryVersion(dpy, &major, &minor);
    debugLogF("GLX Version: {}.{}\n", major, minor);

    XVisualInfo *vi = getVisualInfo(dpy);
    if(!vi) {
        printf("FATAL ERROR: Couldn't glXChooseVisual!\n\n");
        exit(1);
    }

    cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

    // set window attributes
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | FocusChangeMask | KeyPressMask | KeyReleaseMask | ButtonPressMask |
                     ButtonReleaseMask | PointerMotionMask | StructureNotifyMask | KeymapStateMask;

    Screen *defaultScreen = DefaultScreenOfDisplay(dpy);
    win = XCreateWindow(dpy, root, defaultScreen->width / 2 - WINDOW_WIDTH / 2,
                        defaultScreen->height / 2 - WINDOW_HEIGHT / 2, WINDOW_WIDTH, WINDOW_HEIGHT, 0, vi->depth,
                        InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

    XFree(vi);

    // window resize limit
    XSizeHints *winSizeHints = XAllocSizeHints();
    if(!winSizeHints) {
        printf("FATAL ERROR: XAllocSizeHints() out of memory!\n\n");
        exit(1);
    }
    winSizeHints->flags = PMinSize;
    winSizeHints->min_width = WINDOW_WIDTH_MIN;
    winSizeHints->min_height = WINDOW_HEIGHT_MIN;
    XSetWMNormalHints(dpy, win, winSizeHints);
    XFree(winSizeHints);

    // register custom delete message for quitting the engine (received as ClientMessage in the main loop)
    Atom wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", false);
    XSetWMProtocols(dpy, win, &wm_delete_window, 1);

    // hint that compositing should be disabled (disable forced vsync)
    constexpr const unsigned char shouldBypassCompositor = 1;
    Atom _net_wm_bypass_compositor = XInternAtom(dpy, "_NET_WM_BYPASS_COMPOSITOR", false);
    XChangeProperty(dpy, win, _net_wm_bypass_compositor, XA_CARDINAL, 32, PropModeReplace, &shouldBypassCompositor, 1);

    // make window visible & set title
    XMapWindow(dpy, win);
    XStoreName(dpy, win, WINDOW_TITLE);

    // after the window is visible, center it again (if the window manager ignored the position of the window in
    // XCreateWindow(), because fuck you)
    XMoveWindow(dpy, win, defaultScreen->width / 2 - WINDOW_WIDTH / 2, defaultScreen->height / 2 - WINDOW_HEIGHT / 2);

    // get input method
    im = XOpenIM(dpy, NULL, NULL, NULL);
    if(im == NULL) {
        printf("FATAL ERROR: XOpenIM() couldn't open input method!\n\n");
        exit(1);
    }

    // get input context
    ic = XCreateIC(im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, win, NULL);
    if(ic == NULL) {
        printf("FATAL ERROR: XCreateIC() couldn't create input context!\n\n");
        exit(1);
    }

    XSync(dpy, False);

    // discover and cache pointer device information
    XI2Handler::updatePointerCache(dpy);

    // set XInput event masks
    XIEventMask mask;
    std::array<unsigned char, XIMaskLen(XI_LASTEVENT)> mask_bits{};

    XISetMask(mask_bits.data(), XI_DeviceChanged);
    XISetMask(mask_bits.data(), XI_RawMotion);

    // listen to all master devices to get proper source device information
    // this allows us to distinguish between different input devices (mouse vs tablet)
    XIGetClientPointer(dpy, None, &XI2Handler::clientPointerDevID);

    mask.mask_len = mask_bits.size();
    mask.mask = mask_bits.data();
    mask.deviceid = XIAllMasterDevices;

    // and select it on the window
    XISelectEvents(dpy, DefaultRootWindow(dpy), &mask, 1);

    // get keyboard focus
    XSetICFocus(ic);

    XSync(dpy, False);

    // create timers
    auto *frameTimer = new Timer();
    frameTimer->start();
    frameTimer->update();

    auto *deltaTimer = new Timer();
    deltaTimer->start();
    deltaTimer->update();

    // initialize engine
    auto *environment = new LinuxEnvironment(dpy, win);
    g_environment = environment;
    g_engine = new Engine(environment, argc, argv);
    g_engine->loadApp();

    frameTimer->update();
    deltaTimer->update();

    // main loop
    while(g_bRunning) {
        VPROF_MAIN();

        // handle window message queue
        while(XPending(dpy)) {
            XNextEvent(dpy, &xev);

            if(XFilterEvent(&xev, win))  // for keyboard chars
                continue;

            WndProc();
        }

        // update
        {
            deltaTimer->update();
            engine->setFrameTime(deltaTimer->getDelta());

            if(g_bUpdate) g_engine->onUpdate();
        }

        // draw
        if(g_bDraw) {
            g_engine->onPaint();
        }

        // delay the next frame
        frameTimer->update();
        const bool inBackground = /*g_bMinimized ||*/ !g_bHasFocus;
        if((!cv_fps_unlimited.getBool() && cv_fps_max.getInt() > 0) || inBackground) {
            double delayStart = frameTimer->getElapsedTime();
            double delayTime = 0;
            if(inBackground)
                delayTime = (1.f / cv_fps_max_background.getFloat()) - frameTimer->getDelta();
            else
                delayTime = (1.f / cv_fps_max.getFloat()) - frameTimer->getDelta();

            while(delayTime > 0.0) {
                if(inBackground)  // real waiting (very inaccurate, but very good for little background cpu utilization)
                    Timing::sleepMS((1.f / cv_fps_max_background.getFloat()) * 1000.0f);
                else  // more or less "busy" waiting, but giving away the rest of the timeslice at least
                    Timing::sleep(0);

                // decrease the delayTime by the time we spent in this loop
                // if the loop is executed more than once, note how delayStart now gets the value of the previous
                // iteration from getElapsedTime() this works because the elapsed time is only updated in update(). now
                // we can easily calculate the time the Sleep() took and subtract it from the delayTime
                delayStart = frameTimer->getElapsedTime();
                frameTimer->update();
                delayTime -= (frameTimer->getElapsedTime() - delayStart);
            }
        }
    }

    // release the engine
    SAFE_DELETE(g_engine);

    // destroy the window
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);

    // unload GLX
    gladUnloadGLX();

    return 0;
}

#endif
