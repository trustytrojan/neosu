#pragma once
#include "cbase.h"

class App;
class Timer;
class Mouse;
class ConVar;
class Gamepad;
class Keyboard;
class InputDevice;
class SoundEngine;
class ContextMenu;
class NetworkHandler;
class ResourceManager;
class AnimationHandler;
class DiscordInterface;

class CBaseUIContainer;
class VisualProfiler;
class ConsoleBox;
class Console;

class Engine {
   public:
    static void debugLog(const char *fmt, va_list args);
    static void debugLog(Color color, const char *fmt, va_list args);
    static void debugLog(const char *fmt, ...);
    static void debugLog(Color color, const char *fmt, ...);

   public:
    Engine(Environment *environment, const char *args = NULL);
    ~Engine();

    // app
    void loadApp();

    // render/update
    void onPaint();
    void onUpdate();

    // window messages
    void onFocusGained();
    void onFocusLost();
    void onMinimized();
    void onMaximized();
    void onRestored();
    void onResolutionChange(Vector2 newResolution);
    void onDPIChange();
    void onShutdown();

    // primary mouse messages
    void onMouseRawMove(int xDelta, int yDelta, bool absolute = false, bool virtualDesktop = false);
    void onMouseWheelVertical(int delta);
    void onMouseWheelHorizontal(int delta);
    void onMouseLeftChange(bool mouseLeftDown);
    void onMouseMiddleChange(bool mouseMiddleDown);
    void onMouseRightChange(bool mouseRightDown);
    void onMouseButton4Change(bool mouse4down);
    void onMouseButton5Change(bool mouse5down);

    // primary keyboard messages
    void onKeyboardKeyDown(KEYCODE keyCode);
    void onKeyboardKeyUp(KEYCODE keyCode);
    void onKeyboardChar(KEYCODE charCode);

    // convenience functions (passthroughs)
    void shutdown();
    void restart();
    void sleep(unsigned int us);
    void focus();
    void center();
    void toggleFullscreen();
    void disableFullscreen();

    void showMessageInfo(UString title, UString message);
    void showMessageWarning(UString title, UString message);
    void showMessageError(UString title, UString message);
    void showMessageErrorFatal(UString title, UString message);

    // engine specifics
    void blackout() { this->bBlackout = true; }
    void addGamepad(Gamepad *gamepad);
    void removeGamepad(Gamepad *gamepad);

    // interfaces
    inline App *getApp() const { return this->app; }
    inline Graphics *getGraphics() const { return this->graphics; }
    inline SoundEngine *getSound() const { return this->sound; }
    inline ResourceManager *getResourceManager() const { return this->resourceManager; }
    inline Environment *getEnvironment() const { return this->environment; }
    inline NetworkHandler *getNetworkHandler() const { return this->networkHandler; }

    // input devices
    inline Mouse *getMouse() const { return this->mouse; }
    inline Keyboard *getKeyboard() const { return this->keyboard; }
    inline Gamepad *getGamepad() const { return this->gamepad; }
    inline const std::vector<Mouse *> &getMice() const { return this->mice; }
    inline const std::vector<Keyboard *> &getKeyboards() const { return this->keyboards; }
    inline const std::vector<Gamepad *> &getGamepads() const { return this->gamepads; }

    // screen
    void requestResolutionChange(Vector2 newResolution);
    inline Vector2 getScreenSize() const { return this->vScreenSize; }
    inline int getScreenWidth() const { return (int)this->vScreenSize.x; }
    inline int getScreenHeight() const { return (int)this->vScreenSize.y; }

    // vars
    void setFrameTime(double delta);
    inline double getTime() const { return this->dTime; }
    double getTimeReal();
    inline double getTimeRunning() const { return this->dRunTime; }
    inline double getFrameTime() const { return this->dFrameTime; }
    inline unsigned long getFrameCount() const { return this->iFrameCount; }

    UString getArgs() const { return this->sArgs; }

    inline bool hasFocus() const { return this->bHasFocus; }
    inline bool isDrawing() const { return this->bDrawing; }
    inline bool isMinimized() const { return this->bIsMinimized; }

    // debugging/console
    void setConsole(Console *console) { this->console = console; }
    inline ConsoleBox *getConsoleBox() const { return this->consoleBox; }
    inline Console *getConsole() const { return this->console; }
    inline CBaseUIContainer *getGUI() const { return this->guiContainer; }

   private:
    // interfaces
    App *app;
    Graphics *graphics;
    SoundEngine *sound;
    ContextMenu *contextMenu;
    Environment *environment;
    NetworkHandler *networkHandler;
    ResourceManager *resourceManager;
    AnimationHandler *animationHandler;

    // input devices
    Mouse *mouse;
    Keyboard *keyboard;
    Gamepad *gamepad;
    std::vector<Mouse *> mice;
    std::vector<Keyboard *> keyboards;
    std::vector<Gamepad *> gamepads;
    std::vector<InputDevice *> inputDevices;

    // timing
    Timer *timer;
    double dTime;
    double dRunTime;
    unsigned long iFrameCount;
    double dFrameTime;

    // primary screen
    Vector2 vScreenSize;
    Vector2 vNewScreenSize;
    bool bResolutionChange;

    // window
    bool bHasFocus;
    bool bIsMinimized;

    // engine gui, mostly for debugging
    CBaseUIContainer *guiContainer;
    VisualProfiler *visualProfiler;
    static ConsoleBox *consoleBox;
    static Console *console;

    // custom
    UString sArgs;
    bool bBlackout;
    bool bDrawing;
};

extern Engine *engine;

#define debugLog(format, ...) Engine::debugLog(format, ##__VA_ARGS__)

void _exit(void);
void _restart(void);
void _printsize(void);
void _fullscreen(void);
void _borderless(void);
void _windowed(UString args);
void _minimize(void);
void _maximize(void);
void _toggleresizable(void);
void _focus(void);
void _center(void);
void _errortest(void);
void _dpiinfo(void);
