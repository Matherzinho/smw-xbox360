# Super Mario World for Xbox 360 (Xenon)

This is a separate XSDK frontend for the upstream `snesrev/smw` reimplementation. The original desktop SDL2/OpenGL frontend and Makefile remain intact. The Xenon target uses XSDK GPU/presentation APIs and Xam controller input; audio output is not enabled yet.

## Build

```sh
cmake -S xbox360 -B xbox360/build -G Ninja -DCMAKE_TOOLCHAIN_FILE=/home/Matheus/xsdk/cmake/xsdk.cmake
cmake --build xbox360/build --target smw_xbox360
```

The title is `xbox360/build/smw_xbox360.xex`.

## Game assets

Provide a legally obtained, unheadered USA Super Mario World ROM named `smw.sfc` in the repository root. The extractor validates SHA-1 `6B47BB75D16514B6A476AA0C73A683A2A4C18765` and strips a copier header when present. Generate resources with:

```sh
python3 assets/restool.py --rom /home/Matheus/smw-xbox360/smw.sfc --extract-from-rom
```

This creates `smw_assets.dat`. Copy `smw_xbox360.xex` and `smw_assets.dat` side by side in one title directory on the Xbox 360. Relative paths resolve from the XSDK title filesystem (`game:\\`).

Xenia Canary in this environment fails XSDK display LUT initialization, reproduced with the XSDK template. Test presentation on a modified Xbox 360; this port has not yet been tested on hardware.

## Xbox controller

- D-pad or left stick: directions
- Back: Select
- Start: Start
- B: SNES A
- A: SNES B
- Y: SNES X
- X: SNES Y
- Left / Right shoulder: L / R
- Back + Start: exit frontend

The face-button mapping follows upstream `smw.ini`. Audio output and desktop-only options are not wired to Xenon yet.
