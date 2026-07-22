#!/usr/bin/env bash
set -euo pipefail

OUT="${1:-filesystem/game}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

mkdir -p "$OUT"

echo "[FT64 PREP] Downloading official Full Throttle DOS demo..."
curl -L --fail --retry 4 --retry-delay 2 \
  https://downloads.scummvm.org/frs/demos/scumm/ft-dos-demo-en.zip \
  -o "$TMP/ft.zip"

echo "[FT64 PREP] Demo archive bytes: $(wc -c < "$TMP/ft.zip")"

mkdir -p "$TMP/u"
unzip -q "$TMP/ft.zip" -d "$TMP/u"
cp -a "$TMP/u"/. "$OUT"/

echo "[FT64 PREP] Extracted files:"
find "$OUT" -type f -printf '%P  %s bytes\n' | sort

# Normalize the three core files into OUT's root. Avoid cp(1)'s "same file"
# failure if the official archive already placed them there.
for f in FT.000 FT.001 MONSTER.SOU; do
    p="$(find "$OUT" -type f -iname "$f" -print -quit)"
    if [[ -z "$p" ]]; then
        echo "[FT64 PREP] ERROR: required demo file missing: $f" >&2
        exit 2
    fi

    src="$(readlink -f "$p")"
    dst="$(readlink -m "$OUT/$f")"

    if [[ "$src" != "$dst" ]]; then
        echo "[FT64 PREP] Normalizing $p -> $OUT/$f"
        cp "$p" "$OUT/$f"
    else
        echo "[FT64 PREP] $f already in normalized location"
    fi

    test -s "$OUT/$f"
done

echo "[FT64 PREP] Demo validation passed."
