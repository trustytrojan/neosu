include(FetchContent)
include(cmake-win/dependencies.cmake)

# You want options in cache otherwise everything might rebuild
macro(setcb var val)
	set(${var} ${val} CACHE BOOL "" FORCE)
endmacro()

## Source dependencies
setcb(SDL_SHARED OFF)
setcb(SDL_STATIC ON)
setcb(SDL_HIDAPI_LIBUSB OFF)
FetchContent_Declare(sdl3 URL ${SDL3_URL} URL_HASH ${SDL3_HASH})

setcb(FT_DISABLE_HARFBUZZ ON)
setcb(FT_DISABLE_BROTLI ON)
setcb(FT_REQUIRE_ZLIB ON)
setcb(FT_REQUIRE_PNG ON)
setcb(FT_REQUIRE_BZIP2 ON)
FetchContent_Declare(ft2 URL ${FREETYPE2_URL} URL_HASH ${FREETYPE2_HASH})

include(ProcessorCount)
ProcessorCount(CPU_COUNT)
if(CPU_COUNT EQUAL 0)
    set(CPU_COUNT 4)
endif()

include(ExternalProject)
ExternalProject_Add(jpeg
    URL ${LIBJPEG_URL}
    URL_HASH ${LIBJPEG_HASH}	
    CMAKE_ARGS
        -DENABLE_STATIC=ON
        -DENABLE_SHARED=OFF
        -DWITH_JPEG8=ON
        -DWITH_SIMD=ON
        -DREQUIRE_SIMD=OFF
        -DCMAKE_ASM_NASM_FLAGS="-DPIC"
        # -DCMAKE_ASM_NASM_OBJECT_FORMAT=win64
	INSTALL_COMMAND ""
)

setcb(PNG_SHARED OFF)
setcb(PNG_STATIC ON)
setcb(PNG_TESTS OFF)
setcb(PNG_TOOLS OFF)
if(WIN32)
	setcb(MINGW ON)
endif()
FetchContent_Declare(png URL ${LIBPNG_URL} URL_HASH ${LIBPNG_HASH})

setcb(ZLIB_COMPAT ON)
setcb(ZLIB_ENABLE_TESTS OFF)
FetchContent_Declare(zlib URL ${ZLIB_URL} URL_HASH ${ZLIB_HASH})

setcb(ENABLE_STATIC_RUNTIME ON)
setcb(ENABLE_STATIC_LIB ON)
setcb(ENABLE_SHARED_LIB OFF)
setcb(ENABLE_LIB_ONLY ON)
FetchContent_Declare(bz2 URL ${BZIP2_URL} URL_HASH ${BZIP2_HASH})

setcb(FMT_CUDA_TEST OFF)
setcb(FMT_DOC OFF)
setcb(FMT_FUZZ OFF)
setcb(FMT_INSTALL ON)
setcb(FMT_MODULE OFF)
setcb(FMT_OS ON)
setcb(FMT_PEDANTIC OFF)
setcb(FMT_SYSTEM_HEADERS OFF)
setcb(FMT_TEST OFF)
setcb(FMT_UNICODE ON)
setcb(FMT_WERROR OFF)
FetchContent_Declare(fmt URL ${FMT_URL} URL_HASH ${FMT_HASH})

setcb(GLM_BUILD_TESTS OFF)
setcb(GLM_BUILD_LIBRARY OFF)
setcb(GLM_ENABLE_CXX_20 ON)
setcb(GLM_ENABLE_LANG_EXTENSIONS ON)
FetchContent_Declare(glm URL ${GLM_URL} URL_HASH ${GLM_HASH})

setcb(BUILD_TESTING OFF)
setcb(XZ_TOOL_LZMADEC OFF)
setcb(XZ_TOOL_LZMAINFO OFF)
setcb(XZ_TOOL_SCRIPTS OFF)
setcb(XZ_TOOL_SYMLINKS OFF)
setcb(XZ_TOOL_SYMLINKS_LZMA OFF)
setcb(XZ_TOOL_XZ OFF)
setcb(XZ_TOOL_XZDEC OFF)
setcb(XZ_SANDBOX no)
setcb(XZ_THREADS yes)
FetchContent_Declare(lzma URL ${LZMA_URL} URL_HASH ${LZMA_HASH})

