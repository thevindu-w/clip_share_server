ARG DISTRO

FROM ubuntu:noble AS ubuntu_builder

# Install build dependencies
RUN apt-get update && apt-get install --no-install-recommends -y gcc make libc6-dev libx11-dev libxmu-dev libxcb-randr0-dev libpng-dev libssl-dev libunistring-dev

# Install test dependencies
RUN apt-get install --no-install-recommends -y openssl xclip python3-minimal diffutils findutils coreutils socat sed

RUN mkdir -p /debs
WORKDIR /debs
RUN apt-get download xvfb libunwind8 libgl1 libpixman-1-0 libxfont2 libglvnd0 libglx0 libfontenc1 libfreetype6 libbrotli1 xkb-data libxkbfile1 x11-xkb-utils
RUN dpkg --force-depends -i ./*.deb 2>/dev/null && apt-get clean -y

FROM fedora:41 AS fedora_builder

# Install build dependencies
RUN dnf install --setopt=install_weak_deps=False -y gcc make glibc-devel libX11-devel libXmu-devel libpng-devel openssl-devel libunistring-devel

# Install test dependencies
RUN dnf install --setopt=install_weak_deps=False -y xorg-x11-server-Xvfb openssl xclip python3 findutils diffutils coreutils socat sed && dnf clean all

FROM archlinux:base AS arch_builder

# Install build dependencies
RUN pacman -Sy && \
    pacman -S --needed --noconfirm gcc make glibc libx11 libxmu libpng openssl libunistring

# Install test dependencies
RUN pacman -S --needed --noconfirm coreutils findutils diffutils python socat xclip sed
RUN pacman -Sdd --needed --noconfirm pixman freetype2 graphite harfbuzz libfontenc xorg-xkbcomp xorg-setxkbmap xkeyboard-config libxfont2 libunwind libglvnd libxkbfile xorg-server-xvfb

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
RUN make clean && make && make no_ssl && make web

# Prepare the test environment initialization script
COPY --chmod=755 ./docker/run_docker.sh docker/
COPY --chown=user:user ./tests tests

CMD ["docker/run_docker.sh"]
