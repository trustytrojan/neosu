# neosu

![Linux build](https://github.com/kiwec/neosu/actions/workflows/linux-multiarch.yml/badge.svg) ![Windows build](https://github.com/kiwec/neosu/actions/workflows/win-multiarch.yml/badge.svg) [![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/kiwec/neosu)

This is a third-party fork of McKay's [McOsu](https://store.steampowered.com/app/607260/McOsu/).

If you need help, contact `kiwec` or `spec.ta.tor` on Discord, either by direct message or on [the neosu server](https://discord.com/invite/YWPBFSpH8v).

### Building

The recommended way to build (and the way releases are made) is using gcc/gcc-mingw.

- On Linux, for Linux -> run `../configure` in `build`, then `make install`
- On Linux/WSL, for Windows -> run ` ../configure --host=x86_64-w64-mingw32` in `build`, then `make install`

If your `aclocal` version is older than 1.18, you might have to run `autoreconf -fiv` first.

For debugging convenience, you can also do an MSVC build on Windows, by running `buildwin64.bat` in `cmake-win`.
