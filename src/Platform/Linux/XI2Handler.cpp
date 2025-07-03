#ifdef __linux__

#include "LinuxMain.h"
#include "XI2Handler.h"

#include "Engine.h"
#include "Mouse.h"

#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>

namespace XI2Handler {

// set in main_Linux.cpp
int clientPointerDevID = 0;

namespace {  // static

struct DeviceInfo {
    bool isAbsolute = false;  // true if both x and y axes are absolute
    bool isValid = false;
};

std::unordered_map<int, DeviceInfo> deviceCache;

// grab state management

// dumb XInput2 button press implicit grab bug:
// https://web.archive.org/web/20250315031818/https://bugs.freedesktop.org/show_bug.cgi?id=26922

bool g_explicitGrab = false;  // user-requested grab (setCursorClip)

static void cacheDeviceInfo(Display *display, int deviceId) {
    int ndevices = 0;
    XIDeviceInfo *devices = XIQueryDevice(display, deviceId, &ndevices);

    if(!devices || ndevices == 0) {
        if(devices) XIFreeDeviceInfo(devices);
        return;
    }

    XIDeviceInfo *device = &devices[0];
    DeviceInfo info;
    bool hasAbsoluteX = false, hasAbsoluteY = false;

    // check if device has absolute x and y axes
    for(int i = 0; i < device->num_classes; i++) {
        if(device->classes[i]->type == XIValuatorClass) {
            auto *valuator = reinterpret_cast<XIValuatorClassInfo *>(device->classes[i]);
            if(valuator->number == 0 && valuator->mode == XIModeAbsolute) {
                hasAbsoluteX = true;
            } else if(valuator->number == 1 && valuator->mode == XIModeAbsolute) {
                hasAbsoluteY = true;
            }
        }
    }

    info.isAbsolute = hasAbsoluteX && hasAbsoluteY;
    info.isValid = true;
    deviceCache[deviceId] = info;

    XIFreeDeviceInfo(devices);
}

}  // namespace

bool grab(Display *display, bool enable, bool grabHack) {
    if(enable || grabHack) {
        // don't grab if already grabbed
        if(g_explicitGrab) {
            return true;
        }

        // create event mask for grab (all currently handled XI2 events)
        XIEventMask mask;
        std::array<unsigned char, XIMaskLen(XI_LASTEVENT)> maskBits{};
        XISetMask(maskBits.data(), XI_RawMotion);
        XISetMask(maskBits.data(), XI_Motion);
        XISetMask(maskBits.data(), XI_ButtonPress);
        XISetMask(maskBits.data(), XI_ButtonRelease);
        XISetMask(maskBits.data(), XI_DeviceChanged);
        mask.deviceid = clientPointerDevID;
        mask.mask_len = maskBits.size();
        mask.mask = maskBits.data();

        int ret = XIGrabDevice(display, clientPointerDevID, DefaultRootWindow(display), CurrentTime, None,
                               GrabModeAsync, GrabModeAsync, False, &mask);
        if(ret == GrabSuccess || ret == AlreadyGrabbed) {
            if(!grabHack) {
                g_explicitGrab = true;
            }
            XFlush(display);
            if(grabHack) {
                XIUngrabDevice(display, clientPointerDevID, CurrentTime);
                XFlush(display);
                return false;
            }
            return true;
        }
        debugLogF("failed to grab: {}\n", ret);
        return false;
    } else {
        g_explicitGrab = false;

        XIUngrabDevice(display, clientPointerDevID, CurrentTime);
        XFlush(display);
        return false;
    }
}

void handleGenericEvent(Display *dpy, XEvent &xev) {
    XGetEventData(dpy, &xev.xcookie);

    switch(xev.xcookie.evtype) {
        case XI_Motion: {
            auto *event = static_cast<XIDeviceEvent *>(xev.xcookie.data);

            // update cached position for master devices
            if(baseEnv != nullptr) {
                baseEnv->updateMousePos(event->event_x, event->event_y);
            }
            break;
        }

        /* TODO: handle
        case XI_RawButtonPress:
        case XI_RawButtonRelease: */
        case XI_ButtonPress:
        case XI_ButtonRelease: {
            auto *event = static_cast<XIDeviceEvent *>(xev.xcookie.data);

            if(mouse != nullptr) {
                bool pressed = (xev.xcookie.evtype == XI_ButtonPress);

                // workaround for XI2 server bug: grab-ungrab device during button events
                // to ensure we receive raw motion events (freedesktop.org bug #26922)
                if(pressed && !g_explicitGrab) grab(dpy, true, true);

                switch(event->detail) {
                    case 1:  // left button
                        mouse->onLeftChange(pressed);
                        break;
                    case 2:  // middle button
                        mouse->onMiddleChange(pressed);
                        break;
                    case 3:  // right button
                        mouse->onRightChange(pressed);
                        break;
                    case 4:  // wheel up
                        if(pressed) mouse->onWheelVertical(120);
                        break;
                    case 5:  // wheel down
                        if(pressed) mouse->onWheelVertical(-120);
                        break;
                    case 6:  // wheel left
                        if(pressed) mouse->onWheelHorizontal(-120);
                        break;
                    case 7:  // wheel right
                        if(pressed) mouse->onWheelHorizontal(120);
                        break;
                    case 8:  // mouse 4 (back)
                        if(engine != nullptr) {
                            if(pressed)
                                engine->onKeyboardKeyDown(XK_Pointer_Button4);
                            else
                                engine->onKeyboardKeyUp(XK_Pointer_Button4);
                        }
                        break;
                    case 9:  // mouse 5 (forwards)
                        // NOTE: abusing "dead vowels for universal
                        // syllable entry", no idea what this key does
                        if(engine != nullptr) {
                            if(pressed)
                                engine->onKeyboardKeyDown(XK_Pointer_Button5);
                            else
                                engine->onKeyboardKeyUp(XK_Pointer_Button5);
                        }
                        break;
                }
            }
            break;
        }

        case XI_RawMotion: {
            auto *event = static_cast<XIRawEvent *>(xev.xcookie.data);

            if(mouse != nullptr) {
                // check device cache first
                int sourceid = event->sourceid;
                auto it = deviceCache.find(sourceid);
                if(it == deviceCache.end()) {
                    // cache device info if not found
                    cacheDeviceInfo(dpy, sourceid);
                    it = deviceCache.find(sourceid);
                    if(it == deviceCache.end()) {
                        break;  // failed to cache device info
                    }
                }

                const DeviceInfo &devInfo = it->second;
                if(!devInfo.isValid) {
                    break;
                }

                double dx = 0.0, dy = 0.0;
                bool hasX = false, hasY = false;

                // use raw_values for relative devices, values for absolute devices
                const double *values = devInfo.isAbsolute ? event->valuators.values : event->raw_values;
                int valueIndex = 0;

                for(int i = 0; i < event->valuators.mask_len * 8; i++) {
                    if(XIMaskIsSet(event->valuators.mask, i)) {
                        if(i == 0 && !hasX) {  // x axis
                            dx = values[valueIndex];
                            hasX = true;
                        } else if(i == 1 && !hasY) {  // y axis
                            dy = values[valueIndex];
                            hasY = true;
                        }
                        valueIndex++;
                    }
                }

                if(hasX || hasY) {
                    mouse->onRawMove(dx, dy, devInfo.isAbsolute, false);
                }
            }
            break;
        }

            /* TODO: handle
            case XI_RawKeyPress:
            case XI_RawKeyRelease: */
            // case XI_KeyPress:
            // case XI_KeyRelease: {
            //     auto *event = static_cast<XIDeviceEvent *>(xev.xcookie.data);

            //     if(engine != nullptr) {
            //         bool pressed = (xev.xcookie.evtype == XI_KeyPress);
            //         if(pressed) {
            //             engine->onKeyboardKeyDown(event->detail);
            //         } else {
            //             engine->onKeyboardKeyUp(event->detail);
            //         }
            //     }
            //     break;
            // }

        case XI_DeviceChanged: {
            auto *event = static_cast<XIDeviceChangedEvent *>(xev.xcookie.data);
            if(event->reason == XIDeviceChange)  // we don't need to update the cache for XISlaveSwitch
                cacheDeviceInfo(dpy, event->deviceid);
            break;
        }

            /*
            // maybe TODO? low priority
            case XI_Enter:
            case XI_Leave: {
                // handle enter/leave events if needed for cursor management
                break;
            } */

        default:
            break;
    }

    XFreeEventData(dpy, &xev.xcookie);
}

void selectEvents(Display *display, Window window, int root_window) {
    std::array<XIEventMask, 2> evmasks{};
    std::array<unsigned char, XIMaskLen(XI_LASTEVENT)> mask1{};
    std::array<unsigned char, XIMaskLen(XI_LASTEVENT)> mask2{};

    // select device events for the application window
    XISetMask(mask1.data(), XI_Motion);
    XISetMask(mask1.data(), XI_ButtonPress);
    XISetMask(mask1.data(), XI_ButtonRelease);
    // XISetMask(mask1.data(), XI_KeyPress); // TODO: incomplete
    // XISetMask(mask1.data(), XI_KeyRelease);
    // XISetMask(mask1.data(), XI_Enter);
    // XISetMask(mask1.data(), XI_Leave);
    XISetMask(mask1.data(), XI_DeviceChanged);

    evmasks[0].deviceid = XIAllMasterDevices;
    evmasks[0].mask_len = mask1.size();
    evmasks[0].mask = mask1.data();

    // select raw events for the root window
    // raw events are only delivered to root windows
    XISetMask(mask2.data(), XI_RawMotion);
    // XISetMask(mask2.data(), XI_RawButtonPress); // TODO: incomplete
    // XISetMask(mask2.data(), XI_RawButtonRelease);
    // XISetMask(mask2.data(), XI_RawKeyPress);
    // XISetMask(mask2.data(), XI_RawKeyRelease);

    evmasks[1].deviceid = XIAllMasterDevices;
    evmasks[1].mask_len = mask2.size();
    evmasks[1].mask = mask2.data();

    // apply event masks
    XISelectEvents(display, window, &evmasks[0], 1);
    XISelectEvents(display, root_window, &evmasks[1], 1);

    XFlush(display);
}

int getClientPointer(Display *display) {
    int deviceid = -1;
    if(XIGetClientPointer(display, None, &deviceid)) {
        return deviceid;
    }
    return 2;  // fallback to default VCP
}

void updateDeviceCache(Display *display) {
    // clear existing cache
    deviceCache.clear();

    // update client pointer
    clientPointerDevID = getClientPointer(display);

    // cache all current pointer devices
    int ndevices = 0;
    XIDeviceInfo *devices = XIQueryDevice(display, XIAllDevices, &ndevices);

    if(!devices) return;

    for(int i = 0; i < ndevices; i++) {
        XIDeviceInfo *device = &devices[i];
        if(device->use == XIMasterPointer || device->use == XISlavePointer) {
            cacheDeviceInfo(display, device->deviceid);
        }
    }

    XIFreeDeviceInfo(devices);
}

}  // namespace XI2Handler

#endif
