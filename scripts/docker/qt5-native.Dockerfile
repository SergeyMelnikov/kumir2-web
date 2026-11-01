# Native Linux build environment for Kumir2 with Qt5 (regression check of the Qt6 port).
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build python3 git ca-certificates \
        qtbase5-dev qtbase5-dev-tools qttools5-dev-tools libqt5svg5-dev \
        libqt5x11extras5-dev qtscript5-dev libx11-dev libgl-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /work
# Headless GUI testing: virtual X server, fonts, screenshots, input automation
RUN apt-get update && apt-get install -y --no-install-recommends \
        xvfb xauth fonts-dejavu-core imagemagick x11-utils xdotool \
    && rm -rf /var/lib/apt/lists/*
