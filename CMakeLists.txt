cmake_minimum_required(VERSION 3.20)
project(neosu VERSION 40.04 LANGUAGES C CXX)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS OFF)

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
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Debug>:_DEBUG>
)

find_package(OpenGL REQUIRED)

# FetchContent_MakeAvailable() should have made all the downloaded dependencies available as "packages",
# provided that there is a CMake project in their root directories.
find_package(ZLIB REQUIRED)
# find_package(libjpeg-turbo REQUIRED)
find_package(PNG REQUIRED)
find_package(Freetype REQUIRED)
find_package(LibLZMA REQUIRED)
find_package(CURL REQUIRED)
# find_package(SoundTouch REQUIRED)
find_package(fmt REQUIRED)
find_package(SDL3 REQUIRED)

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
)

target_link_libraries(neosu PRIVATE
    OpenGL::GL
    OpenGL::GLU
    SDL3::SDL3-static
    ZLIB::ZLIB
    jpeg-static
    PNG::PNG
    Freetype::Freetype
    fmt::fmt
    LibLZMA::LibLZMA
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
target_binary_resources(neosu PRIVATE
    cacert_pem "${CMAKE_BINARY_DIR}/cacert.pem"
    cursortrail_vsh "assets/shaders/cursortrail_v.glsl"
    cursortrail_fsh "assets/shaders/cursortrail_f.glsl"
    slider_vsh "assets/shaders/slider_v.glsl"
    slider_fsh "assets/shaders/slider_f.glsl"
    flashlight_vsh "assets/shaders/flashlight_v.glsl"
    flashlight_fsh "assets/shaders/flashlight_f.glsl"
    actual_flashlight_vsh "assets/shaders/actual_flashlight_v.glsl"
    actual_flashlight_fsh "assets/shaders/actual_flashlight_f.glsl"
    smoothclip_vsh "assets/shaders/smoothclip_v.glsl"
    smoothclip_fsh "assets/shaders/smoothclip_f.glsl"
)
