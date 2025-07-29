#pragma once

#ifdef _WIN32

#include "WinEnvironment.h"

class Engine;

class WindowsMain final {
   public:
    WindowsMain(int argc, char *argv[], const std::vector<UString> &argCmdline,
                const std::unordered_map<UString, std::optional<UString>> &argMap);
    static int ret;

    // I L L E G A L (sorry, that was the inner McKay in me)
    void handle_cmdline_args(const char *args);

   private:
    bool bRunning{true};
    bool bUpdate{true};
    bool bDraw{true};

    bool bMinimized{false};                       // for fps_max_background
    bool bHasFocus{false};                        // for fps_max_background
    bool bIsCursorVisible{true};                  // local variable
    static bool bSupportsPerMonitorDpiAwareness;  // checked in wndproc on creation (WM_NCCREATE) for enabling
                                                  // non-client dpi scaling

    HWND createWinWindow(HINSTANCE hInstance);

    static LRESULT CALLBACK wndProcWrapper(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK realWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    WPARAM mapLeftRightKeys(WPARAM wParam, LPARAM lParam);

    HRESULT doCoInitialize();
    void doCoUninitialize();
    FARPROC doLoadComBaseFunction(const char *name);

    // rawinputbuffer utils
    struct AlignedBuffer {
        void *ptr{nullptr};
        size_t size{0};

        ~AlignedBuffer() {
            if(this->ptr) _aligned_free(this->ptr);
        }

        void resize(size_t newSize, size_t alignment) {
            if(newSize > this->size) {
                if(this->ptr) _aligned_free(this->ptr);
                this->ptr = _aligned_malloc(newSize, alignment);
                this->size = newSize;
            }
        }
    };

    static constexpr UINT maxRawInputBufferSize{64 * 1024};
    AlignedBuffer vRawInputBuffer;

    void processBufferedRawInput();

    // misc helpers (which shouldn't be here, to be moved)
    void handle_osk(const char *osk_path);
    void handle_osz(const char *osz_path);
    void handle_neosu_url(const char *url);
    void register_neosu_file_associations();
};

using Main = WindowsMain;

extern EnvironmentImpl *baseEnv;
extern Main *mainloopPtrHack;

#endif
