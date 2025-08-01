#ifdef _WIN32

#include "WindowsMain.h"

// clang-format off
// Include order matters
#include "WinDebloatDefs.h"
#include <dwmapi.h>
#include <shellapi.h>
#include <objbase.h>
#include <processthreadsapi.h>  // for IsWow64Process
// clang-format on

#include "Mouse.h"

// NEXTRAWINPUTBLOCK macro requires this
typedef uint64_t QWORD;

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include <utility>

#include "ConVar.h"
#include "Engine.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "Profiler.h"
#include "Timing.h"
#include "WinEnvironment.h"
#include "WinGLLegacyInterface.h"
#include "FPSLimiter.h"

#define WINDOW_TITLE L"neosu"
#define WM_NEOSU_PROTOCOL (WM_USER + 1)

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define WINDOW_WIDTH_MIN 100
#define WINDOW_HEIGHT_MIN 100

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

#define WM_NCUAHDRAWCAPTION 0x00AE
#define WM_NCUAHDRAWFRAME 0x00AF

// FIXME: clean these handle_* functions up to not be 100% windows only

extern bool g_bCursorVisible;  // set by WinEnvironment, for client area cursor invis

extern "C" {  // force switch to the high performance gpu in multi-gpu systems (mostly laptops)
__declspec(dllexport) DWORD NvOptimusEnablement =
    0x00000001;  // http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance =
    0x00000001;  // https://community.amd.com/thread/169965
}

// this is retarded
// https://stackoverflow.com/a/77281559
WPARAM WindowsMain::mapLeftRightKeys(WPARAM wParam, LPARAM lParam) {
    WORD vkCode = LOWORD(wParam);
    WORD keyFlags = HIWORD(lParam);

    WORD scanCode = LOBYTE(keyFlags);
    BOOL isExtendedKey = (keyFlags & KF_EXTENDED) == KF_EXTENDED;

    if(isExtendedKey) scanCode = MAKEWORD(scanCode, 0xE0);

    switch(vkCode) {
        case VK_SHIFT:
        case VK_CONTROL:
        case VK_MENU:
            vkCode = LOWORD(MapVirtualKey(scanCode, MAPVK_VSC_TO_VK_EX));
            break;
    }

    return vkCode;
}

// these were pulled straight out of SDL3 (zlib licensed)
HRESULT WindowsMain::doCoInitialize() {
    typedef HRESULT(WINAPI * CoInitializeEx_t)(LPVOID, COINIT initType);
    auto CoInitializeFunc = reinterpret_cast<CoInitializeEx_t>(doLoadComBaseFunction("CoInitializeEx"));

    HRESULT hr = CoInitializeFunc(NULL, COINIT_APARTMENTTHREADED);
    if(hr == RPC_E_CHANGED_MODE) {
        hr = CoInitializeFunc(NULL, COINIT_MULTITHREADED);
    }
    if(hr == S_FALSE) {
        return S_OK;
    }
    return hr;
}

void WindowsMain::doCoUninitialize(void) {
    typedef void(WINAPI * CoUnInitialize_t)();
    auto CoUninitializeFunc = reinterpret_cast<CoUnInitialize_t>(doLoadComBaseFunction("CoUninitialize"));
    return CoUninitializeFunc();
}

