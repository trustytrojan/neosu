#pragma once

#ifdef __linux__

#include "LinuxEnvironment.h"

class Engine;

class LinuxMain final {
    friend LinuxEnvironment;

   public:
    LinuxMain(int argc, char *argv[], const std::vector<UString> &argCmdline,
              const std::unordered_map<UString, std::optional<UString>> &argMap);

    static int ret;

   private:
    bool bRunning{true};
    bool bUpdate{true};
    bool bDraw{true};
    bool bHasFocus{false};

    Display *dpy{nullptr};
    Window clientWindow{0};
    int screen_num{0};
    Screen *screen{nullptr};

    // input
    XIM im{0};
    XIC ic{0};
    int xi2opcode{0};

    bool WndProc(XEvent *xev);
    KEYCODE normalizeKeysym(KEYCODE keysym);

    Atom WM_PROTOCOLS_atom{};
    Atom WM_DELETE_WINDOW_atom{};

    static bool sdl_eventhook(void *thisptr, XEvent *xev) { return !static_cast<LinuxMain *>(thisptr)->WndProc(xev); }
};

using Main = LinuxMain;
extern EnvironmentImpl *baseEnv;

#endif
