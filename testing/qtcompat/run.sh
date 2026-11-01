#!/bin/sh
# Builds qtcompat_test with Qt5 and Qt6 (Docker images from scripts/docker)
# and checks that both produce identical output.
set -e
SRC="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$SRC/build-qtcompat-test"
for QT in 5 6; do
    docker run --rm -u "$(id -u):$(id -g)" -v "$SRC:/work" -w /work "kumir2-qt$QT-native" sh -c "
        cmake -S testing/qtcompat -B build-qtcompat-test/qt$QT -DUSE_QT=$QT >/dev/null &&
        cmake --build build-qtcompat-test/qt$QT >/dev/null &&
        build-qtcompat-test/qt$QT/qtcompat_test" > "$OUT.qt$QT.txt"
done
if diff -u "$OUT.qt5.txt" "$OUT.qt6.txt"; then
    echo "qtcompat: Qt5 and Qt6 outputs are identical ($(wc -l < "$OUT.qt5.txt") checks)"
else
    echo "qtcompat: Qt5 and Qt6 outputs DIFFER" >&2
    exit 1
fi
