#pragma once
#include <source_location>

#include "cbase.h"
#include "Timing.h"
#include "fmt/color.h"
#include "fmt/printf.h"

class App;
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

#ifdef _DEBUG
#define debugLogF(...) Engine::ContextLogger::log(std::source_location::current(), __FUNCTION__, __VA_ARGS__)
#define debugLog(...) Engine::ContextLogger::logPrintf(__FUNCTION__, __VA_ARGS__)  // deprecated
#else
#define debugLogF(...) Engine::ContextLogger::log(__FUNCTION__, __VA_ARGS__)
#define debugLog(...) Engine::ContextLogger::logPrintf(__FUNCTION__, __VA_ARGS__)  // deprecated
#endif

class Engine {
   public:
    Engine(Environment *environment, i32 argc, char **argv);
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
    [[nodiscard]] inline App *getApp() const { return this->app; }
    [[nodiscard]] inline Graphics *getGraphics() const { return this->graphics; }
    [[nodiscard]] inline SoundEngine *getSound() const { return this->sound; }
    [[nodiscard]] inline ResourceManager *getResourceManager() const { return this->resourceManager; }
    [[nodiscard]] inline Environment *getEnvironment() const { return this->environment; }
    [[nodiscard]] inline NetworkHandler *getNetworkHandler() const { return this->networkHandler; }

    // input devices
    [[nodiscard]] inline Mouse *getMouse() const { return this->mouse; }
    [[nodiscard]] inline Keyboard *getKeyboard() const { return this->keyboard; }
    [[nodiscard]] inline Gamepad *getGamepad() const { return this->gamepad; }
    [[nodiscard]] inline const std::vector<Mouse *> &getMice() const { return this->mice; }
    [[nodiscard]] inline const std::vector<Keyboard *> &getKeyboards() const { return this->keyboards; }
    [[nodiscard]] inline const std::vector<Gamepad *> &getGamepads() const { return this->gamepads; }

    // screen
    void requestResolutionChange(Vector2 newResolution);
    [[nodiscard]] inline Vector2 getScreenSize() const { return this->vScreenSize; }
    [[nodiscard]] inline int getScreenWidth() const { return (int)this->vScreenSize.x; }
    [[nodiscard]] inline int getScreenHeight() const { return (int)this->vScreenSize.y; }

    // vars
    void setFrameTime(double delta);
    [[nodiscard]] inline double getTime() const { return this->dTime; }
    double getTimeReal();
    [[nodiscard]] inline double getTimeRunning() const { return this->dRunTime; }
    [[nodiscard]] inline double getFrameTime() const { return this->dFrameTime; }
    [[nodiscard]] inline unsigned long getFrameCount() const { return this->iFrameCount; }

    [[nodiscard]] inline bool hasFocus() const { return this->bHasFocus; }
    [[nodiscard]] inline bool isDrawing() const { return this->bDrawing; }
    [[nodiscard]] inline bool isMinimized() const { return this->bIsMinimized; }

    // debugging/console
    void setConsole(Console *console) { this->console = console; }
    [[nodiscard]] inline ConsoleBox *getConsoleBox() const { return this->consoleBox; }
    [[nodiscard]] inline Console *getConsole() const { return this->console; }
    [[nodiscard]] inline CBaseUIContainer *getGUI() const { return this->guiContainer; }

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
    i32 iArgc;
    char **sArgv;
    bool bBlackout;
    bool bDrawing;

   public:
    class ContextLogger {
       public:
        // debug build shows full source location
        template <typename... Args>
        static void log(const std::source_location &loc, const char *func, fmt::format_string<Args...> fmt,
                        Args &&...args) {
            // auto contextPrefix =
            //     fmt::format("[{}:{}:{}] [{}]: ", Environment::getFileNameFromFilePath(loc.file_name()), loc.line(),
            //     loc.column(), func);

            auto contextPrefix = fmt::format("[{}:{}:{}] [{}]: ", loc.file_name(), loc.line(), loc.column(), func);

            auto message = fmt::format(fmt, std::forward<Args>(args)...);
            Engine::logImpl(contextPrefix + message);
        }

        template <typename... Args>
        static void log(const std::source_location &loc, const char *func, Color color, fmt::format_string<Args...> fmt,
                        Args &&...args) {
            // auto contextPrefix =
            //     fmt::format("[{}:{}:{}] [{}]: ", Environment::getFileNameFromFilePath(loc.file_name()), loc.line(),
            //     loc.column(), func);

            auto contextPrefix = fmt::format("[{}:{}:{}] [{}]: ", loc.file_name(), loc.line(), loc.column(), func);

            auto message = fmt::format(fmt, std::forward<Args>(args)...);
            Engine::logImpl(contextPrefix + message, color);
        }

