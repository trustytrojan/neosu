# neosu

This is a third-party fork of [McOsu](https://store.steampowered.com/app/607260/McOsu/), unsupported by McKay.

If you need help, contact `kiwec` on Discord, either by direct message or via [his server](https://discord.com/invite/YWPBFSpH8v).

### Building (windows)

Open `neosu.vcxproj` with Visual Studio then press F5.

### Building (linux)

Required dependencies:

- [bass](https://www.un4seen.com/download.php?bass24-linux)
- [bassmix](https://www.un4seen.com/download.php?bassmix24-linux)
- [bass_fx](https://www.un4seen.com/download.php?z/0/bass_fx24-linux)
- [BASSloud](https://www.un4seen.com/download.php?bassloud24-linux)
- blkid
- freetype2
- glew
- libenet
- libjpeg
- liblzma
- xi
- zlib

Once those are installed, just run `make -j8`.
