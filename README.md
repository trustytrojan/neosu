# Multiplayer McOsu

This is a third-party fork of [McOsu](https://store.steampowered.com/app/607260/McOsu/), unsupported by McKay.

If you need help, contact `kiwec` on Discord, either by direct message or via [his server](https://discord.com/invite/YWPBFSpH8v).

### Building (windows)

Required dependencies:

- [MinGW32](https://sourceforge.net/projects/mingw-w64) (extract [this](https://netcologne.dl.sourceforge.net/project/mingw-w64/Toolchains targetting Win32/Personal Builds/mingw-builds/8.1.0/threads-win32/dwarf/i686-8.1.0-release-win32-dwarf-rt_v6-rev0.7z) in `C:\mingw32`, then add `C:\mingw32\bin` to PATH)

Once those are installed, just run `build.bat`.

### Building (linux)

Required dependencies:

- [bass](https://www.un4seen.com/download.php?bass24-linux)
- [bassmix](https://www.un4seen.com/download.php?bassmix24-linux)
- [bass_fx](https://www.un4seen.com/download.php?z/0/bass_fx24-linux)
- blkid
- freetype2
- glew
- libenet
- libjpeg
- liblzma
- OpenCL
- xi
- zlib

Once those are installed, just run `make -j8`.
