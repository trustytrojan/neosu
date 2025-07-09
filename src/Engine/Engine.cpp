#include "Engine.h"

#include <stdio.h>

#include <mutex>

#include "AnimationHandler.h"
#include "CBaseUIContainer.h"
#include "ConVar.h"
#include "Console.h"
#include "ConsoleBox.h"
#include "DiscordInterface.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "NetworkHandler.h"
#include "Osu.h"
#include "Profiler.h"
#include "ResourceManager.h"
#include "SoundEngine.h"
#include "Timing.h"
#include "VisualProfiler.h"

Environment *env = NULL;

std::unique_ptr<Mouse> Engine::s_mouseInstance = nullptr;
std::unique_ptr<Keyboard> Engine::s_keyboardInstance = nullptr;
std::unique_ptr<App> Engine::s_appInstance = nullptr;
std::unique_ptr<Graphics> Engine::s_graphicsInstance = nullptr;
std::unique_ptr<SoundEngine> Engine::s_soundEngineInstance = nullptr;
std::unique_ptr<ResourceManager> Engine::s_resourceManagerInstance = nullptr;
std::unique_ptr<NetworkHandler> Engine::s_networkHandlerInstance = nullptr;
std::unique_ptr<AnimationHandler> Engine::s_animationHandlerInstance = nullptr;

Mouse *mouse = nullptr;
Keyboard *keyboard = nullptr;
App *app = nullptr;
Graphics *g = nullptr;
SoundEngine *soundEngine = nullptr;
ResourceManager *resourceManager = nullptr;
NetworkHandler *networkHandler = nullptr;
AnimationHandler *animationHandler = nullptr;

Engine *engine = NULL;

Console *Engine::console = NULL;
ConsoleBox *Engine::consoleBox = NULL;

Engine::Engine(i32 argc, char **argv) {
    engine = this;

    this->iArgc = argc;
    this->sArgv = argv;

    this->guiContainer = NULL;
    this->visualProfiler = NULL;

    // disable output buffering (else we get multithreading issues due to blocking)
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    // print debug information
    debugLog("-= Engine Startup =-\n");

    // timing
    this->timer = new Timer();
    this->dTime = 0;
    this->dRunTime = 0;
    this->iFrameCount = 0;
    this->dFrameTime = 0.016f;

    // window
    this->bBlackout = false;
    this->bHasFocus = false;
    this->bIsMinimized = false;

    // screen
    this->bResolutionChange = false;
    this->vScreenSize = env->getWindowSize();
    this->vNewScreenSize = this->vScreenSize;

    debugLog("Engine: OsuScreenSize = (%ix%i)\n", (int)this->vScreenSize.x, (int)this->vScreenSize.y);

    // custom
    this->bDrawing = false;

    // initialize all engine subsystems (the order does matter!)
    debugLog("\nEngine: Initializing subsystems ...\n");
    {
        // input devices
        s_mouseInstance = std::make_unique<Mouse>();
        mouse = s_mouseInstance.get();
        runtime_assert(mouse, "Mouse failed to initialize!");
        this->inputDevices.push_back(mouse);
        this->mice.push_back(mouse);

        s_keyboardInstance = std::make_unique<Keyboard>();
        keyboard = s_keyboardInstance.get();
        runtime_assert(keyboard, "Keyboard failed to initialize!");
        this->inputDevices.push_back(keyboard);
        this->keyboards.push_back(keyboard);

        // create graphics through environment
        s_graphicsInstance.reset(env->createRenderer());
        g = s_graphicsInstance.get();
        runtime_assert(g, "Graphics failed to initialize!");
        g->init();  // needs init() separation due to potential graphics access

        // make unique_ptrs for the rest
        s_resourceManagerInstance = std::make_unique<ResourceManager>();
        resourceManager = s_resourceManagerInstance.get();
        runtime_assert(resourceManager, "Resource manager menu failed to initialize!");

        s_soundEngineInstance = std::make_unique<SoundEngine>();
        soundEngine = s_soundEngineInstance.get();
        runtime_assert(soundEngine, "Sound engine failed to initialize!");

        s_animationHandlerInstance = std::make_unique<AnimationHandler>();
        animationHandler = s_animationHandlerInstance.get();
        runtime_assert(animationHandler, "Animation handler failed to initialize!");

        s_networkHandlerInstance = std::make_unique<NetworkHandler>();
        networkHandler = s_networkHandlerInstance.get();
        runtime_assert(networkHandler, "Network handler failed to initialize!");

        init_discord_sdk();

        // default launch overrides
        g->setVSync(false);

        // engine time starts now
        this->timer->start();
    }
    debugLog("Engine: Initializing subsystems done.\n\n");
}

