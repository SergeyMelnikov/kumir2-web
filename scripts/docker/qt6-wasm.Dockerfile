# WebAssembly build environment for Kumir2: Qt for WebAssembly (multithreaded)
# + host Qt of the same version (moc, rcc, uic, lrelease) + Emscripten SDK.
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build python3 python3-venv git ca-certificates \
        xz-utils bzip2 curl \
    && rm -rf /var/lib/apt/lists/*

ARG QT_VERSION=6.10.2
RUN python3 -m venv /opt/aqt && /opt/aqt/bin/pip install --no-cache-dir aqtinstall==3.3.0 \
    && /opt/aqt/bin/aqt install-qt all_os wasm ${QT_VERSION} wasm_multithread -m qt5compat --autodesktop -O /opt/Qt \
    && /opt/aqt/bin/aqt install-qt linux desktop ${QT_VERSION} linux_gcc_64 -m qt5compat -O /opt/Qt \
    && rm -rf /root/.cache

ARG EMSDK_VERSION=4.0.7
RUN git clone --depth 1 https://github.com/emscripten-core/emsdk.git /opt/emsdk \
    && cd /opt/emsdk && ./emsdk install ${EMSDK_VERSION} && ./emsdk activate ${EMSDK_VERSION} \
    && chmod -R a+rwX /opt/emsdk/upstream/emscripten/cache 2>/dev/null || true

# Runtime dependencies of host Qt tools (lrelease uses QtCore with glib event loop)
RUN apt-get update && apt-get install -y --no-install-recommends libglib2.0-0t64 \
    && rm -rf /var/lib/apt/lists/*

ENV EMSDK=/opt/emsdk \
    QT_WASM=/opt/Qt/${QT_VERSION}/wasm_multithread \
    QT_HOST=/opt/Qt/${QT_VERSION}/gcc_64
WORKDIR /work
