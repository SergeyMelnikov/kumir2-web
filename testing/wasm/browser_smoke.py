#!/usr/bin/env python3
"""Smoke test of the WebAssembly build in headless Chromium/Firefox.

Opens the page, records browser console output, takes screenshots, and
optionally replays simple actions.

Usage (see run.sh): browser_smoke.py URL OUTDIR [ACTIONS]
ACTIONS file, one per line:
  wait SECONDS          sleep
  shot NAME             screenshot OUTDIR/NAME.png
  click X Y             mouse click at page coordinates
  key KEY               press a key (Playwright names: F9, Escape, Control+s ...)
  type TEXT             type text (may contain Cyrillic)
  reload                reload in the same browser context
  eval EXPRESSION       log the value of a JavaScript expression
  trace-input           log DOM keyboard and clipboard events
  read-clipboard        log browser clipboard text (requires permission)
  paste TEXT            dispatch a synthetic DOM paste with plain text
  upload FILE           choose FILE with Ctrl+O (html/mock picker mode)
  download NAME         save with Ctrl+S to OUTDIR/NAME (html/mock mode)
  cancel-save           cancel the next native save (mock mode)
  fail-save             fail the next native write (mock mode)
  cancel-upload         cancel the next native open (mock mode)
  picker-delay MS       native pickers answer after MS milliseconds (mock mode)
  mock-files FILE...    files the next native open returns (mock mode)
  check-saves N         fail unless N files were saved by native pickers (mock mode)
"""

import os
import sys
import time
import base64
from pathlib import Path
from playwright.sync_api import sync_playwright


