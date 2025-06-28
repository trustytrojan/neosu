//================ Copyright (c) 2025, WH, All rights reserved. =================//
//
// Purpose:		SDL-based dynamic loading of BASS libraries (workaround linking to broken shared libraries)
//
// $NoKeywords: $snd $bass $loader
//===============================================================================//

#include "BassManager.h"

#if defined(MCENGINE_FEATURE_BASS)

#include "Engine.h"
#include "File.h"

namespace BassManager {
namespace BassFuncs {
template <typename T>
T loadFunction(SDL_SharedObject *lib, const char *funcName) {
    T func = reinterpret_cast<T>(SDL_LoadFunction(lib, funcName));
    if(!func) debugLogF("BassManager: Failed to load function {:s}: {:s}\n", funcName, SDL_GetError());
    return func;
}

#ifdef MCENGINE_PLATFORM_WINDOWS
#define LPREFIX ""
#define LSUFFIX ".dll"
#else
#define LPREFIX "lib"
#define LSUFFIX ".so"
#endif

#define LNAME(x) LPREFIX #x LSUFFIX

// define all the libraries with their properties
// (name, version_func, expected_version, func_group)
#ifdef MCENGINE_PLATFORM_WINDOWS
#define _BASS_WIN_LIBRARIES(X)                                                   \
    X(bassasio, BASS_ASIO_GetVersion, BASSASIOVERSION_REAL, BASS_ASIO_FUNCTIONS) \
    X(basswasapi, BASS_WASAPI_GetVersion, BASSWASAPIVERSION_REAL, BASS_WASAPI_FUNCTIONS)
#else
#define _BASS_WIN_LIBRARIES(X)
#endif

#define BASS_LIBRARIES(X)                                                            \
    X(bass, BASS_GetVersion, BASSVERSION_REAL, BASS_CORE_FUNCTIONS)                  \
    X(bass_fx, BASS_FX_GetVersion, BASSFXVERSION_REAL, BASS_FX_FUNCTIONS)            \
    X(bassmix, BASS_Mixer_GetVersion, BASSMIXVERSION_REAL, BASS_MIX_FUNCTIONS)       \
    X(bassloud, BASS_Loudness_GetVersion, BASSLOUDVERSION_REAL, BASS_LOUD_FUNCTIONS) \
    _BASS_WIN_LIBRARIES(X)

// setup the library handles and paths to check for them
#define DECLARE_LIB(name, ...)                              \
    static SDL_SharedObject *s_lib##name = nullptr;         \
    static constexpr std::initializer_list name##_paths = { \
        LNAME(name), "lib/" LNAME(name)};  // check under lib/ if it's not found in the default search path

BASS_LIBRARIES(DECLARE_LIB)

// generate function pointer definitions from the header
#define DEFINE_BASS_FUNCTION(name) name##_t name = nullptr;

ALL_BASS_FUNCTIONS(DEFINE_BASS_FUNCTION)

#define LOAD_FUNCTION(name) name = loadFunction<name##_t>(currentLib, #name);

#define GENERATE_LIBRARY_LOADER(libname, vfunc, ver, funcgroup)                                                       \
    static bool load_##libname() {                                                                                    \
        failedLoad = #libname;                                                                                        \
        for(auto &path : libname##_paths) {                                                                           \
            s_lib##libname = SDL_LoadObject(path);                                                                    \
            if(!s_lib##libname) continue;                                                                             \
            (vfunc) = loadFunction<vfunc##_t>(s_lib##libname, #vfunc);                                                \
            if(!(vfunc)) {                                                                                            \
                SDL_UnloadObject(s_lib##libname);                                                                     \
                s_lib##libname = nullptr;                                                                             \
                continue;                                                                                             \
            }                                                                                                         \
            uint64_t actualVersion = static_cast<uint64_t>(vfunc());                                                  \
            if(actualVersion >= (ver)) {                                                                              \
                SDL_SharedObject *currentLib = s_lib##libname;                                                        \
                funcgroup(LOAD_FUNCTION) return true;                                                                 \
            }                                                                                                         \
            debugLogF("BassManager: version too old for {:s} (expected {:x}, got {:x})\n", path, ver, actualVersion); \
            SDL_UnloadObject(s_lib##libname);                                                                         \
            s_lib##libname = nullptr;                                                                                 \
            (vfunc) = nullptr;                                                                                        \
        }                                                                                                             \
        debugLogF("BassManager: Failed to load " #libname " library: {:s}\n", SDL_GetError());                        \
        return false;                                                                                                 \
    }

static std::string failedLoad = "none";

BASS_LIBRARIES(GENERATE_LIBRARY_LOADER)

};  // namespace BassFuncs

namespace {
static HPLUGIN plBassFlac = 0;

void unloadPlugins() {
    if(plBassFlac) {
        BASS_PluginEnable(plBassFlac, false);
        BASS_PluginFree(plBassFlac);
        plBassFlac = 0;
    }
}

HPLUGIN loadPlugin(const std::string &pluginname) {
#define LNAMESTR(lib) UString::fmt(LPREFIX "{:s}" LSUFFIX, (lib))
    HPLUGIN ret = 0;
    // handle bassflac plugin separately
    UString tryPath{LNAMESTR(pluginname)};
    if(!env->fileExists(tryPath.toUtf8())) tryPath = UString::fmt("lib{}{}", File::PREF_PATHSEP, LNAMESTR(pluginname));

    // make it a fully qualified path
    // TODO (PORT): need getFolderFromFilePath to give a fully qualified path for this to work

    // if (env->fileExists(tryPath.toUtf8()))
    // 	tryPath = UString::fmt("{}{}", env->getFolderFromFilePath(tryPath.toUtf8()), LNAMESTR(pluginname));
    // else
    // 	tryPath = LNAMESTR(pluginname); // maybe bass will find it?

    ret = BASS_PluginLoad(
        (const char *)tryPath.plat_str(),
        Env::cfg(OS::WINDOWS) ? BASS_UNICODE : 0);  // ??? this wchar_t->char* cast is required for some reason?

    if(ret) {
        if(cv_debug.getBool())
            debugLogF("loaded {:s} version {:#x}\n", pluginname.c_str(), BASS_PluginGetInfo(ret)->version);
        BASS_PluginEnable(ret, true);
    } else {
        failedLoad = std::string(pluginname);
    }

    return ret;
#undef LNAMESTR
}
}  // namespace

bool init() {
    cleanup();  // don't init more than once

#define LOAD_LIBRARY(libname, ...) \
    if(!load_##libname()) {        \
        cleanup();                 \
        return false;              \
    }

    // load all the libraries here
    BASS_LIBRARIES(LOAD_LIBRARY)

    if(!loadPlugin("bassflac")) {
        cleanup();
        return false;
    }

    // if we got here, we loaded everything
    failedLoad = "none";

    return true;
}

void cleanup() {
    // first clean up plugins
    unloadPlugins();

    // unload in reverse order
#define UNLOAD_LIB(name, ...)          \
    if(s_lib##name) {                  \
        SDL_UnloadObject(s_lib##name); \
        s_lib##name = nullptr;         \
    }

#ifdef MCENGINE_PLATFORM_WINDOWS
    UNLOAD_LIB(basswasapi)
    UNLOAD_LIB(bassasio)
#endif
    UNLOAD_LIB(bassloud)
    UNLOAD_LIB(bassmix)
    UNLOAD_LIB(bass_fx)
    UNLOAD_LIB(bass)

    // reset to null
#define RESET_FUNCTION(name) name = nullptr;
    ALL_BASS_FUNCTIONS(RESET_FUNCTION)
}

std::string getFailedLoad() { return failedLoad; }

std::string printBassError(const std::string &context, int code) {
    std::string errstr = fmt::format("Unknown BASS error ({:d})!", code);
    switch(code) {
        case BASS_OK:
            errstr = "No error.\n";
            break;
        case BASS_ERROR_MEM:
            errstr = "Memory error\n";
            break;
        case BASS_ERROR_FILEOPEN:
            errstr = "Can't open the file\n";
            break;
        case BASS_ERROR_DRIVER:
            errstr = "Can't find an available driver\n";
            break;
        case BASS_ERROR_BUFLOST:
            errstr = "The sample buffer was lost\n";
            break;
        case BASS_ERROR_HANDLE:
            errstr = "Invalid handle\n";
            break;
        case BASS_ERROR_FORMAT:
            errstr = "Unsupported sample format\n";
            break;
        case BASS_ERROR_POSITION:
            errstr = "Invalid position\n";
            break;
        case BASS_ERROR_INIT:
            errstr = "BASS_Init has not been successfully called\n";
            break;
        case BASS_ERROR_START:
            errstr = "BASS_Start has not been successfully called\n";
            break;
        case BASS_ERROR_SSL:
            errstr = "SSL/HTTPS support isn't available\n";
            break;
        case BASS_ERROR_REINIT:
            errstr = "Device needs to be reinitialized\n";
            break;
        case BASS_ERROR_ALREADY:
            errstr = "Already initialized\n";
            break;
        case BASS_ERROR_NOTAUDIO:
            errstr = "File does not contain audio\n";
            break;
        case BASS_ERROR_NOCHAN:
            errstr = "Can't get a free channel\n";
            break;
        case BASS_ERROR_ILLTYPE:
            errstr = "An illegal type was specified\n";
            break;
        case BASS_ERROR_ILLPARAM:
            errstr = "An illegal parameter was specified\n";
            break;
        case BASS_ERROR_NO3D:
            errstr = "No 3D support\n";
            break;
        case BASS_ERROR_NOEAX:
            errstr = "No EAX support\n";
            break;
        case BASS_ERROR_DEVICE:
            errstr = "Illegal device number\n";
            break;
        case BASS_ERROR_NOPLAY:
            errstr = "Not playing\n";
            break;
        case BASS_ERROR_FREQ:
            errstr = "Illegal sample rate\n";
            break;
        case BASS_ERROR_NOTFILE:
            errstr = "The stream is not a file stream\n";
            break;
        case BASS_ERROR_NOHW:
            errstr = "No hardware voices available\n";
            break;
        case BASS_ERROR_EMPTY:
            errstr = "The file has no sample data\n";
            break;
        case BASS_ERROR_NONET:
            errstr = "No internet connection could be opened\n";
            break;
        case BASS_ERROR_CREATE:
            errstr = "Couldn't create the file\n";
            break;
        case BASS_ERROR_NOFX:
            errstr = "Effects are not available\n";
            break;
        case BASS_ERROR_NOTAVAIL:
            errstr = "Requested data/action is not available\n";
            break;
        case BASS_ERROR_DECODE:
            errstr = "The channel is/isn't a decoding channel\n";
            break;
        case BASS_ERROR_DX:
            errstr = "A sufficient DirectX version is not installed\n";
            break;
        case BASS_ERROR_TIMEOUT:
            errstr = "Connection timeout\n";
            break;
        case BASS_ERROR_FILEFORM:
            errstr = "Unsupported file format\n";
            break;
        case BASS_ERROR_SPEAKER:
            errstr = "Unavailable speaker\n";
            break;
        case BASS_ERROR_VERSION:
            errstr = "Invalid BASS version\n";
            break;
        case BASS_ERROR_CODEC:
            errstr = "Codec is not available/supported\n";
            break;
        case BASS_ERROR_ENDED:
            errstr = "The channel/file has ended\n";
            break;
        case BASS_ERROR_BUSY:
            errstr = "The device is busy\n";
            break;
        case BASS_ERROR_UNSTREAMABLE:
            errstr = "Unstreamable file\n";
            break;
        case BASS_ERROR_PROTOCOL:
            errstr = "Unsupported protocol\n";
            break;
        case BASS_ERROR_DENIED:
            errstr = "Access Denied\n";
            break;
#ifdef MCENGINE_FEATURE_WINDOWS
        case BASS_ERROR_WASAPI:
            errstr = "No WASAPI\n";
            break;
        case BASS_ERROR_WASAPI_BUFFER:
            errstr = "Invalid buffer size\n";
            break;
        case BASS_ERROR_WASAPI_CATEGORY:
            errstr = "Can't set category\n";
            break;
        case BASS_ERROR_WASAPI_DENIED:
            errstr = "Access denied\n";
            break;
#endif
        case BASS_ERROR_UNKNOWN:  // fallthrough
        default:
            break;
    }
    debugLogF("{:s} error: {:s}", context, errstr);
    return fmt::format("{:s} error: {:s}", context, errstr);  // also return it
}

// XXX: redundant with printBassError, clean this up
UString BassManager::getErrorUString() {
    auto code = BASS_ErrorGetCode();
    switch(code) {
        case BASS_OK:
            return "No error";
        case BASS_ERROR_MEM:
            return "BASS error: Memory error";
        case BASS_ERROR_FILEOPEN:
            return "BASS error: Can't open the file";
        case BASS_ERROR_DRIVER:
            return "BASS error: Can't find an available driver";
        case BASS_ERROR_BUFLOST:
            return "BASS error: The sample buffer was lost";
        case BASS_ERROR_HANDLE:
            return "BASS error: Invalid handle";
        case BASS_ERROR_FORMAT:
            return "BASS error: Unsupported sample format";
        case BASS_ERROR_POSITION:
            return "BASS error: Invalid position";
        case BASS_ERROR_INIT:
            return "BASS error: BASS_Init has not been successfully called";
        case BASS_ERROR_START:
            return "BASS error: BASS_Start has not been successfully called";
        case BASS_ERROR_SSL:
            return "BASS error: SSL/HTTPS support isn't available";
        case BASS_ERROR_REINIT:
            return "BASS error: Device needs to be reinitialized";
        case BASS_ERROR_ALREADY:
            return "BASS error: Already initialized";
        case BASS_ERROR_NOTAUDIO:
            return "BASS error: File does not contain audio";
        case BASS_ERROR_NOCHAN:
            return "BASS error: Can't get a free channel";
        case BASS_ERROR_ILLTYPE:
            return "BASS error: An illegal type was specified";
        case BASS_ERROR_ILLPARAM:
            return "BASS error: An illegal parameter was specified";
        case BASS_ERROR_NO3D:
            return "BASS error: No 3D support";
        case BASS_ERROR_NOEAX:
            return "BASS error: No EAX support";
        case BASS_ERROR_DEVICE:
            return "BASS error: Illegal device number";
        case BASS_ERROR_NOPLAY:
            return "BASS error: Not playing";
        case BASS_ERROR_FREQ:
            return "BASS error: Illegal sample rate";
        case BASS_ERROR_NOTFILE:
            return "BASS error: The stream is not a file stream";
        case BASS_ERROR_NOHW:
            return "BASS error: No hardware voices available";
        case BASS_ERROR_EMPTY:
            return "BASS error: The file has no sample data";
        case BASS_ERROR_NONET:
            return "BASS error: No internet connection could be opened";
        case BASS_ERROR_CREATE:
            return "BASS error: Couldn't create the file";
        case BASS_ERROR_NOFX:
            return "BASS error: Effects are not available";
        case BASS_ERROR_NOTAVAIL:
            return "BASS error: Requested data/action is not available";
        case BASS_ERROR_DECODE:
            return "BASS error: The channel is/isn't a decoding channel";
        case BASS_ERROR_DX:
            return "BASS error: A sufficient DirectX version is not installed";
        case BASS_ERROR_TIMEOUT:
            return "BASS error: Connection timeout";
        case BASS_ERROR_FILEFORM:
            return "BASS error: Unsupported file format";
        case BASS_ERROR_SPEAKER:
            return "BASS error: Unavailable speaker";
        case BASS_ERROR_VERSION:
            return "BASS error: Invalid BASS version";
        case BASS_ERROR_CODEC:
            return "BASS error: Codec is not available/supported";
        case BASS_ERROR_ENDED:
            return "BASS error: The channel/file has ended";
        case BASS_ERROR_BUSY:
            return "BASS error: The device is busy";
        case BASS_ERROR_UNSTREAMABLE:
            return "BASS error: Unstreamable file";
        case BASS_ERROR_PROTOCOL:
            return "BASS error: Unsupported protocol";
        case BASS_ERROR_DENIED:
            return "BASS error: Access Denied";
#ifdef MCENGINE_PLATFORM_WINDOWS
        case BASS_ERROR_WASAPI:
            return "WASAPI error: No WASAPI";
        case BASS_ERROR_WASAPI_BUFFER:
            return "WASAPI error: Invalid buffer size";
        case BASS_ERROR_WASAPI_CATEGORY:
            return "WASAPI error: Can't set category";
        case BASS_ERROR_WASAPI_DENIED:
            return "WASAPI error: Access denied";
#endif
        case BASS_ERROR_UNKNOWN:  // fallthrough
        default:
            return UString::format("Unknown BASS error (%i)!", code);
    }
}

}  // namespace BassManager

#endif
