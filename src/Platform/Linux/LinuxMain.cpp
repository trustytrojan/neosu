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
#include "cbase.h"

#define XLIB_ILLEGAL_ACCESS
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput2.h>
#include <unistd.h>

#include "OpenGLHeaders.h"

#define WINDOW_TITLE "neosu"

// TODO: this is incorrect, the size here doesn't take the decorations into account
#define WINDOW_WIDTH (1280)
#define WINDOW_HEIGHT (720)

#define WINDOW_WIDTH_MIN 100
#define WINDOW_HEIGHT_MIN 100

//********************//
//	Main entry point  //
//********************//

LinuxMain::LinuxMain(int argc, char *argv[], const std::vector<UString> & /*argCmdline*/,
                             const std::unordered_map<UString, std::optional<UString>> & /*argMap*/) {
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

    XVisualInfo *vi = getVisualInfo(this->dpy);
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
                        defaultScreen->height / 2 - WINDOW_HEIGHT / 2, WINDOW_WIDTH, WINDOW_HEIGHT, 0, vi->depth,
                        InputOutput, vi->visual, CWColormap | CWEventMask, &this->swa);

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
    XChangeProperty(this->dpy, this->clientWindow, _net_wm_bypass_compositor, XA_CARDINAL, 32, PropModeReplace, &shouldBypassCompositor, 1);

    // make window visible & set title
    XMapWindow(this->dpy, this->clientWindow);
    XStoreName(this->dpy, this->clientWindow, WINDOW_TITLE);

    // after the window is visible, center it again (if the window manager ignored the position of the window in
    // XCreateWindow(), because fuck you)
    XMoveWindow(this->dpy, this->clientWindow, defaultScreen->width / 2 - WINDOW_WIDTH / 2, defaultScreen->height / 2 - WINDOW_HEIGHT / 2);

    // get input method
    this->im = XOpenIM(this->dpy, nullptr, nullptr, nullptr);
    if(this->im == nullptr) {
        printf("FATAL ERROR: XOpenIM() couldn't open input method!\n\n");
        return;
    }

    // get input context
    this->ic = XCreateIC(this->im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, this->clientWindow, NULL);
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
    auto *frameTimer = new Timer();
    frameTimer->start();
    frameTimer->update();

    auto *deltaTimer = new Timer();
    deltaTimer->start();
    deltaTimer->update();

    // initialize engine
    baseEnv = new LinuxEnvironment(this->dpy, this->clientWindow);
    engine = new Engine(argc, argv);
    engine->loadApp();

    frameTimer->update();
    deltaTimer->update();

    // main loop
    while(this->bRunning) {
        VPROF_MAIN();

        // handle window message queue
        while(XPending(this->dpy)) {
            XNextEvent(this->dpy, &this->xev);

            if(XFilterEvent(&this->xev, this->clientWindow))  // for keyboard chars
                continue;

            WndProc();
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
        frameTimer->update();
        const bool inBackground = /*g_bMinimized ||*/ !this->bHasFocus;
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
    SAFE_DELETE(engine);

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

void LinuxMain::WndProc() {
    switch(this->xev.type) {
        case GenericEvent:
            if(baseEnv && this->xev.xcookie.extension == this->xi2opcode) {
                XI2Handler::handleGenericEvent(this->dpy, this->xev);
            }
            break;

        case ConfigureNotify:
            if(engine != NULL) {
                engine->requestResolutionChange(Vector2(this->xev.xconfigure.width, this->xev.xconfigure.height));
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
            unsigned long key = XLookupKeysym(
                ke, /*(ke->state&ShiftMask) ? 1 : 0*/ 1);  // WARNING: these two must be the same (see below)
            // printf("X: keyPress = %i\n", key);

            if(engine != NULL) {
                engine->onKeyboardKeyDown(key);
            }

            constexpr int buffSize = 20;
            std::array<char, buffSize> buf{};
            Status status = 0;
            KeySym keysym = 0;
            int length = Xutf8LookupString(this->ic, (XKeyPressedEvent *)&this->xev.xkey, buf.data(), buffSize, &keysym, &status);

            if(length > 0) {
                buf[buffSize - 1] = 0;
                // printf("buff = %s\n", buf);
                if(engine != NULL) engine->onKeyboardChar(buf[0]);
            }
        } break;

        case KeyRelease: {
            XKeyEvent *ke = &this->xev.xkey;
            unsigned long key = XLookupKeysym(
                ke, /*(ke->state & ShiftMask) ? 1 : 0*/ 1);  // WARNING: these two must be the same (see above)
            // printf("X: keyRelease = %i\n", key);

            if(engine != NULL) {
                // LINUX: fuck X11 key repeats with release events inbetween
                if(XEventsQueued(this->dpy, QueuedAfterReading)) {
                    XEvent nextEvent;
                    XPeekEvent(this->dpy, &nextEvent);
                    if(nextEvent.type == KeyPress && nextEvent.xkey.time == ke->time &&
                       nextEvent.xkey.keycode == ke->keycode)
                        break;  // throw the event away
                }

                engine->onKeyboardKeyUp(key);
            }
        } break;

        case ClientMessage:
            if(engine != NULL) engine->onShutdown();
            this->bRunning = false;
            break;

        default:
            break;
    }
}

#endif
