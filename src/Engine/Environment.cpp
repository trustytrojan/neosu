#include "Environment.h"

#include "ConVar.h"

Environment::Environment() { this->bFullscreenWindowedBorderless = false; }

void Environment::setFullscreenWindowedBorderless(bool fullscreenWindowedBorderless) {
    this->bFullscreenWindowedBorderless = fullscreenWindowedBorderless;

    if(env->isFullscreen()) {
        env->disableFullscreen();
        env->enableFullscreen();
    }
}
