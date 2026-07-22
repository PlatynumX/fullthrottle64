#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$ROOT/work"
SCUMM="$WORK/scummvm"
GAME="$ROOT/filesystem/game"

rm -rf "$WORK"
mkdir -p "$WORK" "$GAME"

"$ROOT/scripts/fetch_demo.sh" "$GAME"

echo "==> Fetching ScummVM 1.6.0 (last official N64-era source)"
git clone --depth 1 --branch v1.6.0 https://github.com/scummvm/scummvm.git "$SCUMM"

echo "==> Installing libdragon N64 backend overlay"
rm -rf "$SCUMM/backends/platform/n64"
mkdir -p "$SCUMM/backends/platform/n64"
cp -a "$ROOT/overlay/backends/platform/n64/." "$SCUMM/backends/platform/n64/"

# The port uses libdragon's POSIX-compatible ROM filesystem hooks after dfs_init.
# Force only the SCUMM engine and remove optional codecs/engines to keep the link
# small enough to expose real FT engine problems before generic memory pressure.
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

echo "==> ScummVM source prepared at $SCUMM"
