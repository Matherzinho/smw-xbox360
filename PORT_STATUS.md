# Xbox 360 port status

Community port based on upstream `snesrev/smw` commit `eae20c65c58930c8b62c76188d259579ad4130f1`. The original MIT license and bundled third-party notices, including Opus terms, remain in `LICENSE.txt`. This project is not affiliated with Nintendo or Microsoft.

## Implemented

- Xenon CMake target compiles the game core and emits `smw_xbox360.xex`.
- Frontend initializes the game core, advances frames, submits PPU pixels to the XSDK Xenos renderer and reads the Xbox controller through Xam input.
- Controller mapping: D-pad/left stick directions; Back=Select; Start=Start; Xbox B=SNES A, Xbox A=SNES B, Xbox Y=SNES X, Xbox X=SNES Y; LB=L; RB=R. Back+Start exits.
- Asset loader reads and validates `smw_assets.dat` from the title directory.
- Package preparation script checks required inputs and copies the XEX, assets and README.

## Not yet validated

- XEX generation succeeded with the XSDK toolchain; this does not prove runtime functionality.
- Xenia Canary fails at `xsdk_present_open` during display LUT autofill with status `0xC00000B5`; the XSDK sample reproduces it.
- No physical Xbox 360 test or successful frame presentation is confirmed.
- Audio output is not enabled.
- Optional upstream `other/smb1.zst` and `other/smbll.zst` files are compressed ROM payloads for other games. They are omitted from the sanitized public history and are not used by the SMW Xenon target.

## Legal assets

Provide a legally obtained USA `smw.sfc` locally and run `python3 assets/restool.py --rom smw.sfc --extract-from-rom`. The extractor validates SHA-1 `6B47BB75D16514B6A476AA0C73A683A2A4C18765` after removing a copier header when present. Do not commit or distribute the ROM or `smw_assets.dat`.
