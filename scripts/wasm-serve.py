#!/usr/bin/env python3
"""Local HTTP server for the WebAssembly build of Kumir2.

Multithreaded Qt for WebAssembly needs SharedArrayBuffer, which browsers
enable only for cross-origin isolated pages, so every response carries
  Cross-Origin-Opener-Policy: same-origin
  Cross-Origin-Embedder-Policy: require-corp

Usage: scripts/wasm-serve.py [directory] [port]
       (defaults: build-wasm/bin 8090), then open
       http://localhost:8090/kumir2-classic.html
"""

import functools
import http.server
import sys


class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {
        **http.server.SimpleHTTPRequestHandler.extensions_map,
        ".wasm": "application/wasm",
        ".js": "text/javascript",
        ".data": "application/octet-stream",
    }

    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cache-Control", "no-store")
        super().end_headers()


def main():
    directory = sys.argv[1] if len(sys.argv) > 1 else "build-wasm/bin"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 8090
    handler = functools.partial(Handler, directory=directory)
    with http.server.ThreadingHTTPServer(("127.0.0.1", port), handler) as server:
        print(f"Serving {directory} at http://localhost:{port}/kumir2-classic.html", flush=True)
        server.serve_forever()


if __name__ == "__main__":
    main()
