#!/bin/sh
# Build the native macOS (SDL3 + MoltenVK) runtime.
#
# Requirements (Homebrew):
#   brew install cmake sdl3 molten-vk vulkan-loader vulkan-headers glslang
#
# Output: build-macos/biko3 (the biko3-preview target is renamed when
# BK_NATIVE_WINDOW is enabled).
set -eu
cd "$(dirname "$0")"

if [ "$(uname -s)" != "Darwin" ]; then
  echo "build-macos.sh must run on macOS (use build-linux.sh / build-host.sh on other hosts)" >&2
  exit 2
fi
if ! command -v brew >/dev/null 2>&1; then
  echo "Homebrew is required: https://brew.sh" >&2
  exit 2
fi
if ! command -v cmake >/dev/null 2>&1; then
  echo "cmake is required (brew install cmake)" >&2
  exit 2
fi

require_formula() {
  if ! brew --prefix "$1" >/dev/null 2>&1; then
    echo "missing Homebrew formula: $1 (brew install $1)" >&2
    exit 2
  fi
}

require_formula sdl3
require_formula molten-vk
require_formula vulkan-loader
require_formula vulkan-headers
require_formula glslang

BREW_PREFIX="$(brew --prefix)"
SDL3_PREFIX="$(brew --prefix sdl3)"
MOLTENVK_PREFIX="$(brew --prefix molten-vk)"
VULKAN_HEADERS_PREFIX="$(brew --prefix vulkan-headers)"
GLSLANG_PREFIX="$(brew --prefix glslang)"
export PATH="$GLSLANG_PREFIX/bin:$PATH"

if ! command -v glslangValidator >/dev/null 2>&1; then
  echo "glslangValidator not found; install glslang" >&2
  exit 2
fi

BUILD_DIR="${BK_BUILD_DIR:-build-macos}"
MIN_VERSION="${BK_MACOS_MIN:-12.0}"

# One arch by default (native). Set BK_MACOS_ARCH="arm64;x86_64" for a
# universal app; Homebrew MoltenVK is normally single-arch, so universal
# builds need a matching MoltenVK build.
ARCH_ARGS=""
if [ -n "${BK_MACOS_ARCH:-}" ]; then
  ARCH_ARGS="-DCMAKE_OSX_ARCHITECTURES=${BK_MACOS_ARCH}"
fi

echo "Configuring ${BUILD_DIR}"
# shellcheck disable=SC2086
cmake -S . -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE="${BK_BUILD_TYPE:-RelWithDebInfo}" \
  -DBK_NATIVE_WINDOW=ON \
  -DBK_BUILD_TESTS=OFF \
  -DCMAKE_PREFIX_PATH="$BREW_PREFIX" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$MIN_VERSION" \
  -DMOLTENVK_PREFIX="$MOLTENVK_PREFIX" \
  -DMESA_SWITCH_SDK="$VULKAN_HEADERS_PREFIX" \
  ${ARCH_ARGS}

cmake --build "$BUILD_DIR" --parallel "${BK_BUILD_JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || echo 8)}" \
  --target biko3-preview

BIN="$BUILD_DIR/biko3"
if [ ! -x "$BIN" ]; then
  echo "build finished but $BIN is missing" >&2
  exit 1
fi

ICD="$MOLTENVK_PREFIX/etc/vulkan/icd.d/MoltenVK_icd.json"
if [ ! -f "$ICD" ] && [ -f "$MOLTENVK_PREFIX/share/vulkan/icd.d/MoltenVK_icd.json" ]; then
  ICD="$MOLTENVK_PREFIX/share/vulkan/icd.d/MoltenVK_icd.json"
fi
if [ ! -f "$ICD" ]; then
  ICD="/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json"
fi

cat <<EOF

Built: $BIN

Run (windowed):
  BIKO3_GAME="\$HOME/Library/Application Support/Biko3/game" BIKO3_WINDOWED=1 "$BIN"

Default game data directory:
  \$HOME/Library/Application Support/Biko3/game

If the Vulkan loader cannot find MoltenVK, export:
  VK_ICD_FILENAMES="$ICD"

Package a .app:
  ./tools/package_macos_app.sh
EOF
