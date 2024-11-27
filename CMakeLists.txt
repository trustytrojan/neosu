include(FetchContent)
cmake_minimum_required(VERSION 3.16)
project(neosu LANGUAGES CXX)

if(LINUX)
    function(find_or_fetch_bass lib path)
        message(STATUS "Checking for bass${lib}")
        find_library(bass${lib} bass${lib} PATHS /lib /usr/lib)
        if(NOT bass${lib})
            message(STATUS "bass${lib} not found... fetching it now")
            if(NOT path)
                set(path /files/bass${lib}24-linux.zip)
            endif()
            FetchContent_Declare(
                bass${lib}
                URL https://www.un4seen.com${path}
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            )
            FetchContent_MakeAvailable(bass${lib})
            set(LDIR ${bass${lib}_SOURCE_DIR}/libs/${CMAKE_SYSTEM_PROCESSOR})
            message(STATUS "Adding link directory: ${LDIR}")
            link_directories(${LDIR})
        else()
            message(STATUS "bass${lib} found!")
        endif()
    endfunction()

    find_or_fetch_bass("" "")
    find_or_fetch_bass("_fx" /files/z/0/bass_fx24-linux.zip)
    find_or_fetch_bass("mix" "")
    find_or_fetch_bass("loud" "")

    include(FindPkgConfig)
    pkg_check_modules(PKGCONF REQUIRED blkid freetype2 glew libenet libjpeg liblzma xi zlib)
    link_libraries(${PKGCONF_STATIC_LIBRARIES})
    include_directories(${PKGCONF_INCLUDE_DIRS})
elseif(WIN32)
    message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")

    # On 64-bit Windows, CMake gives us the string "AMD64"
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
        set(ARCH_STR "x64")
    else()
        set(ARCH_STR "Win32")
    endif()

    message(STATUS "ARCH_STR: ${ARCH_STR}")

    file(GLOB LIBS libraries/*) # this gets absolute paths
    message(STATUS "Pre-packaged libraries: ${LIBS}")

    foreach(lib IN LISTS LIBS)
        message(VERBOSE "Adding include directory: ${lib}/include")
        include_directories(${lib}/include)
        message(VERBOSE "Adding link directory: ${lib}/${ARCH_STR}")
        link_directories(${lib}/${ARCH_STR})
    endforeach()

    # unfortunately the windows sdk is needed for rtscom.h (stylus support)
    # so i'm going to hardcode a path to my windows sdk for now
    file(GLOB WSDK_INCLUDE_DIRS "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/*")
    foreach(dir IN LISTS WSDK_INCLUDE_DIRS)
        message(STATUS "Adding include directory: ${dir}")
        include_directories(${dir})
    endforeach()

    # stuck with "vcruntime.h not found", i give up for now, just install visual studio
endif()

link_libraries(
    bass
    bassmix
    bassloud
    bass_fx
    curl
)

include_directories(
    src/App
    src/App/Osu
    src/Engine
    src/GUI
    src/GUI/Windows
    src/GUI/Windows/VinylScratcher
    src/Engine/Input
    src/Engine/Platform
    src/Engine/Main
    src/Engine/Renderer
    src/Util
    libraries/bass/include
    libraries/bassasio/include
    libraries/bassmix/include
    libraries/bassfx/include
    libraries/bassloud/include
    libraries/basswasapi/include
    libraries/discord-sdk/include
)

# may want to limit this to GNU compilers only
add_compile_options(
    -std=c++2a
    -fmessage-length=0
    # -fno-exceptions
    -Wno-sign-compare
    -Wno-unused-local-typedefs
    -Wno-reorder
    -Wno-switch
    -Wall
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(neosu ${SOURCES})
