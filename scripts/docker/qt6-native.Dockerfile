# Native Linux build environment for Kumir2 with Qt6 (used to validate the Qt6 port).
FROM ubuntu:26.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build python3 git ca-certificates \
        qt6-base-dev qt6-base-dev-tools qt6-svg-dev qt6-5compat-dev \
        qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools libgl-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /work
# Headless GUI testing: virtual X server, fonts, screenshots, input automation
RUN apt-get update && apt-get install -y --no-install-recommends \
        xvfb xauth fonts-dejavu-core imagemagick x11-utils xdotool \
    && rm -rf /var/lib/apt/lists/*
