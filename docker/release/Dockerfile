ARG DISTRO
ARG VERSION="latest"

FROM ubuntu:${VERSION} as ubuntu_builder

RUN apt-get update && \
    apt-get install --no-install-recommends -y apt-transport-https

# Install dependencies
RUN apt-get install --no-install-recommends -y coreutils gcc make libc6-dev libx11-dev libxmu-dev libxcb-randr0-dev libpng-dev libssl-dev libunistring-dev && apt-get clean -y

FROM fedora:41 as fedora_builder

# Install dependencies
RUN dnf install --setopt=install_weak_deps=False -y coreutils gcc make glibc-devel libX11-devel libXmu-devel libpng-devel openssl-devel libunistring-devel

FROM archlinux:base as arch_builder

# Install dependencies
RUN pacman -Sy && \
    pacman -S --needed --noconfirm coreutils gcc make glibc libx11 libxmu libpng openssl libunistring

# hadolint ignore=DL3006
FROM ${DISTRO}_builder

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

COPY --chmod=755 ./docker/release/copy_release.sh ./

CMD ["./copy_release.sh"]
