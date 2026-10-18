#!/bin/sh
# Desktop (Linux) build of the Biko3 runtime.
set -eu
cd "$(dirname "$0")"
cmake -S . -B build-native \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBK_NATIVE_WINDOW=ON \
  -DBK_BUILD_TESTS=OFF \
  -DMESA_SWITCH_SDK="${MESA_SWITCH_SDK:-/usr}"
cmake --build build-native --parallel "${BK_BUILD_JOBS:-8}" --target biko3-preview
