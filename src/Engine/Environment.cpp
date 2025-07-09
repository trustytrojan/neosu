#include "Environment.h"

#include "ConVar.h"
#include "File.h"

Environment::Environment() { this->bFullscreenWindowedBorderless = false; }

void Environment::setFullscreenWindowedBorderless(bool fullscreenWindowedBorderless) {
    this->bFullscreenWindowedBorderless = fullscreenWindowedBorderless;

    if(env->isFullscreen()) {
        env->disableFullscreen();
        env->enableFullscreen();
    }
}

// modifies the input filename! (checks case insensitively past the last slash)
bool Environment::fileExists(std::string &filename) {
    return File::existsCaseInsensitive(filename) == File::FILETYPE::FILE;
}

// modifies the input directoryName! (checks case insensitively past the last slash)
bool Environment::directoryExists(std::string &directoryName) {
    return File::existsCaseInsensitive(directoryName) == File::FILETYPE::FOLDER;
}

// same as the above, but for string literals (so we can't check insensitively and modify the input)
bool Environment::fileExists(const std::string &filename) { return File::exists(filename) == File::FILETYPE::FILE; }

bool Environment::directoryExists(const std::string &directoryName) {
    return File::exists(directoryName) == File::FILETYPE::FOLDER;
}