Engine::~Engine() {
    debugLog("\n-= Engine Shutdown =-\n");

    // reset() all static unique_ptrs
    debugLog("Engine: Freeing app...\n");
    s_appInstance.reset();
    app = NULL;

    if(this->console != NULL) showMessageErrorFatal("Engine Error", "this->console not set to NULL before shutdown!");

    debugLog("Engine: Freeing engine GUI...\n");
    {
        this->console = NULL;
        this->consoleBox = NULL;
    }
    SAFE_DELETE(this->guiContainer);

    destroy_discord_sdk();

    debugLog("Engine: Freeing network handler...\n");
    s_networkHandlerInstance.reset();
    networkHandler = NULL;

    debugLog("Engine: Freeing animation handler...\n");
    s_animationHandlerInstance.reset();
    animationHandler = NULL;

    debugLog("Engine: Freeing resource manager...\n");
    s_resourceManagerInstance.reset();
    resourceManager = NULL;

    debugLog("Engine: Freeing Sound...\n");
    s_soundEngineInstance.reset();
    soundEngine = NULL;

    debugLog("Engine: Freeing graphics...\n");
    s_graphicsInstance.reset();
    g = NULL;

    debugLog("Engine: Freeing input devices...\n");
    // first remove the mouse and keyboard from the input devices
    std::erase_if(this->inputDevices,
                  [](InputDevice *device) { return device == mouse || device == keyboard; });

    // delete remaining input devices (if any)
    for(auto *device : this->inputDevices) {
        delete device;
    }
    this->inputDevices.clear();
    this->mice.clear();
    this->keyboards.clear();

    // reset the static unique_ptrs
    s_mouseInstance.reset();
    s_keyboardInstance.reset();
    mouse = NULL;
    keyboard = NULL;

    debugLog("Engine: Freeing timer...\n");
    SAFE_DELETE(this->timer);

    debugLog("Engine: Freeing fonts...\n");
    McFont::cleanupSharedResources();

    debugLog("Engine: Goodbye.\n");

    engine = NULL;
}

void Engine::loadApp() {
    // create directories we will assume already exist later on
    if(!env->directoryExists(MCENGINE_DATA_DIR "avatars")) {
        env->createDirectory(MCENGINE_DATA_DIR "avatars");
    }
    if(!env->directoryExists(MCENGINE_DATA_DIR "cfg")) {
        env->createDirectory(MCENGINE_DATA_DIR "cfg");
    }
    if(!env->directoryExists(MCENGINE_DATA_DIR "maps")) {
        env->createDirectory(MCENGINE_DATA_DIR "maps");
    }
    if(!env->directoryExists(MCENGINE_DATA_DIR "replays")) {
        env->createDirectory(MCENGINE_DATA_DIR "replays");
    }
    if(!env->directoryExists(MCENGINE_DATA_DIR "screenshots")) {
        env->createDirectory(MCENGINE_DATA_DIR "screenshots");
    }
    if(!env->directoryExists(MCENGINE_DATA_DIR "skins")) {
        env->createDirectory(MCENGINE_DATA_DIR "skins");
    }

    // load core default resources
    debugLog("Engine: Loading default resources ...\n");
    resourceManager->loadFont("weblysleekuisb.ttf", "FONT_DEFAULT", 15, true, env->getDPI());
    resourceManager->loadFont("tahoma.ttf", "FONT_CONSOLE", 8, false, 96);
    debugLog("Engine: Loading default resources done.\n");

    // load other default resources and things which are not strictly necessary
    {
        Image *missingTexture = resourceManager->createImage(512, 512);
        resourceManager->setResourceName(missingTexture, "MISSING_TEXTURE");
        for(int x = 0; x < 512; x++) {
            for(int y = 0; y < 512; y++) {
                int rowCounter = (x / 64);
                int columnCounter = (y / 64);
                Color color = (((rowCounter + columnCounter) % 2 == 0) ? rgb(255, 0, 221) : rgb(0, 0, 0));
                missingTexture->setPixel(x, y, color);
            }
        }
        missingTexture->load();

        // create engine gui
        this->guiContainer = new CBaseUIContainer(0, 0, engine->getScreenWidth(), engine->getScreenHeight(), "");
        Engine::consoleBox = new ConsoleBox();
        this->guiContainer->addBaseUIElement(this->consoleBox);
        this->visualProfiler = new VisualProfiler();
        this->guiContainer->addBaseUIElement(this->visualProfiler);

        // (engine gui comes first)
        keyboard->addListener(this->guiContainer, true);
    }

    debugLog("\nEngine: Loading app ...\n");
    {
        //*****************//
        //	Load App here  //
        //*****************//

        s_appInstance = std::make_unique<Osu>();
        app = s_appInstance.get();
        runtime_assert(app, "App failed to initialize!");
        // start listening to the default keyboard input
        keyboard->addListener(app);
    }
    debugLog("Engine: Loading app done.\n\n");
}

