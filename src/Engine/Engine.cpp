#include "Engine.h"

#include <stdio.h>

#include <mutex>

#include "AnimationHandler.h"
#include "CBaseUIContainer.h"
#include "ConVar.h"
#include "Console.h"
#include "ConsoleBox.h"
#include "ContextMenu.h"
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
#include "XInputGamepad.h"

using namespace std;

Engine *engine = NULL;
Environment *env = NULL;

Console *Engine::console = NULL;
ConsoleBox *Engine::consoleBox = NULL;

Engine::Engine(Environment *environment, i32 argc, char **argv) {
    // XXX: run curl_global_cleanup() after waiting for network threads to terminate
    curl_global_init(CURL_GLOBAL_DEFAULT);

    engine = this;
    this->environment = environment;
    env = environment;
    this->iArgc = argc;
    this->sArgv = argv;

    this->graphics = NULL;
    this->guiContainer = NULL;
    this->visualProfiler = NULL;
    this->app = NULL;

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
    this->vScreenSize = this->environment->getWindowSize();
    this->vNewScreenSize = this->vScreenSize;

    debugLog("Engine: OsuScreenSize = (%ix%i)\n", (int)this->vScreenSize.x, (int)this->vScreenSize.y);

    // custom
    this->bDrawing = false;

    // initialize all engine subsystems (the order does matter!)
    debugLog("\nEngine: Initializing subsystems ...\n");
    {
        // input devices
        this->mouse = new Mouse();
        this->inputDevices.push_back(this->mouse);
        this->mice.push_back(this->mouse);

        this->keyboard = new Keyboard();
        this->inputDevices.push_back(this->keyboard);
        this->keyboards.push_back(this->keyboard);

        this->gamepad = new XInputGamepad();
        this->inputDevices.push_back(this->gamepad);
        this->gamepads.push_back(this->gamepad);

        // init platform specific interfaces
        this->graphics = this->environment->createRenderer();
        {
            this->graphics->init();  // needs init() separation due to potential engine->getGraphics() access
        }
        this->contextMenu = this->environment->createContextMenu();

        // and the rest
        this->resourceManager = new ResourceManager();
        this->sound = new SoundEngine();
        this->animationHandler = new AnimationHandler();
        this->networkHandler = new NetworkHandler();
        init_discord_sdk();

        // default launch overrides
        this->graphics->setVSync(false);

        // engine time starts now
        this->timer->start();
    }
    debugLog("Engine: Initializing subsystems done.\n\n");
}

