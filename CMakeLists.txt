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

include(deps.cmake)

file(GLOB_RECURSE SOURCES
    "${CMAKE_SOURCE_DIR}/../../src/*.cpp"
    "${CMAKE_SOURCE_DIR}/../../src/*.c"
    "${CMAKE_SOURCE_DIR}/../../libraries/*.cpp"
    "${CMAKE_SOURCE_DIR}/../../libraries/*.c"
)

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
find_package(libjpeg-turbo REQUIRED)
find_package(PNG REQUIRED)
find_package(Freetype REQUIRED)
find_package(LibLZMA REQUIRED)
find_package(CURL REQUIRED)
find_package(SoundTouch REQUIRED)
# find_package(SoLoud REQUIRED)

find_package(fmt REQUIRED)
find_package(SDL3 REQUIRED)
# find_package(glm REQUIRED)

# MPG123 is an exception and has its CMake project in "./ports/cmake"
add_subdirectory(${mpg123_SOURCE_DIR}/ports/cmake)

target_link_libraries(neosu PRIVATE
    OpenGL::GL
    OpenGL::GLU
    SDL3::SDL3-static
    ZLIB::ZLIB
    turbojpeg-static
    PNG::PNG
    Freetype::Freetype
    fmt::fmt
    LibLZMA::LibLZMA
    CURL::libcurl
    SoundTouch::SoundTouch
    soloud
    libmpg123
    bz2_static
    archive_static
)