void Engine::onPaint() {
    VPROF_BUDGET("Engine::onPaint", VPROF_BUDGETGROUP_DRAW);
    if(this->bBlackout || this->bIsMinimized) return;

    this->bDrawing = true;
    {
        // begin
        {
            VPROF_BUDGET("Graphics::beginScene", VPROF_BUDGETGROUP_DRAW);
            g->beginScene();
        }

        // middle
        {
            if(app != NULL) {
                VPROF_BUDGET("App::draw", VPROF_BUDGETGROUP_DRAW);
                app->draw();
            }

            if(this->guiContainer != NULL) this->guiContainer->draw();

            // debug input devices
            for(size_t i = 0; i < this->inputDevices.size(); i++) {
                this->inputDevices[i]->draw();
            }
        }

        // end
        {
            VPROF_BUDGET("Graphics::endScene", VPROF_BUDGETGROUP_DRAW_SWAPBUFFERS);
            g->endScene();
        }
    }
    this->bDrawing = false;

    this->iFrameCount++;
}

void Engine::onUpdate() {
    VPROF_BUDGET("Engine::onUpdate", VPROF_BUDGETGROUP_UPDATE);

    if(this->bBlackout) return;

    // update time
    {
        this->timer->update();
        this->dRunTime = this->timer->getElapsedTime();
        this->dTime += this->dFrameTime;
    }

    // handle pending queued resolution changes
    if(this->bResolutionChange) {
        this->bResolutionChange = false;

        if(cv_debug_engine.getBool())
            debugLog("Engine: executing pending queued resolution change to (%i, %i)\n", (int)this->vNewScreenSize.x,
                     (int)this->vNewScreenSize.y);

        this->onResolutionChange(this->vNewScreenSize);
    }

    // update miscellaneous engine subsystems
    {
        for(size_t i = 0; i < this->inputDevices.size(); i++) {
            this->inputDevices[i]->update();
        }

        {
            VPROF_BUDGET("AnimationHandler::update", VPROF_BUDGETGROUP_UPDATE);
            animationHandler->update();
        }

        {
            VPROF_BUDGET("SoundEngine::update", VPROF_BUDGETGROUP_UPDATE);
            soundEngine->update();
        }

        {
            VPROF_BUDGET("ResourceManager::update", VPROF_BUDGETGROUP_UPDATE);
            resourceManager->update();
        }

        // update gui
        bool propagate_clicks = true;
        if(this->guiContainer != NULL) this->guiContainer->mouse_update(&propagate_clicks);

        // execute queued commands
        // TODO: this is shit
        if(Console::g_commandQueue.size() > 0) {
            for(size_t i = 0; i < Console::g_commandQueue.size(); i++) {
                Console::processCommand(Console::g_commandQueue[i]);
            }
            Console::g_commandQueue = std::vector<UString>();  // reset
        }
    }

    // update app
    if(app != NULL) {
        VPROF_BUDGET("App::update", VPROF_BUDGETGROUP_UPDATE);
        app->update();
    }

    // update discord presence
    tick_discord_sdk();

    // update environment (after app, at the end here)
    {
        VPROF_BUDGET("Environment::update", VPROF_BUDGETGROUP_UPDATE);
        env->update();
    }
}

