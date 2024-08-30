#include "Environment.h"

#include "ConVar.h"

Environment::Environment() { m_bFullscreenWindowedBorderless = false; }

void Environment::setFullscreenWindowedBorderless(bool fullscreenWindowedBorderless) {
    m_bFullscreenWindowedBorderless = fullscreenWindowedBorderless;

    if(env->isFullscreen()) {
        env->disableFullscreen();
        env->enableFullscreen();
    }
}
