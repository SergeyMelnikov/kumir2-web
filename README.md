# Kumir 2 in the browser (kumir2-web)

A fork of [kumir2](https://github.com/a-a-maly/kumir2) that runs the classic Kumir IDE
(`kumir2-classic`) in a web browser via Qt for WebAssembly, so it works on any machine
without installation: https://sergeymelnikov.github.io/kumir2-web/ (deployed from `main` by
`.github/workflows/pages.yml`). The original README is in [README.orig.md](README.orig.md).

What was changed:
- **Qt6 port** (`-DUSE_QT=6`): Qt4/Qt5 code paths are unchanged; differences are in `include/kumir2/qtcompat*.h`.
- **Static plugins** (`-DKUMIR2_STATIC_PLUGINS=ON`): all plugins are linked into one executable.
- **WebAssembly build**: browser file open/save, clipboard and launcher fixes, all behind `Q_OS_WASM`.

How to build and run (needs only Docker):
```sh
docker build -t kumir2-qt6-wasm -f scripts/docker/qt6-wasm.Dockerfile scripts/docker
scripts/docker/build-wasm.sh                         # -> build-wasm/bin/kumir2-classic.*
python3 scripts/wasm-serve.py build-wasm/bin 8090    # then open http://localhost:8090/kumir2-classic.html
```
Native Qt5/Qt6 builds: image `kumir2-qt<N>-native` from `scripts/docker/qt<N>-native.Dockerfile`, then `scripts/docker/build-native.sh <N>`.
Tests: `testing/qtcompat/run.sh` (Qt5 vs Qt6 helpers), `testing/wasm/` (browser scenarios).