void Engine::onFocusGained() {
    this->bHasFocus = true;

    if(cv_debug_engine.getBool()) debugLog("Engine: got focus\n");

    if(app != NULL) app->onFocusGained();
}

void Engine::onFocusLost() {
    this->bHasFocus = false;

    if(cv_debug_engine.getBool()) debugLog("Engine: lost focus\n");

    for(size_t i = 0; i < this->keyboards.size(); i++) {
        this->keyboards[i]->reset();
    }

    if(app != NULL) app->onFocusLost();

    // auto minimize on certain conditions
    if(env->isFullscreen() || env->isFullscreenWindowedBorderless()) {
        if((!env->isFullscreenWindowedBorderless() && cv_minimize_on_focus_lost_if_fullscreen.getBool()) ||
           (env->isFullscreenWindowedBorderless() &&
            cv_minimize_on_focus_lost_if_borderless_windowed_fullscreen.getBool())) {
            env->minimize();
        }
    }
}

void Engine::onMinimized() {
    this->bIsMinimized = true;
    this->bHasFocus = false;

    if(cv_debug_engine.getBool()) debugLog("Engine: window minimized\n");

    if(app != NULL) app->onMinimized();
}

void Engine::onMaximized() {
    this->bIsMinimized = false;

    if(cv_debug_engine.getBool()) debugLog("Engine: window maximized\n");
}

void Engine::onRestored() {
    this->bIsMinimized = false;

    if(cv_debug_engine.getBool()) debugLog("Engine: window restored\n");

    if(app != NULL) app->onRestored();
}

void Engine::onResolutionChange(Vector2 newResolution) {
    debugLog(0xff00ff00, "Engine: onResolutionChange() (%i, %i) -> (%i, %i)\n", (int)this->vScreenSize.x,
             (int)this->vScreenSize.y, (int)newResolution.x, (int)newResolution.y);

    // NOTE: Windows [Show Desktop] button in the superbar causes (0,0)
    if(newResolution.x < 2 || newResolution.y < 2) {
        this->bIsMinimized = true;
        newResolution = Vector2(2, 2);
    }

    // to avoid double resolutionChange
    this->bResolutionChange = false;
    this->vNewScreenSize = newResolution;

    if(this->guiContainer != NULL) this->guiContainer->setSize(newResolution.x, newResolution.y);
    if(this->consoleBox != NULL) Engine::consoleBox->onResolutionChange(newResolution);

    // update everything
    this->vScreenSize = newResolution;
    if(g != NULL) g->onResolutionChange(newResolution);
    if(app != NULL) app->onResolutionChanged(newResolution);
}

void Engine::onDPIChange() {
    debugLog(0xff00ff00, "Engine: DPI changed to %i\n", env->getDPI());

    if(app != NULL) app->onDPIChanged();
}

void Engine::onShutdown() {
    if(this->bBlackout || (app != NULL && !app->onShutdown())) return;

    this->bBlackout = true;
    soundEngine->shutdown();
    env->shutdown();
}

void Engine::onKeyboardKeyDown(KEYCODE keyCode) {
    // hardcoded engine hotkeys
    {
        // handle ALT+F4 quit
        if(keyboard->isAltDown() && keyCode == KEY_F4) {
            this->shutdown();
            return;
        }

        // handle ALT+ENTER fullscreen toggle
        if(keyboard->isAltDown() && (keyCode == KEY_ENTER || keyCode == KEY_NUMPAD_ENTER)) {
            this->toggleFullscreen();
            return;
        }

        // handle CTRL+F11 profiler toggle
        if(keyboard->isControlDown() && keyCode == KEY_F11) {
            cv_vprof.setValue(cv_vprof.getBool() ? 0.0f : 1.0f);
            return;
        }

        // handle profiler display mode change
        if(keyboard->isControlDown() && keyCode == KEY_TAB) {
            if(cv_vprof.getBool()) {
                if(keyboard->isShiftDown())
                    this->visualProfiler->decrementInfoBladeDisplayMode();
                else
                    this->visualProfiler->incrementInfoBladeDisplayMode();
                return;
            }
        }
    }

    keyboard->onKeyDown(keyCode);
}

