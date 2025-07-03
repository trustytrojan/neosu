#pragma once

#ifdef __linux__

#include "LinuxEnvironment.h"

class Engine;

class LinuxMain {
   public:
    LinuxMain(int argc, char *argv[], const std::vector<UString>& argCmdline, const std::unordered_map<UString, std::optional<UString>> &argMap);

    static int ret;

   private:
    bool bRunning{true};
    bool bUpdate{true};
    bool bDraw{true};
    bool bHasFocus{false};

    Display *dpy{nullptr};
    Window root{0};

    Colormap cmap{0};
    XSetWindowAttributes swa{};
    Window win{0};

    XEvent xev{};

    // input
    XIM im{0};
    XIC ic{0};
    int xi2opcode{0};

    void WndProc();
};

using Main = LinuxMain;
extern EnvironmentImpl *baseEnv;

#endif
