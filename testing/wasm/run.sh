#!/bin/sh
# Runs Chromium/Firefox browser_smoke.py in kumir2-browser-test against a server
# started with scripts/wasm-serve.py (default http://localhost:8090).
# Usage: testing/wasm/run.sh OUTDIR [ACTIONS-file] [URL]
# Environment: KUMIR2_TEST_BROWSER=chromium (default) or firefox.
# KUMIR2_TEST_CLIPBOARD=1 grants clipboard permissions (Chromium).
# KUMIR2_TEST_FILE_PICKER=html tests Qt's HTML upload/download fallback.
set -e
SRC="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$(cd "$1" && pwd)"; ACTIONS="$2"; URL="${3:-http://localhost:8090/kumir2-classic.html}"
[ -n "$ACTIONS" ] && cp "$ACTIONS" "$OUT/actions.txt" || printf 'wait 30\nshot started\n' > "$OUT/actions.txt"
docker run --rm --network host -u "$(id -u):$(id -g)" -e HOME=/tmp -e KUMIR2_TEST_BROWSER="${KUMIR2_TEST_BROWSER:-chromium}" -e KUMIR2_TEST_CLIPBOARD="${KUMIR2_TEST_CLIPBOARD:-0}" -e KUMIR2_TEST_FILE_PICKER="${KUMIR2_TEST_FILE_PICKER:-native}" -v "$SRC/testing/wasm:/t:ro" -v "$OUT:/out" \
    kumir2-browser-test python3 /t/browser_smoke.py "$URL" /out /out/actions.txt
