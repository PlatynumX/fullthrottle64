#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

ZIP="${1:-$HOME/storage/downloads/fullthrottle64-scummvm-r3.zip}"
REPO="${2:-$HOME/fullthrottle64}"

test -f "$ZIP" || { echo "Missing $ZIP"; exit 1; }
test -d "$REPO/.git" || { echo "Missing existing repo at $REPO"; exit 1; }

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
unzip -q "$ZIP" -d "$TMP"

cd "$REPO"
cp -a "$TMP"/. "$REPO"/
git add .
git commit -m "Fix demo fetch and add full prep diagnostics (R3)" || true
git push origin main

echo
echo "R3 pushed. GitHub Actions should be running now."
