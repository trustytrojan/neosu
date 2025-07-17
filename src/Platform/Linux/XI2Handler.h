#pragma once

#ifdef __linux__

typedef struct _XDisplay Display;
typedef union _XEvent XEvent;

typedef unsigned long XID;
typedef XID Window;

namespace XI2Handler {

// handle xi2 events for the application window
void handleGenericEvent(Display *display, XEvent &xev);

// select xi2 events for the application window and raw events for the root window
void selectEvents(Display *display, Window window, int root_window);

// get the client pointer device id
int getClientPointer(Display *display);

// update device cache when devices change
void updateDeviceCache(Display *display);

// XI2 pointer grab wrapper for explicit (cursor clip) and implicit (button press) grabs
// returns true if grab is active
bool grab(Display *display, Window window, bool enable, bool grabHack = false);

extern int clientPointerDevID;

}  // namespace XI2Handler

#endif
