# Browser smoke tests

Builds, browsers, and the HTTP server run in Docker. Start the server from the
repository root:

```sh
docker build -f scripts/docker/browser-test.Dockerfile -t kumir2-browser-test .
docker run --rm -d --name kumir2-wasm-server --network host \
  -u "$(id -u):$(id -g)" -v "$PWD:/work:ro" -w /work \
  kumir2-browser-test python3 scripts/wasm-serve.py
```

In another terminal, create an output directory and choose an action fixture:

```sh
mkdir -p /tmp/kumir-browser-test
testing/wasm/run.sh /tmp/kumir-browser-test testing/wasm/robot-execution.actions
KUMIR2_TEST_BROWSER=firefox testing/wasm/run.sh /tmp/kumir-browser-test testing/wasm/robot-execution.actions
```

Use separate output directories to preserve each run. The runner writes console
logs and screenshots. A JavaScript page error makes it exit nonzero. Program
output, actor rendering, and source highlighting require screenshot review;
a zero exit status alone does not prove the program ran correctly.

Fixtures cover Robot execution, stepping and stopping; 20 rapid start/stop
cycles; Draw rendering; terminal input; input cancellation; clipboard; and
program files, unsaved-file prompts, save cancellation/write errors, and
Preferences persistence. Asyncify is enabled by default in the WASM build
for the existing modal Qt dialogs.

For browser file round trips:

```sh
KUMIR2_TEST_FILE_PICKER=html testing/wasm/run.sh /tmp/kumir-browser-test testing/wasm/file-roundtrip.actions
```

`html` exercises Qt's HTML upload/download fallback. `mock` exercises the
native File System Access promise/stream path with a controlled picker, which
also supports `save-cancel.actions`, `save-failure.actions`, and
`upload-cancel.actions`. Native OS picker UI is not automated by Playwright's
`filechooser` event. Firefox uses the HTML fallback by default.

`menu-file-pickers.actions` and `course-workbook.actions` (mock mode) open
pickers from menus with `picker-delay`, so the pickers answer after a second
as with a real user: Program > Save, Robot > Load/Save environment, and
Practicum > Load course with a workbook saved by the save picker. They check
the number of saved files with `check-saves`.

For a Chromium clipboard test with explicit browser permissions:

```sh
KUMIR2_TEST_CLIPBOARD=1 testing/wasm/run.sh /tmp/kumir-browser-test testing/wasm/clipboard.actions
```

Without this option, headless Chromium may deny clipboard writes. Firefox does
not support these Playwright permission grants. Unicode typing uses Chromium
CDP key events or synthetic Firefox DOM key events aimed at Qt's focused
element inside its shadow DOM. These tests do not validate physical keyboard
layouts or IME input.

# WASM compatibility and settings probes

Build the existing compatibility test and the WASM settings probe:

```sh
docker run --rm -u "$(id -u):$(id -g)" -e HOME=/tmp/home \
  -v "$PWD:/work" -w /work kumir2-qt6-wasm sh -c '
    mkdir -p /tmp/home && . /opt/emsdk/emsdk_env.sh >/dev/null 2>&1 &&
    $QT_WASM/bin/qt-cmake -S testing/qtcompat -B build-wasm-qtcompat \
      -G Ninja -DUSE_QT=6 -DQT_HOST_PATH=$QT_HOST -DCMAKE_BUILD_TYPE=Release &&
    cmake --build build-wasm-qtcompat -j 4'
docker run --rm -d --name kumir2-wasm-probes --network host \
  -u "$(id -u):$(id -g)" -v "$PWD:/work:ro" -w /work \
  kumir2-browser-test python3 scripts/wasm-serve.py build-wasm-qtcompat 8091
```

In another terminal:

```sh
mkdir -p /tmp/kumir-settings
testing/wasm/run.sh /tmp/kumir-settings testing/wasm/settings-reload.actions \
  http://localhost:8091/wasm_settings_probe.html
```

The settings probe should log `previous=0 current=1 status=0` before reload and
`previous=1 current=2 status=0` afterward for both formats. It tests Qt storage,
not the application's settings-save lifecycle. Use `qtcompat_test.html` at the
same server to collect compatibility/codec results; its 27 console output lines
should match the native reference produced by `testing/qtcompat/run.sh`.

See `codex.txt` at the repository root for observed results and remaining issues.
Stop the HTTP containers when finished with `docker stop kumir2-wasm-server`
and, if started, `docker stop kumir2-wasm-probes`.
