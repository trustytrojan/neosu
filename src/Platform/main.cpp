// cross-platform entry point

#include <filesystem>

#include "BaseEnvironment.h"

#include "SString.h"
#include "fmt/format.h"

#include "LinuxMain.h"
#include "WindowsMain.h"

#if defined(__SSE__) || (defined(_M_IX86_FP) && (_M_IX86_FP > 0))
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <xmmintrin.h>
#endif
#define SET_FPU_DAZ_FTZ _mm_setcsr(_mm_getcsr() | 0x8040);
#else
#define SET_FPU_DAZ_FTZ
#endif

// this redefines main() if necessary for the target platform
#include <SDL3/SDL_main.h>

int Main::ret = 1;
EnvironmentImpl *baseEnv = nullptr;

namespace {
void setcwdexe(const std::string &exePathStr) noexcept {
    if constexpr(Env::cfg(OS::WASM)) return;
    // Fix path in case user is running it from the wrong folder.
    // We only do this if MCENGINE_DATA_DIR is set to its default value, since if it's changed,
    // the packager clearly wants the executable in a different location.
    std::string dataDir{MCENGINE_DATA_DIR};
    if(dataDir != "./" && dataDir != ".\\") {
        return;
    }
    namespace fs = std::filesystem;

    bool failed = true;
    std::error_code ec;

    UString uPath{exePathStr.c_str()};
    fs::path exe_path{uPath.plat_str()};

    if(!exe_path.empty() && exe_path.has_parent_path()) {
        fs::current_path(exe_path.parent_path(), ec);
        failed = !!ec;
    }

    if(failed) {
        printf("WARNING: failed to set working directory to parent of %s\n", exePathStr.c_str());
    }
}
}  // namespace

int main(int argc, char *argv[]) {
    // improve floating point perf in case this isn't already enabled by the compiler
    SET_FPU_DAZ_FTZ

    // set the current working directory to the executable directory, so that relative paths
    // work as expected
    // this also sets and caches the path in getPathToSelf, so this must be called here
    setcwdexe(Environment::getPathToSelf(argv[0]));

    std::string lowerPackageName = PACKAGE_NAME;
    SString::lower(lowerPackageName);

    // set up some common app metadata (SDL says these should be called as early as possible)
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, PACKAGE_NAME);
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, NEOSU_VERSION);
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING,
                               fmt::format("com.mcengine.{}", lowerPackageName).c_str());
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "kiwec/spectator/McKay");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING, "MIT/GPL3");  // neosu is gpl3, mcengine is mit
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, PACKAGE_URL);
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");

    // parse args
    auto argMap = [&]() -> std::unordered_map<UString, std::optional<UString>> {
        // example usages:
        // args.contains("-file")
        // auto filename = args["-file"].value_or("default.txt");
        // if (args["-output"].has_value())
        // 	auto outfile = args["-output"].value();
        std::unordered_map<UString, std::optional<UString>> args;
        for(int i = 1; i < argc; ++i) {
            std::string_view arg = argv[i];
            if(arg.starts_with('-'))
                if(i + 1 < argc && !(argv[i + 1][0] == '-')) {
                    args[UString(arg)] = argv[i + 1];
                    ++i;
                } else
                    args[UString(arg)] = std::nullopt;
            else
                args[UString(arg)] = std::nullopt;
        }
        return args;
    }();

    // simple vector representation of the whole cmdline including the program name (as the first element)
    auto cmdLine = std::vector<UString>(argv, argv + argc);

    auto ml = Main(argc, argv, cmdLine, argMap);
    return ml.ret;
}
