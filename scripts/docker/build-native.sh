#!/bin/sh
# Configure and build Kumir2 natively inside a Docker image.
# Usage: scripts/docker/build-native.sh <5|6> [extra cmake --build args]
# Images: kumir2-qt5-native / kumir2-qt6-native (see *.Dockerfile here).
# Output goes to build-qt<N>/ in the source tree.
set -e
QT="$1"; shift
case "$QT" in 5|6) ;; *) echo "usage: $0 <5|6> [build args]" >&2; exit 2;; esac
SRC="$(cd "$(dirname "$0")/../.." && pwd)"
docker run --rm -u "$(id -u):$(id -g)" -v "$SRC:/work" -w /work "kumir2-qt$QT-native" sh -c '
    cmake -S . -B build-qt'"$QT"' -G Ninja -DUSE_QT='"$QT"' -DCMAKE_BUILD_TYPE=RelWithDebInfo >/dev/null &&
    cmake --build build-qt'"$QT"' '"$*"'
'
