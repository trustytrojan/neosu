#include "Environment.h"

#include "ConVar.h"

Environment::Environment() { m_bFullscreenWindowedBorderless = false; }

#ifdef _WIN32
i32 Environment::get_nb_cpu_cores() {
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
}
#else
#include <unistd.h>
i32 Environment::get_nb_cpu_cores() { return sysconf(_SC_NPROCESSORS_ONLN); }
#endif

void Environment::setFullscreenWindowedBorderless(bool fullscreenWindowedBorderless) {
    m_bFullscreenWindowedBorderless = fullscreenWindowedBorderless;

    if(env->isFullscreen()) {
        env->disableFullscreen();
        env->enableFullscreen();
    }
}
