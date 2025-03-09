# author: trustytrojan

include(FetchContent)
cmake_minimum_required(VERSION 3.16)
project(neosu LANGUAGES CXX)

if(LINUX)
	# bass libs
    function(find_or_fetch_bass lib path idir)
        find_library(bass${lib} bass${lib} PATHS /lib /usr/lib)
        if(NOT bass${lib})
            message(STATUS "bass${lib} not found... fetching it now")

			if(NOT path)
				message(FATAL_ERROR "path on un4seen server not provided")
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

			if(idir)
				set(FULL_IDIR ${bass${lib}_SOURCE_DIR}${idir})
			else()
				set(FULL_IDIR ${bass${lib}_SOURCE_DIR})
			endif()

			message(STATUS "Adding include directory: ${FULL_IDIR}")
			include_directories(${FULL_IDIR})
        else()
            message(STATUS "bass${lib} found!")
        endif()
    endfunction()

    find_or_fetch_bass("" /files/bass24-linux.zip "")
    find_or_fetch_bass("_fx" /files/z/0/bass_fx24-linux.zip /C)
    find_or_fetch_bass("mix" /files/bassmix24-linux.zip "")
    find_or_fetch_bass("loud" /files/bassloud24-linux.zip "")

	# for some stupid reason un4seen doesnt give bassloud.h in the linux zip
	find_or_fetch_bass("loudosx" /files/bassloud24-osx.zip "")

	# discord sdk
	find_library(discord_game_sdk discord_game_sdk PATHS /lib /usr/lib)
	if(NOT discord_game_sdk)
		FetchContent_Declare(
			discord_game_sdk
			URL https://dl-game-sdk.discordapp.net/3.2.1/discord_game_sdk.zip
			DOWNLOAD_EXTRACT_TIMESTAMP TRUE
		)
		FetchContent_MakeAvailable(discord_game_sdk)

		set(LDIR ${discord_game_sdk_SOURCE_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR})
		message(STATUS "Adding link directory: ${LDIR}")
		link_directories(${LDIR})

		execute_process(
			COMMAND ln -sf discord_game_sdk.so libdiscord_game_sdk.so
			WORKING_DIRECTORY ${discord_game_sdk_SOURCE_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}
			COMMAND_ERROR_IS_FATAL ANY
		)

		set(IDIR ${discord_game_sdk_SOURCE_DIR}/c)
		message(STATUS "Adding include directory: ${IDIR}")
		include_directories(${IDIR})
	endif()

	# everything from the makefile's pkgconf call
    include(FindPkgConfig)
    pkg_check_modules(PKGCONF REQUIRED blkid freetype2 glew libjpeg liblzma xi zlib)
    link_libraries(${PKGCONF_STATIC_LIBRARIES})
    include_directories(${PKGCONF_INCLUDE_DIRS})

	# last thing to make it run properly: copy resources to binary dir
	message(STATUS "Fetching old neosu repo for resources...")
	FetchContent_Declare(
		old-repo
		URL https://git.kiwec.net/kiwec/McOsu/archive/master.zip
		DOWNLOAD_EXTRACT_TIMESTAMP TRUE
	)
	FetchContent_MakeAvailable(old-repo)
	execute_process(
		COMMAND sh -c "cp -nr ${old-repo_SOURCE_DIR}/resources/* ${CMAKE_BINARY_DIR}"
		COMMAND_ERROR_IS_FATAL ANY
	)
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
	discord_game_sdk
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
