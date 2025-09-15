# neosu

![Linux build](https://github.com/kiwec/neosu/actions/workflows/linux-multiarch.yml/badge.svg) ![Windows build](https://github.com/kiwec/neosu/actions/workflows/win-multiarch.yml/badge.svg) [![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/kiwec/neosu)

This is a third-party fork of McKay's [McOsu](https://store.steampowered.com/app/607260/McOsu/).

If you need help, contact `kiwec` or `spec.ta.tor` on Discord, either by direct message or on [the neosu server](https://discord.com/invite/YWPBFSpH8v).

### Building

The recommended way to build (and the way releases are made) is using gcc/gcc-mingw.

- For all *nix systems, run `./autogen.sh` in the top-level folder (once) to generate the build files.
- Create and enter a build subdirectory; e.g. `mkdir build && cd build`
- On Linux, for Linux -> run `../configure`, then `make install`
  - This will build and install everything under `./dist/bin-$arch`, configurable with the `--prefix` option to `configure`
- On Linux/WSL, for Windows -> run ` ../configure --host=x86_64-w64-mingw32`, then `make install`

For debugging convenience, you can also do an MSVC build on Windows, by running `buildwin64.bat` in `cmake-win`.
