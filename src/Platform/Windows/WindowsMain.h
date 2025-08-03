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
    void handle_cmdline_args(std::vector<std::string> args);

   private:
    bool bRunning{true};
    bool bUpdate{true};
    bool bDraw{true};

    bool bMinimized{false};       // for fps_max_background
    bool bHasFocus{false};        // for fps_max_background
    bool bIsCursorVisible{true};  // local variable

    HWND createWinWindow(HINSTANCE hInstance);

    static LRESULT CALLBACK wndProcWrapper(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK realWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    WPARAM mapLeftRightKeys(WPARAM wParam, LPARAM lParam);

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

    // external setup stuff
    struct Setup {
        static HRESULT com_init();
        static void com_uninit(void);
        static void disable_ime();
        static void dpi_early();
        static void dpi_late(HWND hwnd);

       private:
        Setup() = default;
    };

    // misc helpers (which shouldn't be here, to be moved)
    void handle_db(const char *db_path);
    void handle_osk(const char *osk_path);
    void handle_osz(const char *osz_path);
    void handle_neosu_url(const char *url);
    void register_neosu_file_associations();
};

using Main = WindowsMain;

extern EnvironmentImpl *baseEnv;
extern Main *mainloopPtrHack;

#endif
