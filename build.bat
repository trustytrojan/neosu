@echo off
setlocal enabledelayedexpansion

set CXX=g++
set LD=g++

rem Install ccache for faster builds
where ccache >nul 2>nul
if %errorlevel% equ 0 (
	set CXX=ccache %CXX%
)

set CXXFLAGS=-std=c++17 -Wall -fmessage-length=0 -Wno-sign-compare -Wno-unused-local-typedefs -Wno-reorder -Wno-switch -IC:\mingw32\include
set CXXFLAGS=%CXXFLAGS% -D__GXX_EXPERIMENTAL_CXX0X__
set CXXFLAGS=%CXXFLAGS% -Isrc/App -Isrc/App/Osu -Isrc/Engine -Isrc/GUI -Isrc/GUI/Windows -Isrc/GUI/Windows/VinylScratcher -Isrc/Engine/Input -Isrc/Engine/Platform -Isrc/Engine/Main -Isrc/Engine/Renderer -Isrc/Util
set LDFLAGS=-logg -lADLMIDI -lmad -lmodplug -lsmpeg -lgme -lvorbis -lopus -lvorbisfile -ldiscord-rpc -lSDL2_mixer_ext.dll -lSDL2 -ld3dcompiler_47 -ld3d11 -ldxgi -lcurl -llibxinput9_1_0 -lfreetype -lopengl32 -lOpenCL -lvulkan-1 -lglew32 -lglu32 -lgdi32 -lbass -lbassasio -lbass_fx -lbassmix -lbasswasapi -lcomctl32 -lDwmapi -lComdlg32 -lpsapi -lws2_32 -lwinmm -lpthread -llibjpeg -lwbemuuid -lole32 -loleaut32 -llzma

set CXXFLAGS=%CXXFLAGS% -g3
rem set CXXFLAGS=%CXXFLAGS% -O3
rem set LDFLAGS=%LDFLAGS% -mwindows -s

rem PREPARE BUILD DIR
xcopy resources build /E /I /Y > nul

for /d %%i in (libraries\*) do (
	copy %%i\bin\* build\ > nul
)

rem COLLECT LIBRARIES INCLUDE PATHS
set INCLUDEPATHS=
for /d %%i in (libraries\*) do (
	if exist "%%i\include" (
		set VAR=%%i\include
		set INCLUDEPATHS=!INCLUDEPATHS! -I!VAR:\=/!
	)
)
set CXXFLAGS=%CXXFLAGS% %INCLUDEPATHS%

rem COLLECT LIBRARIES LINKER PATHS
set LIBPATHS=
for /d %%i in (libraries\*) do (
	if exist "%%i\lib\windows" (
		set VAR=%%i\lib\windows
		set LIBPATHS=!LIBPATHS! -L!VAR:\=/!
	)
)
set CXXFLAGS=%CXXFLAGS% %LIBPATHS%

rem BUILD OBJECTS
if exist "build_flags.txt" del /q "build_flags.txt"
<nul set /p "=-o build/McOsu.exe " >> build_flags.txt
<nul set /p "=!CXXFLAGS! " >> build_flags.txt
set OBJECTS=
for /r "src" %%i in (*.cpp) do (
	set file_path=%%i
	set obj_path=!file_path:src\=obj\!

	for %%F in ("!obj_path!") do set "obj_dir=%%~dpF"
	if not exist !obj_dir! mkdir !obj_dir!

	set obj=!obj_path:.cpp=.o!
	set VAR=!OBJECTS! !obj!
	set OBJECTS=!VAR!

	echo CC %%i
	<nul set /p "=!obj:\=/! " >> build_flags.txt
	%CXX% %CXXFLAGS% -c %%i -o !obj!
	if !ERRORLEVEL! neq 0 (
		pause
		goto END
	)
)

rem BUILD EXECUTABLE
<nul set /p "=!LDFLAGS! " >> build_flags.txt
%LD% @build_flags.txt
if !ERRORLEVEL! neq 0 (
	pause
) else (
	del /q "build_flags.txt"
	cd build
	McOsu.exe
)

:END
