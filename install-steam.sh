#!/bin/sh
# One-click Steam integration for the native Linux build.
#
#   ./install-steam.sh
#
# It installs/updates:
#   ~/.local/share/biko3/biko3          runtime (built from source if needed)
#   ~/.local/bin/biko3                  launcher with gamepad setup
#   ~/.local/share/applications/biko3.desktop
#   <Steam userdata>/config/shortcuts.vdf  "尾行3" entry + artwork
#
# Game data is not bundled: put the original Data/ and patch.pp in
# ~/.local/share/biko3/game (or set BIKO3_GAME when launching).
set -eu
cd "$(dirname "$0")"

PREFIX="${BIKO3_PREFIX:-$HOME/.local/share/biko3}"
BIN="$PREFIX/biko3"
LAUNCHER="$HOME/.local/bin/biko3"
ICON="$HOME/.local/share/icons/biko3.png"
DATA="${BIKO3_GAME:-$PREFIX/game}"
NAME="${BIKO3_STEAM_NAME:-尾行3}"
JOBS="${BK_BUILD_JOBS:-8}"

say() { printf '%s\n' "$*"; }
die() { printf 'error: %s\n' "$*" >&2; exit 1; }

command -v python3 >/dev/null 2>&1 || die "需要 python3 来写入 Steam shortcuts.vdf"

# 1. Runtime binary ---------------------------------------------------------
if [ -x build-native/biko3 ]; then
  install -Dm755 build-native/biko3 "$BIN"
  say "==> 已安装 build-native/biko3 -> $BIN"
elif [ -x "$BIN" ]; then
  say "==> 使用已有运行时 $BIN"
else
  say "==> 构建桌面版（需要 CMake、Vulkan 头文件、SDL3、glslangValidator）"
  [ -x ./build-linux.sh ] || die "缺少 build-linux.sh"
  BK_BUILD_JOBS="$JOBS" ./build-linux.sh
  install -Dm755 build-native/biko3 "$BIN"
  say "==> 已安装 $BIN"
fi

# 2. Game data --------------------------------------------------------------
if [ ! -d "$DATA/Data" ]; then
  die "找不到游戏数据: $DATA/Data
请把原版 Data/ 和汉化去码 patch.pp 放到 $PREFIX/game，或用 BIKO3_GAME=/path 指定后重试。"
fi

# 3. Launcher ---------------------------------------------------------------
install -d "$HOME/.local/bin"
install -Dm755 tools/biko3-launcher.sh "$LAUNCHER"
say "==> 启动器 $LAUNCHER"

# 4. Desktop entry / icon ---------------------------------------------------
if [ -f "$PREFIX/steam-cover.png" ]; then
  install -d "$HOME/.local/share/icons"
  if command -v ffmpeg >/dev/null 2>&1; then
    ffmpeg -loglevel error -y -i "$PREFIX/steam-cover.png" \
      -vf "crop=600:600:0:0,scale=256:256" "$ICON" 2>/dev/null || cp "$PREFIX/steam-cover.png" "$ICON"
  else
    cp "$PREFIX/steam-cover.png" "$ICON"
  fi
fi
if [ ! -f "$ICON" ] && [ -f "$PREFIX/screenshot-title.png" ] &&
   command -v ffmpeg >/dev/null 2>&1; then
  install -d "$HOME/.local/share/icons"
  ffmpeg -loglevel error -y -i "$PREFIX/screenshot-title.png" \
    -vf "crop=400:400:120:120,scale=256:256" "$ICON" 2>/dev/null || true
fi

install -d "$HOME/.local/share/applications"
cat > "$HOME/.local/share/applications/biko3.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=$NAME
Comment=Native Biko3 runtime (Vulkan)
Exec=$LAUNCHER
Icon=${ICON}
Terminal=false
Categories=Game;
StartupNotify=true
EOF
chmod 755 "$HOME/.local/share/applications/biko3.desktop"
if [ -d "$HOME/Desktop" ]; then
  cp "$HOME/.local/share/applications/biko3.desktop" "$HOME/Desktop/biko3.desktop"
  chmod 755 "$HOME/Desktop/biko3.desktop"
fi
say "==> 桌面快捷方式"

# 5. Steam artwork and shortcuts.vdf ---------------------------------------
ART=""
HERO=""
if [ -f "$PREFIX/steam-cover.png" ]; then
  ART="$PREFIX/steam-cover.png"
elif [ -f "$PREFIX/screenshot-title.png" ] && command -v ffmpeg >/dev/null 2>&1; then
  ART="$PREFIX/steam-cover.png"
  HERO="$PREFIX/steam-hero.png"
  ffmpeg -loglevel error -y -i "$PREFIX/screenshot-title.png" \
    -vf "crop=720:1080:600:0,scale=600:900" "$ART"
  ffmpeg -loglevel error -y -i "$PREFIX/screenshot-title.png" \
    -vf "crop=1920:620:0:230" "$HERO"
fi
[ -f "$PREFIX/steam-hero.png" ] && HERO="$PREFIX/steam-hero.png"

set -- --exe "$LAUNCHER" --name "$NAME" --icon "$ICON"
[ -n "$ART" ] && set -- "$@" --artwork "$ART"
[ -n "$HERO" ] && set -- "$@" --hero "$HERO"
[ -f "$ICON" ] && set -- "$@" --icon-image "$ICON"
python3 tools/add_steam_shortcut.py "$@"

say ""
say "完成。重新打开 Steam，在库中启动「$NAME」。"
say "也可以直接运行: $LAUNCHER"
