#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
XEX="$ROOT/xbox360/build/smw_xbox360.xex"
ASSETS="$ROOT/smw_assets.dat"
OUT="$ROOT/xbox360/build/usb-package"

if [ ! -s "$XEX" ]; then
  echo "Missing or empty XEX: $XEX" >&2
  exit 1
fi
if [ ! -s "$ASSETS" ]; then
  echo "Missing or empty assets: $ASSETS" >&2
  echo "Provide an authorized USA smw.sfc and run assets/restool.py first." >&2
  exit 2
fi
mkdir -p "$OUT"
cp "$XEX" "$OUT/smw_xbox360.xex"
cp "$ASSETS" "$OUT/smw_assets.dat"
printf 'Prepared package: %s\n' "$OUT"
wc -c "$OUT/smw_xbox360.xex" "$OUT/smw_assets.dat"
