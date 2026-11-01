# Headless Chromium and Firefox (Playwright) for WebAssembly smoke tests.
FROM python:3.12-slim-bookworm
RUN pip install --no-cache-dir playwright==1.64.1 \
    && playwright install --with-deps chromium firefox \
    && rm -rf /var/lib/apt/lists/* /root/.cache/pip
ENV PLAYWRIGHT_BROWSERS_PATH=/root/.cache/ms-playwright
RUN chmod -R a+rX /root /root/.cache/ms-playwright
WORKDIR /work