void Engine::onKeyboardKeyUp(KEYCODE keyCode) { keyboard->onKeyUp(keyCode); }

void Engine::onKeyboardChar(KEYCODE charCode) { keyboard->onChar(charCode); }

void Engine::shutdown() { this->onShutdown(); }

void Engine::restart() {
    this->onShutdown();
    env->restart();
}

void Engine::focus() { env->focus(); }

void Engine::center() { env->center(); }

void Engine::toggleFullscreen() {
    if(env->isFullscreen())
        env->disableFullscreen();
    else
        env->enableFullscreen();
}

void Engine::disableFullscreen() { env->disableFullscreen(); }

void Engine::showMessageInfo(const UString& title, const UString& message) {
    debugLog("INFO: [%s] | %s\n", title.toUtf8(), message.toUtf8());
    env->showMessageInfo(title, message);
}

void Engine::showMessageWarning(const UString& title, const UString& message) {
    debugLog("WARNING: [%s] | %s\n", title.toUtf8(), message.toUtf8());
    env->showMessageWarning(title, message);
}

void Engine::showMessageError(const UString& title, const UString& message) {
    debugLog("ERROR: [%s] | %s\n", title.toUtf8(), message.toUtf8());
    env->showMessageError(title, message);
}

void Engine::showMessageErrorFatal(const UString& title, const UString& message) {
    debugLog("FATAL ERROR: [%s] | %s\n", title.toUtf8(), message.toUtf8());
    env->showMessageErrorFatal(title, message);
}

void Engine::requestResolutionChange(Vector2 newResolution) {
    if(newResolution == this->vNewScreenSize) return;

    this->vNewScreenSize = newResolution;
    this->bResolutionChange = true;
}

void Engine::setFrameTime(double delta) {
    // NOTE: clamp to between 10000 fps and 1 fps, very small/big timesteps could cause problems
    // NOTE: special case for loading screen and first app frame
    if(this->iFrameCount < 3)
        this->dFrameTime = delta;
    else
        this->dFrameTime = std::clamp<double>(delta, 0.0001, 1.0);
}

double Engine::getTimeReal() {
    this->timer->update();
    return this->timer->getElapsedTime();
}

void Engine::logToConsole(std::optional<Color> color, const UString &msg) {
    if(Engine::consoleBox != nullptr) {
        if(color.has_value())
            Engine::consoleBox->log(msg, color.value());
        else
            Engine::consoleBox->log(msg);
    }

    if(Engine::console != nullptr) {
        if(color.has_value())
            Engine::console->log(msg, color.value());
        else
            Engine::console->log(msg);
    }
}

//**********************//
//	Engine ConCommands	//
//**********************//

void _exit(void) { engine->shutdown(); }

void _restart(void) { engine->restart(); }

void _printsize(void) {
    Vector2 s = engine->getScreenSize();
    debugLog("Engine: screenSize = (%f, %f)\n", s.x, s.y);
}

void _fullscreen(void) { engine->toggleFullscreen(); }

void _borderless(void) {
    if(cv_fullscreen_windowed_borderless.getBool()) {
        cv_fullscreen_windowed_borderless.setValue(0.0f);
        if(env->isFullscreen()) env->disableFullscreen();
    } else {
        cv_fullscreen_windowed_borderless.setValue(1.0f);
        if(!env->isFullscreen()) env->enableFullscreen();
    }
}

void _minimize(void) { env->minimize(); }

void _maximize(void) { env->maximize(); }

void _toggleresizable(void) { env->setWindowResizable(!env->isWindowResizable()); }

void _focus(void) { engine->focus(); }

void _center(void) { engine->center(); }

void _errortest(void) {
    engine->showMessageError(
        "Error Test",
        "This is an error message, fullscreen mode should be disabled and you should be able to read this");
}

void _dpiinfo(void) { debugLog("env->getDPI() = %i, env->getDPIScale() = %f\n", env->getDPI(), env->getDPIScale()); }
