#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$ROOT/work"
SCUMM="$WORK/scummvm"
GAME="$ROOT/filesystem/game"

rm -rf "$WORK"
rm -rf "$ROOT/filesystem"
mkdir -p "$WORK" "$GAME"

echo "[FT64 PREP] === FullThrottle64 ScummVM preparation ==="
"$ROOT/scripts/fetch_demo.sh" "$GAME"

echo "[FT64 PREP] Fetching ScummVM v1.6.0..."
git clone --depth 1 --branch v1.6.0 \
  https://github.com/scummvm/scummvm.git "$SCUMM"

cd "$SCUMM"
echo "[FT64 PREP] ScummVM commit: $(git rev-parse HEAD)"
echo "[FT64 PREP] ScummVM description: $(git describe --tags --always)"
cd "$ROOT"

echo "[FT64 PREP] Installing libdragon N64 backend overlay..."
rm -rf "$SCUMM/backends/platform/n64"
mkdir -p "$SCUMM/backends/platform/n64"
cp -a "$ROOT/overlay/backends/platform/n64/." "$SCUMM/backends/platform/n64/"

cat > "$SCUMM/config.mk" <<'EOF'
ENABLE_SCUMM = STATIC_PLUGIN
USE_RGB_COLOR = 0
USE_LIBMAD = 0
USE_LIBOGG = 0
USE_VORBIS = 0
USE_FLAC = 0
USE_PNG = 0
USE_FREETYPE2 = 0
EOF

echo "[FT64 PREP] ScummVM source prepared successfully."
