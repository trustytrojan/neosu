# neosu

![Linux build](https://github.com/kiwec/neosu/actions/workflows/linux-multiarch.yml/badge.svg) ![Windows build](https://github.com/kiwec/neosu/actions/workflows/win-multiarch.yml/badge.svg) [![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/kiwec/neosu)

This is a third-party fork of McKay's [McOsu](https://store.steampowered.com/app/607260/McOsu/).

If you need help, contact `kiwec` or `spec.ta.tor` on Discord, either by direct message or on [the neosu server](https://discord.com/invite/YWPBFSpH8v).

### Building

- (MSVC) On Windows, for Windows -> run `buildwin64.bat` in `cmake-win`
- (gcc) On Linux, for Linux -> run `../configure` in `build`, then `make install`
- (gcc-mingw) On Linux/WSL, for Windows -> run ` ../configure --host=x86_64-w64-mingw32` in `build`, then `make install`

Releases are made using gcc (for Linux) and gcc-mingw (for Windows).
