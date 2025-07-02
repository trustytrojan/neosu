#ifdef __linux__

#include "XI2Handler.h"

#include "Engine.h"
#include "Mouse.h"

#include <X11/extensions/XInput2.h>

namespace XI2Handler {

// updated in main_Linux.cpp
int clientPointerDevID{0};

namespace {  // static

struct DevInfo {
    int deviceId{};
    int xAxisNumber{};
    int yAxisNumber{};
    int xAxisMode{XIModeRelative};  // or XIModeAbsolute
    int yAxisMode{XIModeRelative};
    double xAxisMin{}, xAxisMax{};
    double yAxisMin{}, yAxisMax{};
    double xScale{}, yScale{};  // scaling factors for absolute devices
    bool isValid{};
};

std::unordered_map<int, DevInfo> pointerDeviceCache{};

DevInfo queryXI2DevInfo(Display *display, int deviceId) {
    DevInfo info;
    info.deviceId = deviceId;

    int ndevices = 0;
    XIDeviceInfo *devices = XIQueryDevice(display, deviceId, &ndevices);

    if(!devices || ndevices == 0) {
        if(devices) XIFreeDeviceInfo(devices);
        return info;  // isValid remains false
    }

    XIDeviceInfo *device = &devices[0];

    // find x and y axis valuators
    for(int i = 0; i < device->num_classes; i++) {
        if(device->classes[i]->type == XIValuatorClass) {
            auto *valuator = reinterpret_cast<XIValuatorClassInfo *>(device->classes[i]);

            if(valuator->number == 0) {  // x axis
                info.xAxisNumber = valuator->number;
                info.xAxisMode = valuator->mode;
                info.xAxisMin = valuator->min;
                info.xAxisMax = valuator->max;
            } else if(valuator->number == 1) {  // y axis
                info.yAxisNumber = valuator->number;
                info.yAxisMode = valuator->mode;
                info.yAxisMin = valuator->min;
                info.yAxisMax = valuator->max;
            }
        }
    }

    XIFreeDeviceInfo(devices);

    // device is valid if we found both x and y axes
    info.isValid = (info.xAxisNumber >= 0 && info.yAxisNumber >= 0);

    if(info.isValid) {
        // pre-calculate scaling factors for absolute devices
        if(info.xAxisMode == XIModeAbsolute && info.xAxisMax > info.xAxisMin) {
            // scale to current window size, which will be updated when window resizes
            Vector2 windowSize = engine ? engine->getScreenSize() : Vector2(1280, 720);
            info.xScale = windowSize.x / (info.xAxisMax - info.xAxisMin);
        } else {
            info.xScale = 1.0;
        }

        if(info.yAxisMode == XIModeAbsolute && info.yAxisMax > info.yAxisMin) {
            Vector2 windowSize = engine ? engine->getScreenSize() : Vector2(1280, 720);
            info.yScale = windowSize.y / (info.yAxisMax - info.yAxisMin);
        } else {
            info.yScale = 1.0;
        }
    }

    return info;
}

void updateXI2DevInfo(Display *display, int deviceId) {
    DevInfo info = queryXI2DevInfo(display, deviceId);
    if(info.isValid) {
        pointerDeviceCache[deviceId] = info;
        if(cv_debug_env.getBool()) {
            debugLogF("updated device {:d}\n", deviceId);
        }
    } else {
        // device might have been removed
        auto it = pointerDeviceCache.find(deviceId);
        if(it != pointerDeviceCache.end()) {
            pointerDeviceCache.erase(it);
            if(cv_debug_env.getBool()) {
                debugLogF("removed device {:d} from cache\n", deviceId);
            }
        }
    }
}

void handleXI2DevChanged(Display *display, XIDeviceChangedEvent *event) {
    // update device info when device characteristics change
    updateXI2DevInfo(display, event->deviceid);

    // if this is our master pointer, we might need to update the client pointer id
    if(event->deviceid == clientPointerDevID) {
        // re-query client pointer in case it changed
        XIGetClientPointer(display, None, &clientPointerDevID);
    }
}

}  // namespace

void handleGenericEvent(Display *dpy, XEvent &xev) {
    XGetEventData(dpy, &xev.xcookie);

    // get the XI2 event type directly from the cookie
    int xi2EventType = xev.xcookie.evtype;
    // debugLogF("XI2 event type: {:d} (XI_RawMotion={:d})\n", xi2EventType, XI_RawMotion);

    switch(xi2EventType) {
        case XI_DeviceChanged: {
            auto *deviceEvent = static_cast<XIDeviceChangedEvent *>(xev.xcookie.data);
            handleXI2DevChanged(dpy, deviceEvent);
            break;
        }

        case XI_RawMotion: {
            auto *rawEvent = static_cast<XIRawEvent *>(xev.xcookie.data);
            // look up cached device info using sourceid
            int sourceid = rawEvent->sourceid;
            auto it = pointerDeviceCache.find(sourceid);
            if(it == pointerDeviceCache.end()) {
                // device not in cache, try to update it
                if(cv_debug_env.getBool()) {
                    debugLogF("unknown sourceid {}, attempting to cache\n", sourceid);
                }
                updateXI2DevInfo(dpy, sourceid);
                it = pointerDeviceCache.find(sourceid);
                if(it == pointerDeviceCache.end()) {
                    if(cv_debug_env.getBool()) {
                        debugLogF("failed to cache device {}, skipping event\n", sourceid);
                    }
                    break;
                }
            }

            const DevInfo &deviceInfo = it->second;
            if(!deviceInfo.isValid) {
                debugLogF("invalid device info for {}, skipping event\n", sourceid);
                break;
            }

            const bool isAbsolute = (deviceInfo.xAxisMode == XIModeAbsolute && deviceInfo.yAxisMode == XIModeAbsolute);

            const double *values = rawEvent->valuators.values;
            const double *raw_values = rawEvent->raw_values;

            double dx = 0, dy = 0;
            bool xProcessed = false, yProcessed = false;
            int valueIndex = 0;

            // process valuators for abs/rel modes, and scale absolute ones
            for(int i = 0; i <= std::max(deviceInfo.xAxisNumber, deviceInfo.yAxisNumber); i++) {
                if(!XIMaskIsSet(rawEvent->valuators.mask, i)) continue;

                if(i == deviceInfo.xAxisNumber) {
                    if(deviceInfo.xAxisMode == XIModeRelative) {
                        dx = raw_values[valueIndex];
                    } else {
                        dx = (values[valueIndex] - deviceInfo.xAxisMin) * deviceInfo.xScale;
                    }
                    xProcessed = true;
                }

                if(i == deviceInfo.yAxisNumber) {
                    if(deviceInfo.yAxisMode == XIModeRelative) {
                        dy = raw_values[valueIndex];
                    } else {
                        dy = (values[valueIndex] - deviceInfo.yAxisMin) * deviceInfo.yScale;
                    }
                    yProcessed = true;
                }

                valueIndex++;
            }

            if(xProcessed || yProcessed) {
                mouse->onRawMove(static_cast<float>(dx), static_cast<float>(dy), isAbsolute, false);
            }
            break;
        }
        default:
            break;
    }
    XFreeEventData(dpy, &xev.xcookie);
}

void updatePointerCache(Display *display) {
    pointerDeviceCache.clear();

    int ndevices = 0;
    XIDeviceInfo *devices = XIQueryDevice(display, XIAllDevices, &ndevices);

    if(!devices) {
        debugLogF("failed to query devices\n");
        return;
    }

    for(int i = 0; i < ndevices; i++) {
        XIDeviceInfo *device = &devices[i];
        // cache all pointer devices, but prioritize slave devices since those appear in sourceid
        // master devices are also cached as fallback
        if(device->use == XIMasterPointer || device->use == XISlavePointer) {
            DevInfo info = queryXI2DevInfo(display, device->deviceid);
            if(info.isValid) {
                pointerDeviceCache[device->deviceid] = info;
                if(cv_debug_env.getBool()) {
                    debugLogF(
                        "cached device {:d} ({:s}) [{:s}]: x_axis={:d} (mode={:s}), "
                        "y_axis={:d} (mode={:s})\n",
                        device->deviceid, device->name, (device->use == XIMasterPointer) ? "master" : "slave",
                        info.xAxisNumber, (info.xAxisMode == XIModeAbsolute) ? "abs" : "rel", info.yAxisNumber,
                        (info.yAxisMode == XIModeAbsolute) ? "abs" : "rel");
                }
            }
        }
    }

    XIFreeDeviceInfo(devices);
    if(cv_debug_env.getBool()) {
        debugLogF("cached {} devices\n", pointerDeviceCache.size());
    }
}

}  // namespace XI2Handler

#endif
