#!/bin/sh
# Assembles a static site (GitHub Pages) from the WebAssembly build output.
# Usage: scripts/pages/prepare-site.sh [build-bin-dir] [site-dir]
#        (defaults: build-wasm/bin _site)
# The page gets coi-serviceworker.js, because static hosts can not send the
# COOP/COEP headers required by multithreaded WebAssembly.
set -e
BIN="${1:-build-wasm/bin}"
SITE="${2:-_site}"
HERE="$(cd "$(dirname "$0")" && pwd)"
APP=kumir2-classic

for f in "$APP.html" "$APP.js" "$APP.wasm" "$APP.data" qtloader.js; do
    [ -f "$BIN/$f" ] || { echo "Missing $BIN/$f: build with scripts/docker/build-wasm.sh first" >&2; exit 1; }
done

rm -rf "$SITE"
mkdir -p "$SITE"
cp "$BIN/$APP.js" "$BIN/$APP.wasm" "$BIN/$APP.data" "$BIN/qtloader.js" "$SITE/"
[ -f "$BIN/qtlogo.svg" ] && cp "$BIN/qtlogo.svg" "$SITE/"
cp "$HERE/coi-serviceworker.js" "$SITE/"

# Load the service worker script first in <head>, and do not start the
# application while the service worker reload is pending
grep -q '</head>' "$BIN/$APP.html" || { echo "No </head> in $BIN/$APP.html" >&2; exit 1; }
grep -q 'onload="init()"' "$BIN/$APP.html" || { echo "No onload=\"init()\" in $BIN/$APP.html" >&2; exit 1; }
sed -e 's|</head>|    <script src="coi-serviceworker.js"></script>\n  </head>|' \
    -e 's|onload="init()"|onload="if (!window.coiReloading) init()"|' \
    "$BIN/$APP.html" > "$SITE/$APP.html"
cp "$SITE/$APP.html" "$SITE/index.html"
ls -l "$SITE"