Engine::~Engine() {
    debugLog("\n-= Engine Shutdown =-\n");

    debugLog("Engine: Freeing app...\n");
    SAFE_DELETE(this->app);

    if(this->console != NULL) this->showMessageErrorFatal("Engine Error", "console not set to NULL before shutdown!");

    debugLog("Engine: Freeing engine GUI...\n");
    Engine::console = NULL;
    Engine::consoleBox = NULL;
    SAFE_DELETE(this->guiContainer);

    debugLog("Engine: Freeing resource manager...\n");
    SAFE_DELETE(this->resourceManager);

    debugLog("Engine: Freeing Sound...\n");
    SAFE_DELETE(this->sound);

    debugLog("Engine: Freeing context menu...\n");
    SAFE_DELETE(this->contextMenu);

    debugLog("Engine: Freeing animation handler...\n");
    SAFE_DELETE(this->animationHandler);

    debugLog("Engine: Freeing network handler...\n");
    SAFE_DELETE(this->networkHandler);

    debugLog("Engine: Freeing input devices...\n");
    for(size_t i = 0; i < this->inputDevices.size(); i++) {
        delete this->inputDevices[i];
    }
    this->inputDevices.clear();

    debugLog("Engine: Freeing timer...\n");
    SAFE_DELETE(this->timer);

    debugLog("Engine: Freeing graphics...\n");
    SAFE_DELETE(this->graphics);

	debugLog("Engine: Freeing fonts...\n");
	McFont::cleanupSharedResources();

    debugLog("Engine: Freeing environment...\n");
    SAFE_DELETE(this->environment);
    destroy_discord_sdk();

    debugLog("Engine: Goodbye.");

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
    engine->getResourceManager()->loadFont("weblysleekuisb.ttf", "FONT_DEFAULT", 15, true, this->environment->getDPI());
    engine->getResourceManager()->loadFont("tahoma.ttf", "FONT_CONSOLE", 8, false, 96);
    debugLog("Engine: Loading default resources done.\n");

    // load other default resources and things which are not strictly necessary
    {
        Image *missingTexture = engine->getResourceManager()->createImage(512, 512);
        missingTexture->setName("MISSING_TEXTURE");
        for(int x = 0; x < 512; x++) {
            for(int y = 0; y < 512; y++) {
                int rowCounter = (x / 64);
                int columnCounter = (y / 64);
                Color color = (((rowCounter + columnCounter) % 2 == 0) ? COLOR(255, 255, 0, 221) : COLOR(255, 0, 0, 0));
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
        this->keyboard->addListener(this->guiContainer, true);
    }

    debugLog("\nEngine: Loading app ...\n");
    {
        //*****************//
        //	Load App here  //
        //*****************//
        osu = new Osu();
        this->app = osu;

        // start listening to the default keyboard input
        if(this->app != NULL) this->keyboard->addListener(this->app);
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
            this->graphics->beginScene();
        }

        // middle
        {
            if(this->app != NULL) {
                VPROF_BUDGET("App::draw", VPROF_BUDGETGROUP_DRAW);
                this->app->draw(this->graphics);
            }

            if(this->guiContainer != NULL) this->guiContainer->draw(this->graphics);

            // debug input devices
            for(size_t i = 0; i < this->inputDevices.size(); i++) {
                this->inputDevices[i]->draw(this->graphics);
            }
        }

        // end
        {
            VPROF_BUDGET("Graphics::endScene", VPROF_BUDGETGROUP_DRAW_SWAPBUFFERS);
            this->graphics->endScene();
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
            this->animationHandler->update();
        }

        {
            VPROF_BUDGET("SoundEngine::update", VPROF_BUDGETGROUP_UPDATE);
            this->sound->update();
        }

        {
            VPROF_BUDGET("ResourceManager::update", VPROF_BUDGETGROUP_UPDATE);
            this->resourceManager->update();
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
    if(this->app != NULL) {
        VPROF_BUDGET("App::update", VPROF_BUDGETGROUP_UPDATE);
        this->app->update();
    }

    // update discord presence
    tick_discord_sdk();

    // update environment (after app, at the end here)
    {
        VPROF_BUDGET("Environment::update", VPROF_BUDGETGROUP_UPDATE);
        this->environment->update();
    }
}

void Engine::onFocusGained() {
    this->bHasFocus = true;

    if(cv_debug_engine.getBool()) debugLog("Engine: got focus\n");

    if(this->app != NULL) this->app->onFocusGained();
}

void Engine::onFocusLost() {
    this->bHasFocus = false;

    if(cv_debug_engine.getBool()) debugLog("Engine: lost focus\n");

    for(size_t i = 0; i < this->keyboards.size(); i++) {
        this->keyboards[i]->reset();
    }

    if(this->app != NULL) this->app->onFocusLost();

    // auto minimize on certain conditions
    if(this->environment->isFullscreen() || this->environment->isFullscreenWindowedBorderless()) {
        if((!this->environment->isFullscreenWindowedBorderless() &&
            cv_minimize_on_focus_lost_if_fullscreen.getBool()) ||
           (this->environment->isFullscreenWindowedBorderless() &&
            cv_minimize_on_focus_lost_if_borderless_windowed_fullscreen.getBool())) {
            this->environment->minimize();
        }
    }
}

void Engine::onMinimized() {
    this->bIsMinimized = true;
    this->bHasFocus = false;

    if(cv_debug_engine.getBool()) debugLog("Engine: window minimized\n");

    if(this->app != NULL) this->app->onMinimized();
}

void Engine::onMaximized() {
    this->bIsMinimized = false;

    if(cv_debug_engine.getBool()) debugLog("Engine: window maximized\n");
}

void Engine::onRestored() {
    this->bIsMinimized = false;

    if(cv_debug_engine.getBool()) debugLog("Engine: window restored\n");

    if(this->app != NULL) this->app->onRestored();
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
    if(this->graphics != NULL) this->graphics->onResolutionChange(newResolution);
    if(this->app != NULL) this->app->onResolutionChanged(newResolution);
}

void Engine::onDPIChange() {
    debugLog(0xff00ff00, "Engine: DPI changed to %i\n", this->environment->getDPI());

    if(this->app != NULL) this->app->onDPIChanged();
}

void Engine::onShutdown() {
    if(this->bBlackout || (this->app != NULL && !this->app->onShutdown())) return;

    this->bBlackout = true;
    this->sound->shutdown();
    this->environment->shutdown();
}

void Engine::onMouseRawMove(int xDelta, int yDelta, bool absolute, bool virtualDesktop) {
    this->mouse->onRawMove(xDelta, yDelta, absolute, virtualDesktop);
}

void Engine::onMouseWheelVertical(int delta) { this->mouse->onWheelVertical(delta); }

void Engine::onMouseWheelHorizontal(int delta) { this->mouse->onWheelHorizontal(delta); }

std::mutex g_engineMouseLeftClickMutex;

void Engine::onMouseLeftChange(bool mouseLeftDown) {
    // async calls from WinRealTimeStylus must be protected
    std::lock_guard<std::mutex> lk(g_engineMouseLeftClickMutex);

    if(this->mouse->isLeftDown() !=
       mouseLeftDown)  // necessary due to WinRealTimeStylus and Touch, would cause double clicks otherwise
        this->mouse->onLeftChange(mouseLeftDown);
}

void Engine::onMouseMiddleChange(bool mouseMiddleDown) { this->mouse->onMiddleChange(mouseMiddleDown); }

void Engine::onMouseRightChange(bool mouseRightDown) {
    if(this->mouse->isRightDown() != mouseRightDown)  // necessary due to Touch, would cause double clicks otherwise
        this->mouse->onRightChange(mouseRightDown);
}

void Engine::onMouseButton4Change(bool mouse4down) { this->mouse->onButton4Change(mouse4down); }

void Engine::onMouseButton5Change(bool mouse5down) { this->mouse->onButton5Change(mouse5down); }

void Engine::onKeyboardKeyDown(KEYCODE keyCode) {
    // hardcoded engine hotkeys
    {
        // handle ALT+F4 quit
        if(this->keyboard->isAltDown() && keyCode == KEY_F4) {
            this->shutdown();
            return;
        }

        // handle ALT+ENTER fullscreen toggle
        if(this->keyboard->isAltDown() && keyCode == KEY_ENTER) {
            this->toggleFullscreen();
            return;
        }

        // handle CTRL+F11 profiler toggle
        if(this->keyboard->isControlDown() && keyCode == KEY_F11) {
            cv_vprof.setValue(cv_vprof.getBool() ? 0.0f : 1.0f);
            return;
        }

        // handle profiler display mode change
        if(this->keyboard->isControlDown() && keyCode == KEY_TAB) {
            if(cv_vprof.getBool()) {
                if(this->keyboard->isShiftDown())
                    this->visualProfiler->decrementInfoBladeDisplayMode();
                else
                    this->visualProfiler->incrementInfoBladeDisplayMode();
                return;
            }
        }
    }

    this->keyboard->onKeyDown(keyCode);
}

void Engine::onKeyboardKeyUp(KEYCODE keyCode) { this->keyboard->onKeyUp(keyCode); }

void Engine::onKeyboardChar(KEYCODE charCode) { this->keyboard->onChar(charCode); }

void Engine::shutdown() { this->onShutdown(); }

void Engine::restart() {
    this->onShutdown();
    this->environment->restart();
}

void Engine::sleep(unsigned int us) { this->environment->sleep(us); }

void Engine::focus() { this->environment->focus(); }

void Engine::center() { this->environment->center(); }

void Engine::toggleFullscreen() {
    if(this->environment->isFullscreen())
        this->environment->disableFullscreen();
    else
        this->environment->enableFullscreen();
}

void Engine::disableFullscreen() { this->environment->disableFullscreen(); }

void Engine::showMessageInfo(UString title, UString message) {
    debugLog("INFO: [%s] | %s\n", title.toUtf8(), message.toUtf8());
    this->environment->showMessageInfo(title, message);
}

void Engine::showMessageWarning(UString title, UString message) {
    debugLog("WARNING: [%s] | %s\n", title.toUtf8(), message.toUtf8());
    this->environment->showMessageWarning(title, message);
}

void Engine::showMessageError(UString title, UString message) {
    debugLog("ERROR: [%s] | %s\n", title.toUtf8(), message.toUtf8());
    this->environment->showMessageError(title, message);
}

void Engine::showMessageErrorFatal(UString title, UString message) {
    debugLog("FATAL ERROR: [%s] | %s\n", title.toUtf8(), message.toUtf8());
    this->environment->showMessageErrorFatal(title, message);
}

void Engine::addGamepad(Gamepad *gamepad) {
    if(gamepad == NULL) {
        this->showMessageError("Engine Error", "addGamepad(NULL)!");
        return;
    }

    this->gamepads.push_back(gamepad);
}

void Engine::removeGamepad(Gamepad *gamepad) {
    if(gamepad == NULL) {
        this->showMessageError("Engine Error", "removeGamepad(NULL)!");
        return;
    }

    for(size_t i = 0; i < this->gamepads.size(); i++) {
        if(this->gamepads[i] == gamepad) {
            this->gamepads.erase(this->gamepads.begin() + i);
            break;
        }
    }
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
        this->dFrameTime = clamp<double>(delta, 0.0001, 1.0);
}

double Engine::getTimeReal() {
    this->timer->update();
    return this->timer->getElapsedTime();
}

void Engine::logToConsole(std::optional<Color> color, const UString &msg)
{
	if (Engine::consoleBox != nullptr)
	{
		if (color.has_value())
			Engine::consoleBox->log(msg, color.value());
		else
			Engine::consoleBox->log(msg);
	}

	if (Engine::console != nullptr)
	{
		if (color.has_value())
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
