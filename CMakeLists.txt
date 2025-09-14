cmake_minimum_required(VERSION 3.20)
project(neosu VERSION 40.04 LANGUAGES C CXX)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS OFF)

set(CMAKE_C_COMPILER_LAUNCHER ccache)
set(CMAKE_CXX_COMPILER_LAUNCHER ccache)

include(deps.cmake)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.c libraries/*.cpp libraries/*.c)
add_executable(neosu ${SOURCES})

target_compile_definitions(neosu PRIVATE
    GLM_ENABLE_EXPERIMENTAL
    FMT_USE_EXCEPTIONS=0
    _HAS_EXCEPTIONS=0
    ST_NO_EXCEPTION_HANDLING=1
    SOLOUD_NO_ASSERTS=1
    _UNICODE
    UNICODE=1
    SHADERS_INCDIR="${CMAKE_SOURCE_DIR}/assets/shaders/"
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Debug>:_DEBUG>
)

find_package(OpenGL REQUIRED)

if(WIN32)
    # setcb defined in deps.cmake
    setcb(MCENGINE_PLATFORM_WINDOWS ON)
    setcb(MCENGINE_FEATURE_OPENGL ON)
    setcb(MCENGINE_FEATURE_BASS ON)
    setcb(MCENGINE_FEATURE_SOLOUD ON)
    configure_file("cmake-win/config.h.in" "${CMAKE_BINARY_DIR}/config.h" @ONLY)
endif()

target_include_directories(neosu PRIVATE
    src
    src/App
    src/App/Osu
    src/App/Osu/SongBrowser
    src/Binary
    src/Engine
    src/Engine/Input
    src/Engine/Renderer
    src/Engine/Renderer/OpenGL
    src/Engine/Resources
    src/Engine/Sound
    src/Engine/Sound/BASS
    src/Engine/Sound/SoLoud
    src/GUI
    src/GUI/Windows
    src/Platform
    src/Util
    libraries
    libraries/delegatesv2
    libraries/glad
    libraries/glad/KHR
    ${bass_SOURCE_DIR}
    ${bassflac_SOURCE_DIR}/c
    ${bassfx_SOURCE_DIR}
    ${bassloud_SOURCE_DIR}/c
    ${bassmix_SOURCE_DIR}
    ${discord_game_sdk_SOURCE_DIR}/c
    ${CMAKE_BINARY_DIR}
    ${lzma_SOURCE_DIR}/src/liblzma/api
    ${bassasio_SOURCE_DIR}
    ${basswasapi_header_SOURCE_DIR}/c
    ${CMAKE_BINARY_DIR}/jpeg-prefix/src/jpeg/src
)

target_link_directories(neosu PRIVATE ${CMAKE_BINARY_DIR}/jpeg-prefix/src/jpeg-build)
target_link_libraries(neosu PRIVATE
    OpenGL::GL
    OpenGL::GLU
    SDL3::SDL3-static
    turbojpeg
    png_static
    freetype
    fmt::fmt
    liblzma
    CURL::libcurl
    glm::glm
    SoundTouch
    soloud
    libmpg123
    bz2_static
    archive_static
)

include(cmake-win/src/binclude.cmake)
enable_language(ASM_NASM)
if(NOT EXISTS "${CMAKE_BINARY_DIR}/cacert.pem")
    file(DOWNLOAD https://curl.se/ca/cacert.pem cacert.pem)
endif()
target_binary_resources(neosu PRIVATE cacert_pem "${CMAKE_BINARY_DIR}/cacert.pem")

if(LINUX)
    if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        message(FATAL_ERROR "do extra work here for other architectures")
    endif()
    make_directory(${CMAKE_BINARY_DIR}/lib)
    file(CREATE_LINK ${bass-linux_SOURCE_DIR}/x86_64/libbass.so ${CMAKE_BINARY_DIR}/lib/libbass.so SYMBOLIC)
    file(CREATE_LINK ${bassfx-linux_SOURCE_DIR}/x86_64/libbass_fx.so ${CMAKE_BINARY_DIR}/lib/libbass_fx.so SYMBOLIC)
    file(CREATE_LINK ${bassmix-linux_SOURCE_DIR}/x86_64/libbassmix.so ${CMAKE_BINARY_DIR}/lib/libbassmix.so SYMBOLIC)
    file(CREATE_LINK ${bassloud-linux_SOURCE_DIR}/libs/x86_64/libbassloud.so ${CMAKE_BINARY_DIR}/lib/libbassloud.so SYMBOLIC)
    file(CREATE_LINK ${bassflac-linux_SOURCE_DIR}/libs/x86_64/libbassflac.so ${CMAKE_BINARY_DIR}/lib/libbassflac.so SYMBOLIC)
endif()

if(NOT WIN32)
    # on windows, just do this manually with admin command prompt
    file(CREATE_LINK ${CMAKE_SOURCE_DIR}/assets/fonts ${CMAKE_BINARY_DIR}/fonts SYMBOLIC)
    file(CREATE_LINK ${CMAKE_SOURCE_DIR}/assets/materials ${CMAKE_BINARY_DIR}/materials SYMBOLIC)
endif()
