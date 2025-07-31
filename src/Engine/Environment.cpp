#include "Environment.h"

#include "ConVar.h"
#include "Engine.h"
#include "File.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_misc.h>
#include <SDL3/SDL_thread.h>

#ifdef MCENGINE_PLATFORM_WINDOWS
#include "WinDebloatDefs.h"
#include <stringapiset.h>
#include <libloaderapi.h>
#endif

#include <string>
#include <sstream>
#include <iomanip>

// convar callback
void Environment::setProcessPriority(float newPrio) {
    SDL_SetCurrentThreadPriority(!!static_cast<int>(newPrio) ? SDL_THREAD_PRIORITY_HIGH : SDL_THREAD_PRIORITY_NORMAL);
}

Environment::Environment(const std::vector<UString> &argCmdline,
                         const std::unordered_map<UString, std::optional<UString>> &argMap)
    : mArgMap(argMap), vCmdLine(argCmdline) {
    this->bFullscreenWindowedBorderless = false;

    // set high priority right away
    Environment::setProcessPriority(cv::win_processpriority.getFloat());
}

void Environment::setFullscreenWindowedBorderless(bool fullscreenWindowedBorderless) {
    this->bFullscreenWindowedBorderless = fullscreenWindowedBorderless;

    if(env->isFullscreen()) {
        env->disableFullscreen();
        env->enableFullscreen();
    }
}

// cached on startup from main.cpp with argv[0] passed, after that argv0 can be null
const std::string &Environment::getPathToSelf(const char *argv0) {
    static std::string pathStr{};
    if(!pathStr.empty()) return pathStr;
    namespace fs = std::filesystem;

    std::error_code ec;
    fs::path exe_path{};

    if constexpr(Env::cfg(OS::LINUX))
        exe_path = fs::canonical("/proc/self/exe", ec);
    else {
        UString uPath{argv0};
        exe_path = fs::canonical(fs::path(uPath.plat_str()), ec);
    }

    if(!ec && !exe_path.empty())  // canonical path found
    {
        UString uPath{exe_path.string().c_str()};
        pathStr = uPath.toUtf8();
    } else {
#if defined(MCENGINE_PLATFORM_WINDOWS)  // fallback to GetModuleFileNameW
        std::array<wchar_t, MAX_PATH> buf;
        GetModuleFileNameW(nullptr, buf.data(), MAX_PATH);

        auto size = WideCharToMultiByte(CP_UTF8, 0, buf.data(), buf.size(), NULL, 0, NULL, NULL);
        std::string utf8path(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, buf.data(), size, (LPSTR)utf8path.c_str(), size, NULL, NULL);
        pathStr = utf8path;
#else
#ifndef MCENGINE_PLATFORM_LINUX
        printf("WARNING: unsupported platform for " __FUNCTION__ "\n");
#endif
        std::string sp;
        std::ifstream("/proc/self/comm") >> sp;
        if(!sp.empty()) {  // fallback to data dir + self
            pathStr = MCENGINE_DATA_DIR + sp;
        } else {  // fallback to data dir + package name
            pathStr = std::string{MCENGINE_DATA_DIR} + PACKAGE_NAME;
        }
#endif
    }
    return pathStr;
}

std::string Environment::encodeStringToURL(const std::string &stringToConvert) noexcept {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for(const char c : stringToConvert) {
        // keep alphanumerics and other accepted characters intact
        if(std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/') {
            escaped << c;
        } else {
            // any other characters are percent-encoded
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
            escaped << std::nouppercase;
        }
    }

    return escaped.str();
}

std::string Environment::filesystemPathToURI(const std::filesystem::path &path) noexcept {
    namespace fs = std::filesystem;
    // convert to absolute path and normalize
    auto abs_path = fs::absolute(path);
    // convert to path with forward slashes
    const UString path_str = UString{abs_path.generic_string()};
    // URL encode the path
    std::string uri = encodeStringToURL(path_str.toUtf8());

    // prepend with file:///
    if(uri[0] == '/')
        uri = fmt::format("file://{}", uri);
    else
        uri = fmt::format("file:///{}", uri);

    // add trailing slash if it's a directory
    if(fs::is_directory(abs_path) && !uri.ends_with('/')) {
        uri += '/';
    }
    return uri;
}

void Environment::openURLInDefaultBrowser(const std::string &url) noexcept {
    if(!SDL_OpenURL(url.c_str())) {
        debugLogF("Failed to open URL: {:s}\n", SDL_GetError());
    }
}

