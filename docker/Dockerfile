ARG DISTRO='ubuntu'

FROM ubuntu:resolute AS ubuntu_builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install --no-install-recommends -y gcc make pkgconf libc6-dev libx11-dev libxmu-dev libxcb-randr0-dev libpng-dev libssl-dev libunistring-dev

# Install test dependencies
RUN apt-get install --no-install-recommends -y openssl xclip python3-minimal diffutils findutils coreutils socat sed

RUN mkdir -p /debs
WORKDIR /debs
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
RUN apt-get download xvfb libnettle8t64 libunwind8 libgl1 libpixman-1-0 libxfont2 libglvnd0 libglx0 libfontenc1 libfreetype6 libbrotli1 xkb-data libxkbfile1 x11-xkb-utils shared-mime-info
RUN apt-get install --no-install-recommends -y -s libgtk-3-dev libayatana-appindicator3-dev | grep "^Inst" | awk '{print $2}' | grep "\-dev$" | sort -u | xargs apt-get download
RUN ls -1 *.deb | xargs -I {} dpkg -x {} / && rm -f *.deb && if [ -d /lib ] && [ -z "$(ls -A /lib)" ]; then rmdir /lib && ln -s /usr/lib /lib; fi && apt-get install -f -y && apt-get clean -y

FROM fedora:43 AS fedora_builder

# Install build dependencies
RUN dnf install --setopt=install_weak_deps=False -y gcc make glibc-devel libX11-devel libXmu-devel libpng-devel openssl-devel libunistring-devel gtk3-devel libayatana-appindicator-gtk3-devel

# Install test dependencies
RUN dnf install --setopt=install_weak_deps=False -y xorg-x11-server-Xvfb openssl xclip python3 findutils diffutils coreutils socat sed && dnf clean all

FROM archlinux:base AS arch_builder

# Install build dependencies
RUN pacman -Sy && \
    pacman -S --needed --noconfirm gcc make pkgconf glibc libx11 libxmu libpng openssl libunistring gtk3 libayatana-appindicator

# Install test dependencies
RUN pacman -S --needed --noconfirm coreutils findutils diffutils python socat xclip sed
RUN pacman -Sdd --needed --noconfirm pixman freetype2 graphite harfbuzz libfontenc xorg-xkbcomp xorg-setxkbmap xkeyboard-config libxfont2 libunwind libglvnd libxkbfile xorg-server-xvfb

FROM debian:bullseye-slim AS debian_builder
ARG NO_XVFB=''

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install --no-install-recommends -y gcc make libc6-dev libx11-dev libxmu-dev libxcb-randr0-dev libpng-dev libssl-dev libunistring-dev libgtk-3-dev libayatana-appindicator3-dev

# Install test dependencies
RUN apt-get install --no-install-recommends -y openssl xclip python3-minimal diffutils findutils coreutils socat sed
RUN if [ -z "$NO_XVFB" ]; then apt-get install --no-install-recommends -y xvfb; fi

# hadolint ignore=DL3006
FROM ${DISTRO}_builder

RUN useradd -mU -s /bin/bash user
USER user

# Set environment variables for tests
ENV TERM=xterm-256color
ENV DISPLAY=:0
ENV HOME=/home/user

RUN mkdir -p "$HOME/src"
WORKDIR "$HOME/src"

# Copy the source files
COPY Makefile ./
COPY ./src src

# Build
RUN make clean && make && make no_ssl

# Prepare the test environment initialization script
COPY --chmod=755 ./docker/run_docker.sh docker/
COPY --chown=user:user ./tests tests

CMD ["docker/run_docker.sh"]
