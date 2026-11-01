#!/bin/sh
# Starts kumir2-classic from build-qt<N>/ under Xvfb, optionally types keys,
# saves screenshots and the application log into <outdir>.
# Usage: scripts/docker/gui-smoke.sh <5|6> <outdir> [xdotool-script] [file-in-outdir-to-open]
#   xdotool-script: file with xdotool commands (one per line), executed after startup;
#   a line "shot NAME" saves a screenshot NAME.png, "sleep N" waits.
# Environment: BUILD_DIR (default build-qt<N>) selects the build to run.
set -e
QT="$1"; OUT="$(cd "$2" && pwd)"; SCRIPT="$3"; OPEN="${4:+/out/$4}"
SRC="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:-build-qt$QT}"
[ -n "$SCRIPT" ] && cp "$SCRIPT" "$OUT/actions.txt" || : > "$OUT/actions.txt"
docker run --rm -u "$(id -u):$(id -g)" -e HOME=/tmp/home -e LANG=C.UTF-8 -e QT_LOGGING_RULES="${QT_LOGGING_RULES:-}" \
    -v "$SRC:/work:ro" -v "$OUT:/out" -w /work "kumir2-qt$QT-native" sh -c '
mkdir -p /tmp/home
xvfb-run -a -s "-screen 0 1280x800x24" sh -c "
  '"$BUILD_DIR"'/bin/kumir2-classic '"$OPEN"' > /out/app.log 2>&1 &
  PID=\$!
  sleep 8
  import -window root /out/start.png
  while read -r cmd arg; do
    case \"\$cmd\" in
      \"\"|\#*) ;;
      shot) import -window root /out/\$arg.png ;;
      sleep) sleep \$arg ;;
      *) xdotool \$cmd \$arg ;;
    esac
  done < /out/actions.txt
  if kill -0 \$PID 2>/dev/null; then echo alive=yes; kill \$PID; sleep 1; else echo alive=no; fi
"'
