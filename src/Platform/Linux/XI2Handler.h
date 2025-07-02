#pragma once

#ifdef __linux__

typedef struct _XDisplay Display;
typedef union _XEvent XEvent;

// xinput2 device management
namespace XI2Handler {

void handleGenericEvent(Display *display, XEvent &xev);
void updatePointerCache(Display *display);

extern int clientPointerDevID;
}  // namespace XI2Handler

#endif