def main():
    url, outdir = sys.argv[1], sys.argv[2]
    actions = []
    if len(sys.argv) > 3:
        with open(sys.argv[3], encoding="utf-8") as f:
            actions = [l.rstrip("\n") for l in f if l.strip() and not l.startswith("#")]

    log = open(f"{outdir}/console.log", "w", encoding="utf-8")
    t0 = time.time()
    errors = []

    def stamp(msg):
        log.write(f"[{time.time() - t0:7.2f}] {msg}\n")
        log.flush()

    def page_error(error):
        errors.append(str(error))
        stamp(f"PAGEERROR: {error}\n{error.stack}")

    with sync_playwright() as p:
        engine = os.environ.get("KUMIR2_TEST_BROWSER", "chromium")
        if engine not in ("chromium", "firefox"):
            raise SystemExit(f"unsupported browser: {engine}")
        args = ["--enable-unsafe-swiftshader", "--use-angle=swiftshader"] if engine == "chromium" else []
        browser = getattr(p, engine).launch(args=args)
        page_options = {"viewport": {"width": 1280, "height": 800}}
        if os.environ.get("KUMIR2_TEST_CLIPBOARD") == "1":
            page_options["permissions"] = ["clipboard-read", "clipboard-write"]
        page = browser.new_page(**page_options)
        picker_mode = os.environ.get("KUMIR2_TEST_FILE_PICKER", "native")
        if picker_mode == "mock":
            page.add_init_script(path=str(Path(__file__).with_name("native_file_picker_mock.js")))
        elif picker_mode == "html":
            # Native File System Access pickers are not Playwright filechooser
            # events. Exercise Qt's standard HTML input/download fallback.
            page.add_init_script("window.showOpenFilePicker = undefined; window.showSaveFilePicker = undefined;")
        keyboard = page.context.new_cdp_session(page) if engine == "chromium" else None
        stamp(f"browser={engine}; version={browser.version}")
        page.on("console", lambda m: stamp(f"console.{m.type}: {m.text}"))
        page.on("pageerror", page_error)
        page.on("requestfailed", lambda r: stamp(f"REQUESTFAILED: {r.url} {r.failure}"))
        page.goto(url)
        try:
            stamp(f"loaded {url}; crossOriginIsolated={page.evaluate('self.crossOriginIsolated')}")
        except Exception as error:  # e.g. the page reloads itself (coi-serviceworker)
            stamp(f"loaded {url}; initial state not available: {error}")
        for action in actions:
            cmd, _, arg = action.partition(" ")
            stamp(f"action: {action}")
            if cmd == "wait":
                page.wait_for_timeout(float(arg) * 1000)
            elif cmd == "reload":
                page.reload()
            elif cmd == "eval":
                stamp(f"eval {arg} = {page.evaluate(arg)}")
            elif cmd == "trace-input":
                page.evaluate("""() => {
                    for (const type of ['keydown', 'keyup', 'copy', 'cut', 'paste', 'beforeinput'])
                        document.addEventListener(type, event => {
                            const target = event.composedPath()[0];
                            const targetClass = target?.className;
                            const clipboardTypes = Array.from(event.clipboardData?.types || []);
                            const clipboardText = event.clipboardData?.getData('text/plain');
                            queueMicrotask(() => console.log('DOMINPUT ' + JSON.stringify({
                                type, key: event.key, code: event.code,
                                trusted: event.isTrusted, prevented: event.defaultPrevented,
                                inputType: event.inputType, targetClass, clipboardTypes, clipboardText
                            })));
                        }, true);
                }""")
            elif cmd == "read-clipboard":
                stamp(f"clipboard-text: {page.evaluate('navigator.clipboard.readText()')!r}")
            elif cmd == "paste":
                page.evaluate("""text => {
                    let target = document.activeElement;
                    while (target.shadowRoot?.activeElement)
                        target = target.shadowRoot.activeElement;
                    const data = new DataTransfer();
                    data.setData('text/plain', text);
                    const event = new Event('paste', {
                        bubbles: true, composed: true, cancelable: true
                    });
                    Object.defineProperty(event, 'clipboardData', { value: data });
                    target.dispatchEvent(event);
                }""", arg)
            elif cmd == "upload":
                if picker_mode == "mock":
                    encoded = base64.b64encode(Path(arg).read_bytes()).decode("ascii")
                    page.evaluate("""file => {
                        const bytes = Uint8Array.from(atob(file.data), c => c.charCodeAt(0));
                        filePickerTest.files = [new File([bytes], file.name)];
                    }""", {"name": Path(arg).name, "data": encoded})
                    page.keyboard.press("Control+o")
                else:
                    with page.expect_file_chooser() as chooser:
                        page.keyboard.press("Control+o")
                    chooser.value.set_files(arg)
            elif cmd == "download":
                if picker_mode == "mock":
                    before = page.evaluate("filePickerTest.saves.length")
                    page.keyboard.press("Control+s")
                    page.wait_for_function("count => filePickerTest.saves.length > count", arg=before)
                    result = page.evaluate("filePickerTest.saves.at(-1)")
                    Path(outdir, arg).write_bytes(bytes(result["bytes"]))
                    stamp(f"native-picker saved: {result['name']} -> {arg}")
                else:
                    with page.expect_download() as pending:
                        page.keyboard.press("Control+s")
                    download = pending.value
                    download.save_as(f"{outdir}/{arg}")
                    stamp(f"downloaded: {download.suggested_filename} -> {arg}")
            elif cmd == "cancel-save":
                page.evaluate("filePickerTest.cancelSave = true")
                page.keyboard.press("Control+s")
            elif cmd == "fail-save":
                page.evaluate("filePickerTest.failWrite = true")
                page.keyboard.press("Control+s")
            elif cmd == "cancel-upload":
                page.evaluate("filePickerTest.cancelOpen = true")
                page.keyboard.press("Control+o")
            elif cmd == "picker-delay":
                page.evaluate("ms => { filePickerTest.delayMs = ms; }", int(arg))
            elif cmd == "mock-files":
                files = [{"name": Path(f).name, "data": base64.b64encode(Path(f).read_bytes()).decode("ascii")}
                         for f in arg.split()]
                page.evaluate("""files => {
                    filePickerTest.files = files.map(file => new File(
                        [Uint8Array.from(atob(file.data), c => c.charCodeAt(0))], file.name));
                }""", files)
            elif cmd == "check-saves":
                saves = page.evaluate("filePickerTest.saves.map(save => [save.name, save.bytes.length])")
                stamp(f"native-picker saves: {saves}")
                if len(saves) != int(arg):
                    errors.append(f"expected {arg} native saves, got {len(saves)}")
            elif cmd == "shot":
                page.screenshot(path=f"{outdir}/{arg}.png")
            elif cmd == "click":
                x, y = arg.split()
                page.mouse.click(float(x), float(y))
            elif cmd == "key":
                page.keyboard.press(arg)
            elif cmd == "type":
                # Qt's canvas editor handles key events. Playwright's type()
                # uses insertText for non-ASCII, which bypasses that handler.
                for char in arg:
                    if ord(char) > 127 and keyboard:
                        keyboard.send("Input.dispatchKeyEvent", {
                            "type": "keyDown", "key": char, "text": char,
                            "unmodifiedText": char,
                        })
                        keyboard.send("Input.dispatchKeyEvent", {
                            "type": "keyUp", "key": char,
                        })
                    elif ord(char) > 127:
                        # Firefox has no CDP key dispatch. Exercise Qt's DOM
                        # key handler; these events are synthetic/untrusted.
                        page.evaluate("""char => {
                            let target = document.activeElement;
                            while (target.shadowRoot?.activeElement)
                                target = target.shadowRoot.activeElement;
                            for (const type of ['keydown', 'keyup'])
                                target.dispatchEvent(new KeyboardEvent(type, {
                                    key: char, bubbles: true, cancelable: true
                                }));
                        }""", char)
                    else:
                        page.keyboard.type(char, delay=30)
            else:
                raise SystemExit(f"unknown action: {action}")
        browser.close()
    stamp("done")
    log.close()
    if errors:
        raise SystemExit(f"browser smoke test failed: {len(errors)} page error(s); see {outdir}/console.log")


if __name__ == "__main__":
    main()
