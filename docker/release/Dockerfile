ARG DISTRO='ubuntu'
ARG VERSION='latest'

FROM ubuntu:${VERSION} AS ubuntu_builder

ARG APPIMAGE='0'
ENV DEBIAN_FRONTEND=noninteractive
# Install dependencies
RUN apt-get update && apt-get install --no-install-recommends -y coreutils gcc make libc6-dev libx11-dev libxmu-dev libxcb-randr0-dev libpng-dev libssl-dev libunistring-dev libgtk-3-dev && \
    if [ "$APPIMAGE" = '1' ]; then apt-get install --no-install-recommends -y ca-certificates wget file; fi && \
    apt-get clean -y

FROM fedora:43 AS fedora_builder

# Install dependencies
RUN dnf install --setopt=install_weak_deps=False -y coreutils gcc make glibc-devel libX11-devel libXmu-devel libpng-devel openssl-devel libunistring-devel gtk3-devel

FROM archlinux:base AS arch_builder

# Install dependencies
RUN pacman -Sy && \
    pacman -S --needed --noconfirm coreutils gcc make pkgconf glibc libx11 libxmu libpng openssl libunistring gtk3

FROM debian:bullseye-slim AS debian_builder

# Install dependencies
RUN apt-get update && apt-get install --no-install-recommends -y coreutils gcc make libc6-dev libx11-dev libxmu-dev libxcb-randr0-dev libpng-dev libssl-dev libunistring-dev libgtk-3-dev && apt-get clean -y

# hadolint ignore=DL3006
FROM ${DISTRO}_builder

RUN if [ "$APPIMAGE" = '1' ]; then wget https://github.com/AppImage/appimagetool/releases/download/1.9.1/appimagetool-x86_64.AppImage && mv appimagetool-x86_64.AppImage /usr/bin/appimagetool && chmod +x /usr/bin/appimagetool; fi

RUN useradd -mU -s /bin/bash user
USER user
ENV HOME=/home/user

RUN mkdir -p "$HOME/src"
WORKDIR "$HOME/src"

# Copy the source files
COPY Makefile ./
COPY ./src src

# Build
RUN make clean && make

COPY --chown=user ./docker/release/AppDir AppDir
COPY --chmod=755 ./docker/release/build_appimage.sh ./
RUN if [ "$APPIMAGE" = '1' ]; then ./build_appimage.sh; fi

COPY --chmod=755 ./docker/release/copy_release.sh ./

CMD ["./copy_release.sh"]