FARPROC WindowsMain::doLoadComBaseFunction(const char *name) {
    static bool s_bLoaded;
    static HMODULE s_hComBase;

    if(!s_bLoaded) {
        s_hComBase = LoadLibraryEx(TEXT("combase.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
        s_bLoaded = true;
    }
    if(s_hComBase) {
        return GetProcAddress(s_hComBase, name);
    } else {
        return NULL;
    }
}

//********************//
//	Main entry point  //
//********************//

Main *mainloopPtrHack = nullptr;  // FIXME: why is the handle_cmdline_args shit in the windows main file

bool WindowsMain::bSupportsPerMonitorDpiAwareness = false;

WindowsMain::WindowsMain(int argc, char *argv[], const std::vector<UString> &argCmdline,
                         const std::unordered_map<UString, std::optional<UString>> &argMap) {
    mainloopPtrHack = this;
    // @spec: TEST THIS?
    // see: Engine.h:250
    // #ifdef _MSC_VER
    //     // When building with MSVC, vprintf() is not returning the correct value unless we have a console allocated.
    //     FILE *dummy;
    // #ifdef _DEBUG
    //     AllocConsole();
    //     freopen_s(&dummy, "CONOUT$", "w", stdout);
    //     freopen_s(&dummy, "CONOUT$", "w", stderr);
    // #else
    //     freopen_s(&dummy, "NUL", "w", stdout);
    //     freopen_s(&dummy, "NUL", "w", stderr);
    // #endif
    // #endif

    // register .osk, .osr, .osz, and neosu:// protocol
    register_neosu_file_associations();

    // if a neosu instance is already running, send it a message then quit
    HWND existing_window = FindWindowW(L"neosu", NULL);
    if(existing_window) {
        for(i32 i = 0; i < argc; i++) {
            COPYDATASTRUCT cds;
            cds.dwData = WM_NEOSU_PROTOCOL;
            cds.cbData = strlen(argv[i]) + 1;
            cds.lpData = argv[i];
            SendMessage(existing_window, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
        }

        SetForegroundWindow(existing_window);
        ret = 0;
        return;
    }

    // disable IME text input if -noime (or if the feature won't be supported)
#ifdef MCENGINE_FEATURE_IMESUPPORT
    if(argMap.contains("-noime"))
#endif
    {
        typedef BOOL(WINAPI * pfnImmDisableIME)(DWORD);
        HMODULE hImm32 = LoadLibrary(TEXT("imm32.dll"));
        if(hImm32 != NULL) {
            auto pImmDisableIME = (pfnImmDisableIME)GetProcAddress(hImm32, "ImmDisableIME");
            if(pImmDisableIME != NULL) pImmDisableIME(-1);
            FreeLibrary(hImm32);
        }
    }

    // enable fancy themed windows controls (v6+), requires McEngine.exe.manifest AND linking to comctl32, for fucks
    // sake only noticeable in MessageBox()-es and a few other dialogs
    {
        INITCOMMONCONTROLSEX icc;
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_WIN95_CLASSES;
        InitCommonControlsEx(&icc);
    }

    // if supported (>= Windows Vista), enable DPI awareness so that GetSystemMetrics returns correct values
    // without this, on e.g. 150% scaling, the screen pixels of a 1080p monitor would be reported by
    // GetSystemMetrics(SM_CXSCREEN/SM_CYSCREEN) as only 720p! also on even newer systems (>= Windows 8.1) we can get
    // WM_DPICHANGED notified
    // enable DPI awareness if not -nodpi
    if(!argMap.contains("-nodpi")) {
        // Windows 8.1+
        // per-monitor dpi scaling
        {
            HMODULE shcore = GetModuleHandle(TEXT("shcore.dll"));
            if(shcore != NULL) {
                typedef HRESULT(WINAPI * PSPDAN)(int);
                PSPDAN pSetProcessDpiAwareness = (PSPDAN)GetProcAddress(shcore, "SetProcessDpiAwareness");
                if(pSetProcessDpiAwareness != NULL) {
                    const HRESULT result = pSetProcessDpiAwareness(2);  // 2 == PROCESS_PER_MONITOR_DPI_AWARE
                    WindowsMain::bSupportsPerMonitorDpiAwareness = (result == S_OK || result == E_ACCESSDENIED);
                }
            }
        }

        if(!WindowsMain::bSupportsPerMonitorDpiAwareness) {
            // Windows Vista+
            // system-wide dpi scaling
            {
                auto pSetProcessDPIAware = GetProcAddress(GetModuleHandle(TEXT("user32.dll")), "SetProcessDPIAware");
                if(pSetProcessDPIAware != NULL) pSetProcessDPIAware();
            }
        }
    }

    HINSTANCE hInstance = GetModuleHandle(NULL);

    auto hIcon =
        static_cast<HICON>(LoadImage(hInstance, MAKEINTRESOURCE(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
    auto hIconSm = static_cast<HICON>(LoadImage(hInstance, MAKEINTRESOURCE(1), IMAGE_ICON, 16, 16, LR_SHARED));

    // prepare window class
    WNDCLASSEXW wc;

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = wndProcWrapper;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = hIcon;  // Set the large icon
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(0x00000000);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WINDOW_TITLE;
    wc.hIconSm = hIconSm;  // Set the small icon

    // register window class
    if(!RegisterClassExW(&wc)) {
        MessageBox(NULL, TEXT("Couldn't RegisterClassEx()!"), TEXT("Fatal Error"), MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    // create the window
    HWND hwnd = createWinWindow(hInstance);
    if(hwnd == NULL) {
        printf("FATAL ERROR: hwnd == NULL!!!\n");
        MessageBox(NULL, TEXT("Couldn't createWinWindow()!"), TEXT("Fatal Error"), MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    // get the screen refresh rate, and set fps_max to that as default
    {
        DEVMODE lpDevMode;
        memset(&lpDevMode, 0, sizeof(DEVMODE));
        lpDevMode.dmSize = sizeof(DEVMODE);
        lpDevMode.dmDriverExtra = 0;

        if(EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &lpDevMode)) {
            f32 displayFrequency = static_cast<f32>(lpDevMode.dmDisplayFrequency);
            cv::fps_max.setValue(displayFrequency * 4.f);
            cv::fps_max.setDefaultFloat(displayFrequency * 4.f);
            cv::fps_max_menu.setValue(displayFrequency);
            cv::fps_max_menu.setDefaultFloat(displayFrequency);
        }
    }

    // initialize raw input
    {
        RAWINPUTDEVICE Rid[1];
        Rid[0].usUsagePage = ((USHORT)0x01);
        Rid[0].usUsage = ((USHORT)0x02);
        Rid[0].dwFlags = /*RIDEV_INPUTSINK | RIDEV_DEVNOTIFY*/ 0;
        Rid[0].hwndTarget = hwnd;
        if(RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])) == FALSE) {
            printf("WARNING: Couldn't RegisterRawInputDevices(), GetLastError() = %i\n", (int)GetLastError());
            MessageBox(NULL, TEXT("Couldn't RegisterRawInputDevices()!"), TEXT("Warning"), MB_ICONEXCLAMATION | MB_OK);
        }
    }

    {
        // initialize COM as apartment-threaded before any libraries get the chance to try initializing it as
        // multi-threaded
        HRESULT hr = doCoInitialize();
        if(hr != S_OK && hr != S_FALSE) {
            printf("WARNING: CoInitialize(COINIT_APARTMENTTHREADED) failed, hr: %#lx\n", hr);
        }
    }

    // NOTE: it seems that focus events get lost between CreateWindow() above and ShowWindow() here
    // can be simulated by sleeping and alt-tab for testing.
    // seems to be a Windows bug? if you switch to the window after all of this, no activate or focus events will be
    // received!
    // Sleep(1000);

    // make the window visible
    ShowWindow(hwnd, SW_SHOWNORMAL);

    // create timers
    Timer *deltaTimer = new Timer();

    // initialize engine
    baseEnv = new WinEnvironment(hwnd, hInstance, argCmdline, argMap);
    engine = new Engine(argc, argv);

    deltaTimer->start();
    deltaTimer->update();

    engine->loadApp();

    this->bHasFocus = this->bHasFocus && (GetForegroundWindow() == hwnd);  // HACKHACK: workaround (1), see above
    bool wasLaunchedInBackgroundAndWaitingForFocus = !this->bHasFocus;     // HACKHACK: workaround (2), see above

    if(this->bHasFocus) engine->onFocusGained();

    DragAcceptFiles(hwnd, TRUE);

    // main loop
    MSG msg;
    msg.message = WM_NULL;
    unsigned long tickCounter = 0;

    while(this->bRunning) {
        VPROF_MAIN();

        // handle window message queue
        {
            VPROF_BUDGET("Events", VPROF_BUDGETGROUP_EVENTS);

            if(cv::mouse_raw_input.getBool() && cv::win_mouse_raw_input_buffer.getBool() && mouse != NULL) {
                // need to process buffered inputs outside of the window procedure and remove those messages first
                this->processBufferedRawInput();

                // process all messages EXCEPT WM_INPUT (leave those for the buffer)
                while(PeekMessageW(&msg, NULL, 0, WM_INPUT - 1, PM_REMOVE) != 0) {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
                while(PeekMessageW(&msg, NULL, WM_INPUT + 1, UINT_MAX, PM_REMOVE) != 0) {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
            } else {
                // non-RawInputBuffer path
                while(PeekMessageW(&msg, NULL, 0U, 0U, PM_REMOVE) != 0) {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
            }
        }

        // HACKHACK: focus bug workaround (3), see above
        {
            if(wasLaunchedInBackgroundAndWaitingForFocus) {
                const bool actuallyGotFocusEvenThoughThereIsNoFocusOrActivateEvent = (GetForegroundWindow() == hwnd);
                if(actuallyGotFocusEvenThoughThereIsNoFocusOrActivateEvent) {
                    wasLaunchedInBackgroundAndWaitingForFocus = false;

                    this->bHasFocus = true;
                    if(!engine->hasFocus()) engine->onFocusGained();
                }
            }
        }

        // update
        {
            deltaTimer->update();
            engine->setFrameTime(deltaTimer->getDelta());

            if(this->bUpdate) engine->onUpdate();
        }

        const bool inBackground = (this->bMinimized || !this->bHasFocus);

        // draw
        {
            const unsigned long interleaved = cv::fps_max_background_interleaved.getInt();
            if(this->bDraw && (!inBackground || interleaved < 2 || tickCounter % interleaved == 0)) {
                engine->onPaint();
            }
        }

        tickCounter++;

        // delay the next frame
        {
            VPROF_BUDGET("FPSLimiter", VPROF_BUDGETGROUP_SLEEP);

            const int target_fps = inBackground
                                       ? cv::fps_max_background.getInt()
                                       : (osu->isInPlayMode() ? cv::fps_max.getInt() : cv::fps_max_menu.getInt());
            FPSLimiter::limit_frames(target_fps);
        }
    }

    doCoUninitialize();

    // release the timers
    SAFE_DELETE(deltaTimer);

    const bool isRestartScheduled = baseEnv->isRestartScheduled();

    // release engine
    SAFE_DELETE(engine);

    // clean up environment
    SAFE_DELETE(baseEnv);

    // finally, destroy the window
    DestroyWindow(hwnd);

    // handle potential restart
    if(isRestartScheduled) {
        wchar_t full_path[MAX_PATH];
        GetModuleFileNameW(NULL, full_path, MAX_PATH);
        STARTUPINFOW startupInfo = {sizeof(STARTUPINFOW)};
        PROCESS_INFORMATION processInfo;
        CreateProcessW(full_path, NULL, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &startupInfo,
                       &processInfo);
    }

    ret = 0;
}

//****************//
//	Message loop  //
//****************//

void WindowsMain::processBufferedRawInput() {
    if(mouse == NULL) return;

    UINT bufferSize = 0;
    UINT result = GetRawInputBuffer(NULL, &bufferSize, sizeof(RAWINPUTHEADER));
    if(std::cmp_equal(result, -1) || bufferSize == 0) {
        return;
    }

    static const bool isWow64 = []() {
        BOOL wow64 = FALSE;
        IsWow64Process(GetCurrentProcess(), &wow64);
        return wow64 != FALSE;
    }();

    // WoW64 needs 8-byte alignment
    static const size_t alignment = isWow64 ? 8 : sizeof(DWORD);

    if(isWow64) {
        bufferSize *= 8;  // WoW64 requirement from MSDN
    }

    // MSDN example allows 16 messages to be queued, seems reasonable?
    bufferSize *= 16;

    bufferSize = std::min(bufferSize, this->maxRawInputBufferSize);
    if(this->vRawInputBuffer.size < bufferSize) {
        this->vRawInputBuffer.resize(bufferSize, alignment);
    }

    auto *rawInputArray = static_cast<RAWINPUT *>(this->vRawInputBuffer.ptr);

    UINT actualBufferSize = bufferSize;
    UINT numInputs = GetRawInputBuffer(rawInputArray, &actualBufferSize, sizeof(RAWINPUTHEADER));
    if(std::cmp_equal(numInputs, -1)) {
        return;
    }

    // process all raw inputs in the buffer
    if(isWow64) {
        // on WoW64, the structure layout is different
        // mouse data is at offset 16+8=24 instead of 16
        auto *currentPtr = reinterpret_cast<BYTE *>(rawInputArray);

        for(UINT i = 0; i < numInputs; i++) {
            auto *header = reinterpret_cast<RAWINPUTHEADER *>(currentPtr);

            if(header->dwType == RIM_TYPEMOUSE) {
                // on WoW64, mouse data starts at header + 24 bytes (16 + 8 padding)
                auto *mouseData = reinterpret_cast<RAWMOUSE *>(currentPtr + 24);

                mouse->onRawMove(mouseData->lLastX, mouseData->lLastY, (mouseData->usFlags & MOUSE_MOVE_ABSOLUTE) != 0,
                                 (mouseData->usFlags & MOUSE_VIRTUAL_DESKTOP) != 0);
            }

            // move to next input, manually calculate size with 8-byte alignment (not sure if this is required)
            UINT inputSize = header->dwSize;
            inputSize = (inputSize + 7) & ~7;  // round up to 8-byte boundary
            currentPtr += inputSize;
        }
    } else {
        RAWINPUT *currentInput = rawInputArray;
        for(UINT i = 0; i < numInputs; i++) {
            if(currentInput->header.dwType == RIM_TYPEMOUSE) {
                const LONG deltaX = currentInput->data.mouse.lLastX;
                const LONG deltaY = currentInput->data.mouse.lLastY;
                const USHORT flags = currentInput->data.mouse.usFlags;

                mouse->onRawMove(deltaX, deltaY, (flags & MOUSE_MOVE_ABSOLUTE) != 0,
                                 (flags & MOUSE_VIRTUAL_DESKTOP) != 0);
            }

            // move to next input in buffer
            currentInput = NEXTRAWINPUTBLOCK(currentInput);
        }
    }
}

LRESULT CALLBACK WindowsMain::realWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_COPYDATA: {
            PCOPYDATASTRUCT cds = (PCOPYDATASTRUCT)lParam;
            if(cds->dwData != WM_NEOSU_PROTOCOL) return 0;

            i32 argc = 0;
            char **argv = (char **)cds->lpData;
            std::vector<std::string> args;
            for(i32 i = 0; i < argc; i++) {
                args.push_back(argv[i]);
            }
            handle_cmdline_args(args);

            return 1;
        }

        case WM_DROPFILES: {
            HDROP hDrop = (HDROP)wParam;
            UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);

            for(UINT i = 0; i < fileCount; i++) {
                UINT pathLength = DragQueryFileW(hDrop, i, NULL, 0);
                wchar_t *filePath = new wchar_t[pathLength + 1];
                DragQueryFileW(hDrop, i, filePath, pathLength + 1);

                // Convert filepath to UTF-8
                int size = WideCharToMultiByte(CP_UTF8, 0, filePath, pathLength, NULL, 0, NULL, NULL);
                std::string utf8filepath(size, 0);
                WideCharToMultiByte(CP_UTF8, 0, filePath, size, (LPSTR)utf8filepath.c_str(), size, NULL, NULL);
                delete[] filePath;

                std::vector<std::string> args;
                args.push_back(utf8filepath.c_str());
                handle_cmdline_args(args);
            }

            DragFinish(hDrop);

            return 0;
        }

        // graceful shutdown request
        case WM_DESTROY:
            if(engine != NULL) engine->shutdown();  // this will in turn trigger a WM_CLOSE
            return 0;

        // alt-f4, window X button press, right click > close, "exit" ConCommand and WM_DESTROY will all send WM_CLOSE
        case WM_CLOSE:
            if(this->bRunning) {
                this->bRunning = false;
                if(engine != NULL) engine->onShutdown();
            }
            return 0;

        // paint nothing on repaint
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
        }
            return 0;

        // focus and active
        case WM_ACTIVATE:
            if(this->bRunning) {
                if(!HIWORD(wParam))  // if we are not minimized
                {
                    // if (engine != NULL)
                    //	printf("WndProc() : WM_ACTIVATE, not minimized\n");

                    this->bUpdate = true;
                    this->bDraw = true;
                } else {
                    // if (engine != NULL)
                    //	printf("WndProc() : WM_ACTIVATE, minimized\n");

                    /// this->bUpdate = false;
                    this->bDraw = false;
                    this->bHasFocus = false;
                }
            }
            break;

        // focus
        case WM_SETFOCUS:
            this->bHasFocus = true;
            if(this->bRunning && engine != NULL) {
                if(!engine->hasFocus()) engine->onFocusGained();
            }
            break;

        case WM_KILLFOCUS:
            this->bHasFocus = false;
            if(this->bRunning && engine != NULL) {
                if(engine->hasFocus()) engine->onFocusLost();
            }
            break;

        // OnKeyDown
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            if(keyboard != NULL) {
                keyboard->onKeyDown(mapLeftRightKeys(wParam, lParam));
                return 0;
            }
            break;

        // OnKeyUp
        case WM_SYSKEYUP:
        case WM_KEYUP:
            if(keyboard != NULL) {
                keyboard->onKeyUp(mapLeftRightKeys(wParam, lParam));
                return 0;
            }
            break;

        // OnCharDown
        case WM_CHAR:
            if(keyboard != NULL) {
                keyboard->onChar(wParam);
                return 0;
            }
            break;

        // left mouse button, inject as keyboard key as well
        case WM_LBUTTONDOWN:
            if(keyboard != NULL && mouse != NULL) {
                mouse->onButtonChange(ButtonIndex::BUTTON_LEFT, true);
                keyboard->onKeyDown(VK_LBUTTON);
            }
            SetCapture(hwnd);
            break;
        case WM_LBUTTONUP:
            if(keyboard != NULL && mouse != NULL) {
                mouse->onButtonChange(ButtonIndex::BUTTON_LEFT, false);
                keyboard->onKeyUp(VK_LBUTTON);
            }
            ReleaseCapture();
            break;

        // middle mouse button, inject as keyboard key as well
        case WM_MBUTTONDOWN:
            if(keyboard != NULL && mouse != NULL) {
                mouse->onButtonChange(ButtonIndex::BUTTON_MIDDLE, true);
                keyboard->onKeyDown(VK_MBUTTON);
            }
            SetCapture(hwnd);
            break;
        case WM_MBUTTONUP:
            if(keyboard != NULL && mouse != NULL) {
                mouse->onButtonChange(ButtonIndex::BUTTON_MIDDLE, false);
                keyboard->onKeyUp(VK_MBUTTON);
            }
            ReleaseCapture();
            break;

        // right mouse button, inject as keyboard key as well
        case WM_RBUTTONDOWN:
            if(keyboard != NULL && mouse != NULL) {
                mouse->onButtonChange(ButtonIndex::BUTTON_RIGHT, true);
                keyboard->onKeyDown(VK_RBUTTON);
            }
            SetCapture(hwnd);
            break;
        case WM_RBUTTONUP:
            if(keyboard != NULL && mouse != NULL) {
                mouse->onButtonChange(ButtonIndex::BUTTON_RIGHT, false);
                keyboard->onKeyUp(VK_RBUTTON);
            }
            ReleaseCapture();
            break;

        // mouse sidebuttons (4 and 5), inject them as keyboard keys as well
        case WM_XBUTTONDOWN:
            if(keyboard != NULL && mouse != NULL) {
                const DWORD fwButton = GET_XBUTTON_WPARAM(wParam);
                if(fwButton == XBUTTON1) {
                    mouse->onButtonChange(ButtonIndex::BUTTON_X1, true);
                    keyboard->onKeyDown(VK_XBUTTON1);
                } else if(fwButton == XBUTTON2) {
                    mouse->onButtonChange(ButtonIndex::BUTTON_X2, true);
                    keyboard->onKeyDown(VK_XBUTTON2);
                }
            }
            return TRUE;
        case WM_XBUTTONUP:
            if(keyboard != NULL && mouse != NULL) {
                const DWORD fwButton = GET_XBUTTON_WPARAM(wParam);
                if(fwButton == XBUTTON1) {
                    mouse->onButtonChange(ButtonIndex::BUTTON_X1, false);
                    keyboard->onKeyUp(VK_XBUTTON1);
                } else if(fwButton == XBUTTON2) {
                    mouse->onButtonChange(ButtonIndex::BUTTON_X2, false);
                    keyboard->onKeyUp(VK_XBUTTON2);
                }
            }
            return TRUE;

        // cursor visibility handling (for client area)
        case WM_SETCURSOR:
            if(!g_bCursorVisible) {             // this variable isn't confusing at all..
                if(LOWORD(lParam) == HTCLIENT)  // hide if in client area
                {
                    if(this->bIsCursorVisible) {
                        this->bIsCursorVisible = false;

                        int check = ShowCursor(false);
                        for(int i = 0; i <= check; i++) {
                            ShowCursor(false);
                        }
                    }
                } else if(!this->bIsCursorVisible)  // show if not in client area (e.g. window border)
                {
                    this->bIsCursorVisible = true;

                    int check = ShowCursor(true);
                    if(check < 0) {
                        for(int i = check; i <= 0; i++) {
                            ShowCursor(true);
                        }
                    }
                }
            }

            // this logic below would mean that we have to handle the cursor when moving from resizing into the client
            // area seems very annoying; unfinished
            /*
            if (LOWORD(lParam) == HTCLIENT) // if we are inside the client area, we handle the cursor
                    return TRUE;
            else
                    break; // if not, let DefWindowProc do its thing
            */

            // avoid cursor flicker when using non-normal cursor set by engine
            if(env && env->getCursor() != CURSORTYPE::CURSOR_NORMAL) return TRUE;

            break;

        // raw input handling (only for mouse movement atm)
        case WM_INPUT: {
            if(!cv::mouse_raw_input.getBool()) break;
            RAWINPUT raw;
            UINT dwSize = sizeof(raw);

            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &dwSize, sizeof(RAWINPUTHEADER));

            if(raw.header.dwType == RIM_TYPEMOUSE) {
                if(mouse != NULL) {
                    mouse->onRawMove(raw.data.mouse.lLastX, raw.data.mouse.lLastY,
                                     (raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0,
                                     (raw.data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0);
                }
            }
        } break;

        // vertical mouse wheel
        case WM_MOUSEWHEEL:
            if(mouse != NULL) mouse->onWheelVertical(GET_WHEEL_DELTA_WPARAM(wParam));
            break;

        // horizontal mouse wheel
        case WM_MOUSEHWHEEL:
            if(mouse != NULL) mouse->onWheelHorizontal(GET_WHEEL_DELTA_WPARAM(wParam));
            break;

        // minimizing/maximizing
        case WM_SYSCOMMAND:
            switch(wParam) {
                case SC_MINIMIZE:
                    this->bDraw = false;
                    /// this->bUpdate = false;
                    this->bMinimized = true;
                    if(engine != NULL) engine->onMinimized();
                    break;
                case SC_MAXIMIZE:
                    this->bMinimized = false;
                    if(engine != NULL) {
                        RECT rect;
                        GetClientRect(hwnd, &rect);
                        engine->onResolutionChange(Vector2(rect.right, rect.bottom));
                        engine->onMaximized();
                    }
                    break;
                case SC_RESTORE:
                    this->bUpdate = true;
                    this->bDraw = true;
                    this->bMinimized = false;
                    if(engine != NULL) engine->onRestored();
                    break;

                // ignore ALT key opening the window context menu
                case SC_KEYMENU:
                    if((lParam >> 16) <= 0)  // still allow for mouse tho
                        return 0;
                    break;
            }
            break;

        // resolution change
        case WM_DISPLAYCHANGE:
        case WM_SIZE:
            /// case WM_EXITSIZEMOVE: // this would fire resize events for every pixel change, destroying performance if
            /// e.g. RenderTargets are resized in onResolutionChange()
            if(engine != NULL && this->bUpdate) {
                RECT rect;
                GetClientRect(hwnd, &rect);
                engine->requestResolutionChange(Vector2(rect.right, rect.bottom));
            }
            break;

        // DPI scaling change (e.g. because user changed scaling in settings, or moved the window to a monitor with
        // different scaling)
        case WM_DPICHANGED:
            if(engine != NULL && baseEnv != NULL) {
                baseEnv->setDPIOverride(HIWORD(wParam));
                engine->onDPIChange();

                RECT *const prcNewWindow = (RECT *)lParam;
                SetWindowPos(hwnd, NULL, prcNewWindow->left, prcNewWindow->top,
                             prcNewWindow->right - prcNewWindow->left, prcNewWindow->bottom - prcNewWindow->top,
                             SWP_NOZORDER | SWP_NOACTIVATE);
            }
            break;

        // resize limit
        case WM_GETMINMAXINFO: {
            WINDOWPLACEMENT wPos;
            { wPos.length = sizeof(WINDOWPLACEMENT); }
            GetWindowPlacement(hwnd, &wPos);

            // min
            LPMINMAXINFO pMMI = (LPMINMAXINFO)lParam;

            // printf("before: %i %i %i %i\n", (int)pMMI->ptMinTrackSize.x, (int)pMMI->ptMinTrackSize.y,
            // (int)pMMI->ptMaxTrackSize.x, (int)pMMI->ptMaxTrackSize.y); printf("window pos: left = %i, top = %i,
            // bottom = %i, right = %i\n", (int)wPos.rcNormalPosition.left, (int)wPos.rcNormalPosition.top,
            // (int)wPos.rcNormalPosition.bottom, (int)wPos.rcNormalPosition.right);

            pMMI->ptMinTrackSize.x = WINDOW_WIDTH_MIN;
            pMMI->ptMinTrackSize.y = WINDOW_HEIGHT_MIN;

            // NOTE: this is only required for OpenGL and custom renderers
            // allow dynamic overscale (offscreen window borders/decorations)
            // this also clamps all user-initiated resolution changes to the resolution of the monitor the window is on

            // HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            //  HACKHACK: use center instead of MonitorFromWindow() in order to workaround windows display scaling
            //  bullshit bug
            POINT centerOfWindow;
            {
                centerOfWindow.x =
                    wPos.rcNormalPosition.left + (wPos.rcNormalPosition.right - wPos.rcNormalPosition.left) / 2;
                centerOfWindow.y =
                    wPos.rcNormalPosition.top + (wPos.rcNormalPosition.bottom - wPos.rcNormalPosition.top) / 2;
            }
            HMONITOR monitor = MonitorFromPoint(centerOfWindow, MONITOR_DEFAULTTONEAREST);
            {
                MONITORINFO info;
                info.cbSize = sizeof(MONITORINFO);

                if(GetMonitorInfo(monitor, &info) >= 0) {
                    RECT clientRect;
                    RECT windowRect;

                    GetClientRect(hwnd, &clientRect);
                    GetWindowRect(hwnd, &windowRect);

                    const LONG decorationsSumWidth = (windowRect.right - windowRect.left) - clientRect.right;
                    const LONG decorationsSumHeight = (windowRect.bottom - windowRect.top) - clientRect.bottom;

                    // printf("window rect: left = %i, top = %i, bottom = %i, right = %i\n", (int)windowRect.left,
                    // (int)windowRect.top, (int)windowRect.bottom, (int)windowRect.right); printf("client rect: left =
                    // %i, top = %i, bottom = %i, right = %i\n", (int)clientRect.left, (int)clientRect.top,
                    // (int)clientRect.bottom, (int)clientRect.right); printf("monitor width = %i, height = %i\n",
                    // (int)std::abs(info.rcMonitor.left - info.rcMonitor.right), (int)std::abs(info.rcMonitor.top -
                    // info.rcMonitor.bottom)); printf("decorations: width = %i, height = %i\n",
                    // (int)decorationsSumWidth, (int)decorationsSumHeight);

                    pMMI->ptMaxTrackSize.x = std::abs(info.rcMonitor.left - info.rcMonitor.right) + decorationsSumWidth;
                    pMMI->ptMaxTrackSize.y =
                        std::abs(info.rcMonitor.top - info.rcMonitor.bottom) + decorationsSumHeight;
                }
            }

            // printf("after: %i %i %i %i\n", (int)pMMI->ptMinTrackSize.x, (int)pMMI->ptMinTrackSize.y,
            // (int)pMMI->ptMaxTrackSize.x, (int)pMMI->ptMaxTrackSize.y);
        }
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WindowsMain::wndProcWrapper(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto *ml = reinterpret_cast<WindowsMain *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if(ml) return ml->realWndProc(hwnd, msg, wParam, lParam);

    // the GWLP_USERDATA will be empty until after the window has already been created, so we have to check this here
    if(msg == WM_NCCREATE && WindowsMain::bSupportsPerMonitorDpiAwareness) {
        typedef BOOL(WINAPI * EPNCDS)(HWND);
        EPNCDS pEnableNonClientDpiScaling =
            (EPNCDS)GetProcAddress(GetModuleHandle(TEXT("user32.dll")), "EnableNonClientDpiScaling");
        if(pEnableNonClientDpiScaling != NULL) pEnableNonClientDpiScaling(hwnd);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

//*******************//
//	Window creation  //
//*******************//

HWND WindowsMain::createWinWindow(HINSTANCE hInstance) {
    HWND hwnd;

    // window style
    LONG_PTR style = WinEnvironment::getWindowStyleWindowed();
    LONG_PTR exStyle = WS_EX_WINDOWEDGE;

    style &= ~WS_VISIBLE;  // always start out invisible, we have a ShowWindow() call later anyway

    int xPos = (GetSystemMetrics(SM_CXSCREEN) / 2) - (WINDOW_WIDTH / 2);
    int yPos = (GetSystemMetrics(SM_CYSCREEN) / 2) - (WINDOW_HEIGHT / 2);
    int width = WINDOW_WIDTH;
    int height = WINDOW_HEIGHT;

    RECT clientArea;
    clientArea.left = xPos;
    clientArea.top = yPos;
    clientArea.right = xPos + width;
    clientArea.bottom = yPos + height;
    AdjustWindowRect(&clientArea, style, FALSE);

    xPos = clientArea.left;
    yPos = clientArea.top;
    width = clientArea.right - clientArea.left;
    height = clientArea.bottom - clientArea.top;

    // create the window
    hwnd = CreateWindowExW(exStyle, WINDOW_TITLE, WINDOW_TITLE, style, xPos, yPos, width, height, NULL, NULL, hInstance,
                           NULL);
    if(hwnd == NULL) {
        MessageBox(NULL, TEXT("Couldn't CreateWindowEx()!"), TEXT("Fatal Error"), MB_ICONEXCLAMATION | MB_OK);
        return NULL;
    }

    // for the wndproc wrapper to cast to ourselves
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // NOTE: Hardcoded "1" from resource.rc
    auto hIcon =
        static_cast<HICON>(LoadImage(hInstance, MAKEINTRESOURCE(1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
    auto hIconSm = static_cast<HICON>(LoadImage(hInstance, MAKEINTRESOURCE(1), IMAGE_ICON, 16, 16, LR_SHARED));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

    return hwnd;
}

#endif
