#!/bin/bash

IFS=$'\n\t'
set -xeou pipefail

build_src=build-src
BUILD_IMG=capd_deb_img
PKG_DIR=$(dirname "$0")

TAG=$(cat "$PKG_DIR"/../VERSION)

CAPD_DEB=capd_22.2.17-1_amd64.deb
SRC_TAR=build/meson-dist/capd-${TAG}.tar.xz

meson ${build_src}
meson dist -C ${build_src} --include-subprojects
PKG_NAME=capd_22.2.17
SRC_TAR=capd-${TAG}.tar.xz
cp "${build_src}/meson-dist/capd-${TAG}.tar.xz" .

cd "$PKG_DIR"/..

# Build
export DOCKER_BUILDKIT=0
docker build \
       -f "$PKG_DIR"/Dockerfile-deb \
       -t ${BUILD_IMG} \
       --build-arg SRC_TAR="${SRC_TAR}" \
       --build-arg PKG_NAME="${PKG_NAME}" \
       .

# Copy .deb to host/
docker run --rm -v "$PWD":/host --user "$( id -u ):$( id -g )" ${BUILD_IMG} /bin/sh -c "cp ${CAPD_DEB} /host"

# Test
docker run --rm -v "$PWD":/host ubuntu:18.04 /bin/sh -c "apt update; apt install -y /host/${CAPD_DEB}; dpkg -L capd"
