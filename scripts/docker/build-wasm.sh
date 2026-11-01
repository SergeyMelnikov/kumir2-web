#!/bin/sh
# Configure and build kumir2-classic for WebAssembly in the kumir2-qt6-wasm image.
# Usage: scripts/docker/build-wasm.sh [extra cmake --build args]
# Environment: KUMIR2_CMAKE_ARGS - extra configure arguments
#              BUILD_DIR - build directory (default build-wasm)
# Output: $BUILD_DIR/bin/kumir2-classic.{html,js,wasm,data}
set -e
SRC="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-build-wasm}"
docker run --rm -u "$(id -u):$(id -g)" -e HOME=/tmp/home -e BUILD_DIR="$BUILD_DIR" -v "$SRC:/work" -w /work kumir2-qt6-wasm sh -c '
    mkdir -p "$BUILD_DIR"
    exec 9>"$BUILD_DIR/.build.lock"
    flock -n 9 || { echo "Another WASM build is using $BUILD_DIR" >&2; exit 1; }
    mkdir -p /tmp/home && . /opt/emsdk/emsdk_env.sh >/dev/null 2>&1 &&
    $QT_WASM/bin/qt-cmake -S . -B "$BUILD_DIR" -G Ninja -DUSE_QT=6 -DCMAKE_BUILD_TYPE=Release \
        -DQT_HOST_PATH=$QT_HOST -DKumir2_DIR=/work/cmake '"$KUMIR2_CMAKE_ARGS"' > "$BUILD_DIR-configure.log" 2>&1 || { tail -40 "$BUILD_DIR-configure.log"; exit 1; }
    cmake --build "$BUILD_DIR" --target kumir2-classic '"$*"'
'
