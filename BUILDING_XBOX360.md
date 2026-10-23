# Building the Xbox 360 Xenon target

The frontend in `xbox360/` builds the upstream game core with XSDK GPU/presentation and Xam controller input. It remains separate from the original SDL2/OpenGL desktop frontend.

## Requirements

- Debian under WSL2.
- XSDK at `/home/Matheus/xsdk`.
- PowerPC Xenon compiler at `/home/Matheus/xsdk-gcc/bin/powerpc-xsdk-elf-gcc`.
- CMake toolchain at `/home/Matheus/xsdk/cmake/xsdk.cmake`.
- CMake and Ninja.

The SDK and compiler are external dependencies; do not copy them into this repository.

## Configure and build

From the repository root in Debian/WSL:

```sh
cmake -S xbox360 -B xbox360/build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/home/Matheus/xsdk/cmake/xsdk.cmake
cmake --build xbox360/build --target smw_xbox360
```

The target writes `xbox360/build/smw_xbox360.xex`. The upstream desktop Makefile remains available.

## Game assets

Provide your own legally obtained USA Super Mario World ROM as `smw.sfc` in the repository root. The extractor validates the headerless ROM SHA-1 `6B47BB75D16514B6A476AA0C73A683A2A4C18765`. The upstream loader removes a 512-byte copier header when the file length modulo 1 MiB is 512 bytes.

Generate assets using the upstream extractor:

```sh
python3 assets/restool.py --rom smw.sfc --extract-from-rom
```

This creates `smw_assets.dat`. Run `sh xbox360/prepare-usb-package.sh` to copy the XEX, assets and README into `xbox360/build/usb-package/`. Keep the ROM and extracted assets out of Git.

## Runtime note

Xenia Canary in the development environment fails during `xsdk_present_open` display LUT autofill with status `0xC00000B5`; this was reproduced with an XSDK sample. Presentation still needs testing on a physical Xbox 360.