// just open the file manager in a certain folder, but not do anything with it
void Environment::openFileBrowser(const std::string &initialpath) noexcept {
    std::string pathToOpen{initialpath};
    if(pathToOpen.empty())
        pathToOpen = getExeFolder();
    else
        pathToOpen = getFolderFromFilePath(pathToOpen);

    namespace fs = std::filesystem;
    std::string encodedPath =
        Env::cfg(OS::WINDOWS) ? fmt::format("file:///{}", pathToOpen) : filesystemPathToURI(fs::path{pathToOpen});

    if(!SDL_OpenURL(encodedPath.c_str()))
        debugLogF("Failed to open file URI {:s}: {:s}\n", encodedPath, SDL_GetError());
}

std::string Environment::getEnvVariable(const std::string &varToQuery) noexcept {
    static SDL_Environment *sdlEnv = nullptr;
    static bool sdlEnvInit = false;
    if(!sdlEnvInit) {
        sdlEnvInit = true;
        sdlEnv = SDL_GetEnvironment();
    }

    const char *varVal = nullptr;
    if(sdlEnv && !varToQuery.empty()) {
        varVal = SDL_GetEnvironmentVariable(sdlEnv, varToQuery.c_str());
        if(varVal) {
            return std::string{varVal};
        }
    }
    return {""};
}

const std::string &Environment::getExeFolder() {
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

    userdataStr = ".";  // set it to non-empty to avoid endlessly failing if SDL_GetPrefPath fails once

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
    return SDL_CreateDirectory(directoryName.c_str());  // returns true if it already exists
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
    const auto extIdx = filepath.find_last_of('.');
    if(extIdx != std::string::npos) {
        return filepath.substr(extIdx + 1);
    } else
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

// used to signal main loop that SDL_PumpEvents should be called
int Environment::s_sdl_dialog_opened = 0;

void Environment::openFileWindow(FileDialogCallback callback, const char *filetypefilters, const UString & /*title*/,
                                 const UString &initialpath) {
    // convert filetypefilters (Windows-style)
    std::vector<std::string> filterNames;
    std::vector<std::string> filterPatterns;
    std::vector<SDL_DialogFileFilter> sdlFilters;

    if(filetypefilters && *filetypefilters) {
        const char *curr = filetypefilters;
        // add the filetype filters to the SDL dialog filter
        while(*curr) {
            const char *name = curr;
            curr += strlen(name) + 1;

            if(!*curr) break;

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
    s_sdl_dialog_opened++;
    SDL_ShowOpenFileDialog(sdlFileDialogCallback, callbackData, nullptr,
                           sdlFilters.empty() ? nullptr : sdlFilters.data(), static_cast<int>(sdlFilters.size()),
                           initialpath.length() > 0 ? initialpath.toUtf8() : nullptr, false);
}

void Environment::openFolderWindow(FileDialogCallback callback, const UString &initialpath) {
    // callback data to be passed to SDL
    auto *callbackData = new FileDialogCallbackData{std::move(callback)};

    // show it
    s_sdl_dialog_opened++;
    SDL_ShowOpenFolderDialog(sdlFileDialogCallback, callbackData, nullptr,
                             initialpath.length() > 0 ? initialpath.toUtf8() : nullptr, false);
}

// internal

// TODO: filter?
// for open{File,Folder}Window
void Environment::sdlFileDialogCallback(void *userdata, const char *const *filelist, int /*filter*/) {
    auto *callbackData = static_cast<FileDialogCallbackData *>(userdata);
    if(!callbackData) {
        s_sdl_dialog_opened--;
        return;
    }

    std::vector<UString> results;

    if(filelist) {
        for(const char *const *curr = filelist; *curr; curr++) {
            results.emplace_back(*curr);
        }
    }

    // call the callback
    callbackData->callback(results);

    // data is no longer needed
    delete callbackData;
    s_sdl_dialog_opened--;
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
        if(ec) continue;
        auto fileType = entry.status(ec).type();

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

    std::string retPath{path};

    // find the last path separator (either / or \)
    auto lastSlash = retPath.find_last_of('/');
    if(lastSlash == std::string::npos) lastSlash = retPath.find_last_of('\\');

    if(folder) {
        // if path ends with separator, it's already a directory
        bool endsWithSeparator = retPath.back() == '/' || retPath.back() == '\\';

        UString ustrPath{retPath};
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
            if(lastSlash != std::string::npos)  // return parent
                ustrPath = ustrPath.substr(0, lastSlash);
            else  // no separators found, just use ./
                ustrPath = UString::fmt("." PREF_PATHSEP "{}", ustrPath);
        }
        retPath = ustrPath.utf8View();
        // make sure whatever we got now ends with a slash
        if(retPath.back() != '/' && retPath.back() != '\\') retPath = retPath + PREF_PATHSEP;
    } else if(lastSlash != std::string::npos)  // just return the file
    {
        retPath = retPath.substr(lastSlash + 1);
    }
    // else: no separators found, entire path is the filename

    return retPath;
}
