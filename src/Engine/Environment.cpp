#include "Environment.h"

#include "ConVar.h"
#include "Engine.h"
#include "File.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_dialog.h>

Environment::Environment() { this->bFullscreenWindowedBorderless = false; }

void Environment::setFullscreenWindowedBorderless(bool fullscreenWindowedBorderless) {
    this->bFullscreenWindowedBorderless = fullscreenWindowedBorderless;

    if(env->isFullscreen()) {
        env->disableFullscreen();
        env->enableFullscreen();
    }
}

const std::string &Environment::getExecutablePath() {
    static std::string pathStr{};
    if(!pathStr.empty()) return pathStr;
    // sdl caches this internally, but we'll cache a std::string representation of it
    const char *path = SDL_GetBasePath();

    if(!path)
        pathStr = "." PREF_PATHSEP;
    else
        pathStr = path;
    return pathStr;
}

// i.e. toplevel appdata path
const std::string &Environment::getUserDataPath() {
    static std::string userdataStr{};
    if(!userdataStr.empty()) return userdataStr;

    char *path = SDL_GetPrefPath("", "");
    if(path != nullptr) {
        userdataStr = path;
        // since this is kind of an abuse of SDL_GetPrefPath, we remove the extra slashes at the end
        while(!userdataStr.empty() && (userdataStr.ends_with('\\') || userdataStr.ends_with('/'))) {
            userdataStr.pop_back();
        }
        userdataStr.append(PREF_PATHSEP);
        SDL_free(path);
    }

    return userdataStr;
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

bool Environment::createDirectory(const std::string &directoryName) {
    return SDL_CreateDirectory(directoryName.c_str());
}

bool Environment::renameFile(const std::string &oldFileName, const std::string &newFileName) {
    if(oldFileName == newFileName) {
        return true;
    }
    if(!SDL_RenamePath(oldFileName.c_str(), newFileName.c_str())) {
        std::string tempFile{newFileName + ".tmp"};
        if(!SDL_CopyFile(oldFileName.c_str(), tempFile.c_str())) {
            return false;
        }
        if(!SDL_RenamePath(tempFile.c_str(), newFileName.c_str())) {
            return false;
        }
        // return true if we were able to copy the path (and the file exists in the end), even if removing the old file
        // didn't work
        SDL_RemovePath(oldFileName.c_str());
    }
    return Environment::fileExists(newFileName);
}

bool Environment::deleteFile(const std::string &filePath) { return SDL_RemovePath(filePath.c_str()); }

std::vector<std::string> Environment::getFilesInFolder(const std::string &folder) noexcept {
    return enumerateDirectory(folder, SDL_PATHTYPE_FILE);
}

std::vector<std::string> Environment::getFoldersInFolder(const std::string &folder) noexcept {
    return enumerateDirectory(folder, SDL_PATHTYPE_DIRECTORY);
}

std::string Environment::getFileNameFromFilePath(const std::string &filepath) noexcept {
    return getThingFromPathHelper(filepath, false);
}

std::string Environment::getFolderFromFilePath(const std::string &filepath) noexcept {
    return getThingFromPathHelper(filepath, true);
}

std::string Environment::getFileExtensionFromFilePath(const std::string &filepath, bool /*includeDot*/) noexcept {
    const int idx = UString{filepath}.findLast(".");
    if(idx != -1)
        return filepath.substr(idx + 1);
    else
        return {""};
}

void Environment::showMessageInfo(const UString &title, const UString &message) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, title.toUtf8(), message.toUtf8(), nullptr);
}

void Environment::showMessageWarning(const UString &title, const UString &message) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, title.toUtf8(), message.toUtf8(), nullptr);
}

void Environment::showMessageError(const UString &title, const UString &message) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.toUtf8(), message.toUtf8(), nullptr);
}

void Environment::showMessageErrorFatal(const UString &title, const UString &message) {
    showMessageError(title, message);
}

void Environment::openFileWindow(FileDialogCallback callback, const char *filetypefilters, const UString & /*title*/, const UString &initialpath)
{
	// convert filetypefilters (Windows-style)
	std::vector<std::string> filterNames;
	std::vector<std::string> filterPatterns;
	std::vector<SDL_DialogFileFilter> sdlFilters;

	if (filetypefilters && *filetypefilters)
	{
		const char *curr = filetypefilters;
		// add the filetype filters to the SDL dialog filter
		while (*curr)
		{
			const char *name = curr;
			curr += strlen(name) + 1;

			if (!*curr)
				break;

			const char *pattern = curr;
			curr += strlen(pattern) + 1;

			filterNames.emplace_back(name);
			filterPatterns.emplace_back(pattern);

			SDL_DialogFileFilter filter = {filterNames.back().c_str(), filterPatterns.back().c_str()};
			sdlFilters.push_back(filter);
		}
	}

	// callback data to be passed to SDL
	auto *callbackData = new FileDialogCallbackData{std::move(callback)};

	// show it
	SDL_ShowOpenFileDialog(sdlFileDialogCallback, callbackData, nullptr, sdlFilters.empty() ? nullptr : sdlFilters.data(), static_cast<int>(sdlFilters.size()),
	                       initialpath.length() > 0 ? initialpath.toUtf8() : nullptr, false);
}