        // release build only shows function name
        template <typename... Args>
        static void log(const char *func, fmt::format_string<Args...> fmt, Args &&...args) {
            auto contextPrefix = fmt::format("[{}] ", func);
            auto message = fmt::format(fmt, std::forward<Args>(args)...);
            Engine::logImpl(contextPrefix + message);
        }

        template <typename... Args>
        static void log(const char *func, Color color, fmt::format_string<Args...> fmt, Args &&...args) {
            auto contextPrefix = fmt::format("[{}] ", func);
            auto message = fmt::format(fmt, std::forward<Args>(args)...);
            Engine::logImpl(contextPrefix + message, color);
        }

        // debug build shows full source location
        template <typename... Args>
        [[deprecated("use compile-time format string checked debugLogF instead")]]
        static void logPrintf(const std::source_location &loc, const char *func, const std::string_view &fmt, Args &&...args) {
            // auto contextPrefix =
            //     fmt::format("[{}:{}:{}] [{}]: ", Environment::getFileNameFromFilePath(loc.file_name()), loc.line(),
            //     loc.column(), func);

            auto contextPrefix = fmt::format("[{}:{}:{}] [{}]: ", loc.file_name(), loc.line(), loc.column(), func);

            auto message = fmt::format(fmt, std::forward<Args>(args)...);
            Engine::logImpl(contextPrefix + message);
        }

        template <typename... Args>
        [[deprecated("use compile-time format string checked debugLogF instead")]]
        static void logPrintf(const std::source_location &loc, const char *func, Color color, const std::string_view &fmt,
                              Args &&...args) {
            // auto contextPrefix =
            //     fmt::format("[{}:{}:{}] [{}]: ", Environment::getFileNameFromFilePath(loc.file_name()), loc.line(),
            //     loc.column(), func);

            auto contextPrefix = fmt::format("[{}:{}:{}] [{}]: ", loc.file_name(), loc.line(), loc.column(), func);

            std::string message = fmt::sprintf(fmt, std::forward<Args>(args)...);
            Engine::logImpl(contextPrefix + message, color);
        }

        // release build only shows function name
        template <typename... Args>
        [[deprecated("use compile-time format string checked debugLogF instead")]]
        static void logPrintf(const char *func, const std::string_view &fmt, Args &&...args) {
            auto contextPrefix = fmt::format("[{}] ", func);
            std::string message = fmt::sprintf(fmt, std::forward<Args>(args)...);
            Engine::logImpl(contextPrefix + message);
        }

        template <typename... Args>
        [[deprecated("use compile-time format string checked debugLogF instead")]]
        static void logPrintf(const char *func, Color color, const std::string_view &fmt, Args &&...args) {
            auto contextPrefix = fmt::format("[{}] ", func);
            std::string message = fmt::sprintf(fmt, std::forward<Args>(args)...);
            Engine::logImpl(contextPrefix + message, color);
        }
    };
    template <typename... Args>
    static void logRaw(fmt::format_string<Args...> fmt, Args &&...args) {
        auto message = fmt::format(fmt, std::forward<Args>(args)...);
        Engine::logImpl(message);
    }

   private:
    // logging stuff (implementation)
    static void logToConsole(std::optional<Color> color, const UString &message);

    static void logImpl(const std::string &message, Color color = rgb(255, 255, 255)) {
        if constexpr(Env::cfg(OS::WINDOWS))  // hmm... odd bug with fmt::print (or mingw?), when the stdout isn't
                                             // redirected to a file
        {
            if(color == rgb(255, 255, 255) || !Environment::isaTTY())
                printf("%s", fmt::format("{}", message).c_str());
            else
                printf("%s", fmt::format(fmt::fg(fmt::rgb(color.R(), color.G(), color.B())), "{}", message).c_str());
        } else {
            if(color == rgb(255, 255, 255) || !Environment::isaTTY())
                fmt::print("{}", message);
            else
                fmt::print(fmt::fg(fmt::rgb(color.R(), color.G(), color.B())), "{}", message);
        }
        logToConsole(color, UString(message));
    }
};

extern Engine *engine;

void _exit(void);
void _restart(void);
void _printsize(void);
void _fullscreen(void);
void _borderless(void);
void _minimize(void);
void _maximize(void);
void _toggleresizable(void);
void _focus(void);
void _center(void);
void _errortest(void);
void _dpiinfo(void);
