#!/usr/bin/env bash
set -euo pipefail
OUT="${1:-filesystem/game}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$OUT"
curl -L --fail --retry 4 \
  https://downloads.scummvm.org/frs/demos/scumm/ft-dos-demo-en.zip \
  -o "$TMP/ft.zip"
unzip -q "$TMP/ft.zip" -d "$TMP/u"
cp -a "$TMP/u"/. "$OUT"/

# Flatten the three core resource files to the root expected by -p rom:/game.
for f in FT.000 FT.001 MONSTER.SOU; do
  p="$(find "$OUT" -type f -iname "$f" -print -quit)"
  test -n "$p"
  cp "$p" "$OUT/$f"
done