void Environment::openFolderWindow(FileDialogCallback callback, const UString &initialpath)
{
	// callback data to be passed to SDL
	auto *callbackData = new FileDialogCallbackData{std::move(callback)};

	// show it
	SDL_ShowOpenFolderDialog(sdlFileDialogCallback, callbackData, nullptr, initialpath.length() > 0 ? initialpath.toUtf8() : nullptr, false);
}

// internal

// TODO: filter?
// for open{File,Folder}Window
void Environment::sdlFileDialogCallback(void *userdata, const char *const *filelist, int  /*filter*/)
{
	auto *callbackData = static_cast<FileDialogCallbackData *>(userdata);
	if (!callbackData)
		return;

	std::vector<UString> results;

	if (filelist)
	{
		for (const char *const *curr = filelist; *curr; curr++)
		{
			results.emplace_back(*curr);
		}
	}

	// call the callback
	callbackData->callback(results);

	// data is no longer needed
	delete callbackData;
}

// for getting files in folder/ folders in folder
std::vector<std::string> Environment::enumerateDirectory(const std::string &pathToEnum,
                                                         /* enum SDL_PathType */ unsigned int type) noexcept {
    namespace fs = std::filesystem;

    std::vector<std::string> contents;
    contents.reserve(512);

    std::error_code ec;
    const bool wantFiles = (type == SDL_PATHTYPE_FILE);
    const bool wantDirs = (type == SDL_PATHTYPE_DIRECTORY);

    for(const auto &entry : fs::directory_iterator(pathToEnum, ec)) {
        auto fileType = entry.status(ec).type();
        if(ec) continue;

        if((wantFiles && fileType == fs::file_type::regular) || (wantDirs && fileType == fs::file_type::directory)) {
            const auto &filename = entry.path().filename();
            contents.emplace_back(filename.string());
        }
    }

    if(ec && contents.empty()) {
        debugLogF("Failed to enumerate directory: {}\n", ec.message());
    }

    return contents;
}

// folder = true means return the canonical filesystem path to the folder containing the given path
//			if the path is already a folder, just return it directly
// folder = false means to strip away the file path separators from the given path and return just the filename itself
std::string Environment::getThingFromPathHelper(const std::string &path, bool folder) noexcept {
    if(path.empty()) return path;
    namespace fs = std::filesystem;

    UString ustrPath{path};  // convenience for rebasing, findlast is kinda annoying

    // find the last path separator (either / or \)
    int lastSlash = ustrPath.findLast("/");
    if(lastSlash == -1) lastSlash = ustrPath.findLast("\\");

    if(folder) {
        // if path ends with separator, it's already a directory
        bool endsWithSeparator = ustrPath.endsWith("/") || ustrPath.endsWith("\\");

        std::error_code ec;
        auto abs_path = fs::canonical(ustrPath.plat_str(), ec);

        if(!ec)  // canonical path found
        {
            auto status = fs::status(abs_path, ec);
            // if it's already a directory or it doesn't have a parent path then just return it directly
            if(ec || status.type() == fs::file_type::directory || !abs_path.has_parent_path())
                ustrPath = abs_path.c_str();
            // else return the parent directory for the file
            else if(abs_path.has_parent_path() && !abs_path.parent_path().empty())
                ustrPath = abs_path.parent_path().c_str();
        } else if(!endsWithSeparator)  // canonical failed, handle manually (if it's not already a directory)
        {
            if(lastSlash != -1)  // return parent
                ustrPath = ustrPath.substr(0, lastSlash);
            else  // no separators found, just use ./
                ustrPath = UString::fmt(".{}{}", Env::cfg(OS::WINDOWS) ? "\\" : "/", ustrPath);
        }
        // make sure whatever we got now ends with a slash
        if(!ustrPath.endsWith("/") && !ustrPath.endsWith("\\")) ustrPath = ustrPath + PREF_PATHSEP;
    } else if(lastSlash != -1)  // just return the file
    {
        ustrPath = ustrPath.substr(lastSlash + 1);
    }
    // else: no separators found, entire path is the filename

    return std::string{ustrPath.utf8View()};
}
