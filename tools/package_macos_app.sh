#!/bin/sh
# Package build-macos/biko3 as Biko3.app.
#
# The game data is NOT bundled: it stays outside the app in
#   ~/Library/Application Support/Biko3/game
# (or BIKO3_GAME). This keeps the bundle small and avoids App Translocation
# making the writable Data/ folder read-only.
#
# Usage:
#   tools/package_macos_app.sh [--build DIR] [--app PATH] [--icon PNG]
set -eu
cd "$(dirname "$0")/.."

BUILD_DIR="build-macos"
APP="dist/Biko3.app"
ICON=""
while [ $# -gt 0 ]; do
  case "$1" in
    --build) BUILD_DIR="$2"; shift 2 ;;
    --app)   APP="$2"; shift 2 ;;
    --icon)  ICON="$2"; shift 2 ;;
    -h|--help)
      sed -n '2,14p' "$0" | sed 's/^# \{0,1\}//'
      exit 0 ;;
    *) echo "unknown option: $1" >&2; exit 2 ;;
  esac
done

case "$APP" in
  /|.|..|"") echo "refusing to package into: $APP" >&2; exit 2 ;;
esac

BIN="$BUILD_DIR/biko3"
if [ ! -x "$BIN" ]; then
  echo "missing $BIN; run ./build-macos.sh first" >&2
  exit 1
fi

VERSION="$(python3 - <<'PY'
import json
print(json.load(open('config/dependencies.lock.json'))['project_version'])
PY
)"

rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"

# ---------------------------------------------------------------------------
# Info.plist
# ---------------------------------------------------------------------------
PLIST="$APP/Contents/Info.plist"
cat > "$PLIST" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key><string>ja</string>
    <key>CFBundleDisplayName</key><string>尾行3</string>
    <key>CFBundleName</key><string>Biko3</string>
    <key>CFBundleIdentifier</key><string>local.biko3.runtime</string>
    <key>CFBundleExecutable</key><string>biko3</string>
    <key>CFBundlePackageType</key><string>APPL</string>
    <key>CFBundleShortVersionString</key><string>${VERSION}</string>
    <key>CFBundleVersion</key><string>${VERSION}</string>
    <key>LSMinimumSystemVersion</key><string>12.0</string>
    <key>NSHighResolutionCapable</key><true/>
    <key>NSSupportsAutomaticGraphicsSwitching</key><true/>
    <key>LSApplicationCategoryType</key><string>public.app-category.action-games</string>
</dict>
</plist>
EOF

cp "$BIN" "$APP/Contents/MacOS/biko3"
chmod 755 "$APP/Contents/MacOS/biko3"

# Third-party attribution / notices travel with the app.
if [ -f THIRD_PARTY.md ]; then
  cp THIRD_PARTY.md "$APP/Contents/Resources/THIRD_PARTY.md"
fi
if [ -d licenses ]; then
  cp -R licenses "$APP/Contents/Resources/licenses"
fi

# ---------------------------------------------------------------------------
# Icon (optional): PNG -> .iconset -> .icns via sips/iconutil.
# ---------------------------------------------------------------------------
if [ -n "$ICON" ]; then
  if [ ! -f "$ICON" ]; then
    echo "icon not found: $ICON" >&2
    exit 1
  fi
  if ! command -v sips >/dev/null 2>&1 || ! command -v iconutil >/dev/null 2>&1; then
    echo "skipping icon: sips/iconutil are not available" >&2
  else
    ICONSET="$(mktemp -d)/icon.iconset"
    mkdir -p "$ICONSET"
    for size in 16 32 64 128 256 512; do
      sips -z "$size" "$size" "$ICON" --out "$ICONSET/icon_${size}x${size}.png" >/dev/null
      double=$((size * 2))
      sips -z "$double" "$double" "$ICON" --out "$ICONSET/icon_${size}x${size}@2x.png" >/dev/null
    done
    iconutil -c icns "$ICONSET" -o "$APP/Contents/Resources/AppIcon.icns"
    rm -rf "$(dirname "$ICONSET")"
    /usr/libexec/PlistBuddy -c "Add :CFBundleIconFile string AppIcon" "$PLIST" 2>/dev/null || \
      /usr/libexec/PlistBuddy -c "Set :CFBundleIconFile AppIcon" "$PLIST"
  fi
fi

# Ad-hoc signing is enough for local use. Distribution needs a Developer ID
# certificate and notarization.
if command -v codesign >/dev/null 2>&1; then
  codesign --force --deep --sign - "$APP" >/dev/null 2>&1 || \
    echo "warning: ad-hoc codesign failed; the app may still run locally" >&2
fi

cat <<EOF

Built app: $APP

Put the game files here (the folder that contains Data/):
  \$HOME/Library/Application Support/Biko3/game

Or set BIKO3_GAME when launching from a terminal:
  BIKO3_GAME="/path/to/MAINDIR" "$APP/Contents/MacOS/biko3"

Optional: patch.pp (Chinese/uncensor) goes in the same game folder.
EOF