setcb(ENABLE_BZ2LIB ON)
setcb(ENABLE_ZLIB ON)
setcb(ENABLE_LZMA ON)
setcb(ENABLE_CAT_SHARED OFF)
setcb(ENABLE_CAT OFF)
setcb(ENABLE_CPIO_SHARED OFF)
setcb(ENABLE_CPIO OFF)
setcb(ENABLE_EXPAT OFF)
setcb(ENABLE_ICONV OFF)
setcb(ENABLE_LIBB2 OFF)
setcb(ENABLE_LZ4 OFF)
setcb(ENABLE_LZO OFF)
setcb(ENABLE_MBEDTLS OFF)
setcb(ENABLE_NETTLE OFF)
setcb(ENABLE_OPENSSL OFF)
setcb(ENABLE_SAFESEH OFF)
setcb(ENABLE_TAR_SHARED OFF)
setcb(ENABLE_TAR OFF)
setcb(ENABLE_TEST OFF)
setcb(ENABLE_UNZIP_SHARED OFF)
setcb(ENABLE_UNZIP OFF)
setcb(ENABLE_WIN32_XMLLITE OFF)
setcb(ENABLE_XML2 OFF)
setcb(ENABLE_ZSTD OFF)
FetchContent_Declare(libarchive URL ${LIBARCHIVE_URL} URL_HASH ${LIBARCHIVE_HASH})

setcb(BUILD_STATIC_LIBS ON)
setcb(BUILD_CURL_EXE OFF)
setcb(BUILD_STATIC_CURL OFF)
setcb(CURL_USE_LIBPSL OFF)
if(WIN32)
	setcb(CURL_USE_SCHANNEL ON)
endif()
FetchContent_Declare(curl URL ${CURL_URL} URL_HASH ${CURL_HASH})

setcb(BUILD_LIBOUT123 OFF)
FetchContent_Declare(mpg123
	URL ${MPG123_URL}
	URL_HASH ${MPG123_HASH}
	SOURCE_SUBDIR ports/cmake)

setcb(ST_NO_EXCEPTION_HANDLING ON)
setcb(SOUNDSTRETCH OFF)
setcb(SOUNDTOUCH_DLL OFF)
setcb(OPENMP OFF)
FetchContent_Declare(soundtouch URL ${SOUNDTOUCH_URL} URL_HASH ${SOUNDTOUCH_HASH})

setcb(SOLOUD_WITH_SDL3 ON)
setcb(SOLOUD_WITH_MPG123 ON)
setcb(SOLOUD_WITH_FFMPEG OFF)
if(MSVC)
	set(PKG_CONFIG_ARGN "--msvc-syntax")
endif()
FetchContent_Declare(soloud URL ${SOLOUD_URL} URL_HASH ${SOLOUD_HASH})

FetchContent_MakeAvailable(sdl3 ft2 png zlib bz2 fmt glm lzma libarchive curl mpg123 soundtouch soloud)

## Binary dependencies
FetchContent_Declare(discord_game_sdk URL ${DISCORDSDK_URL} URL_HASH ${DISCORDSDK_HASH})
FetchContent_Declare(bass URL ${BASS_URL} URL_HASH ${BASS_HASH})
FetchContent_Declare(bassfx URL ${BASSFX_URL} URL_HASH ${BASSFX_HASH})
FetchContent_Declare(bassmix URL ${BASSMIX_URL} URL_HASH ${BASSMIX_HASH})
FetchContent_Declare(bassloud URL ${BASSLOUD_URL} URL_HASH ${BASSLOUD_HASH})
FetchContent_Declare(bassflac URL ${BASSFLAC_URL} URL_HASH ${BASSFLAC_HASH})
FetchContent_MakeAvailable(discord_game_sdk bass bassfx bassmix bassloud bassflac)

if(LINUX)
	FetchContent_Declare(bass-linux URL ${BASS_LINUX_URL})
	FetchContent_Declare(bassfx-linux URL ${BASSFX_LINUX_URL})
	FetchContent_Declare(bassmix-linux URL ${BASSMIX_LINUX_URL})
	FetchContent_Declare(bassloud-linux URL ${BASSLOUD_LINUX_URL})
	FetchContent_Declare(bassflac-linux URL ${BASSFLAC_LINUX_URL})
	FetchContent_MakeAvailable(bass-linux bassfx-linux bassmix-linux bassloud-linux bassflac-linux)
endif()

if(WIN32 OR MSVC)
	FetchContent_Declare(basswasapi URL ${BASSWASAPI_URL} URL_HASH ${BASSWASAPI_HASH})
	FetchContent_Declare(basswasapi_header URL ${BASSWASAPI_HEADER_URL} URL_HASH ${BASSWASAPI_HEADER_HASH})
	FetchContent_Declare(bassasio URL ${BASSASIO_URL} URL_HASH ${BASSASIO_HASH})
	FetchContent_MakeAvailable(basswasapi basswasapi_header bassasio)
endif()
