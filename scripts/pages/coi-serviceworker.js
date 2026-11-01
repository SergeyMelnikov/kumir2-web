// Makes the page cross-origin isolated on static hosts that can not send
// COOP/COEP headers (GitHub Pages). Multithreaded Qt for WebAssembly needs
// SharedArrayBuffer, which browsers enable only for isolated pages.
//
// Included from the page as <script src="coi-serviceworker.js"></script>:
// the page registers this file as a service worker, which re-serves every
// same-origin response with the two headers; then the page reloads once.
// While that reload is pending, window.coiReloading is true: the page should
// not start the application (prepare-site.sh makes the Qt shell check it).

if (typeof window === "undefined") {
	// Service worker context
	self.addEventListener("install", () => self.skipWaiting());
	self.addEventListener("activate", (event) => event.waitUntil(self.clients.claim()));
	self.addEventListener("fetch", (event) => {
		const request = event.request;
		if (request.cache === "only-if-cached" && request.mode !== "same-origin") {
			return;
		}
		event.respondWith(
			fetch(request).then((response) => {
				if (response.status === 0) {
					return response; // opaque response: headers can not be changed
				}
				const headers = new Headers(response.headers);
				headers.set("Cross-Origin-Embedder-Policy", "require-corp");
				headers.set("Cross-Origin-Opener-Policy", "same-origin");
				return new Response(response.body, {
					status: response.status,
					statusText: response.statusText,
					headers: headers,
				});
			})
		);
	});
} else {
	// Page context
	(() => {
		const reloadKey = "coiServiceWorkerReloaded";
		if (window.crossOriginIsolated) {
			sessionStorage.removeItem(reloadKey);
			return;
		}
		if (!window.isSecureContext || !("serviceWorker" in navigator)) {
			console.error("coi-serviceworker: service workers are not available, threads will not work");
			return;
		}
		if (sessionStorage.getItem(reloadKey)) {
			console.error("coi-serviceworker: page is still not cross-origin isolated after reload");
			return;
		}
		window.coiReloading = true;
		navigator.serviceWorker.register(document.currentScript.src).then(
			() => navigator.serviceWorker.ready
		).then(() => {
			// Reload once so the page itself is served with the headers
			sessionStorage.setItem(reloadKey, "1");
			window.location.reload();
		}, (error) => {
			console.error("coi-serviceworker: registration failed", error);
			window.coiReloading = false;
		});
	})();
}
