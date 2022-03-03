#!/bin/bash

IFS=$'\n\t'
set -xeou pipefail

DOCKER_BUILDKIT=0
build_src=build-src
CAPD_DEB=capd_inst.deb
BUILD_IMG=capd_deb_img
PKG_DIR=$(dirname "$0")

TAG=$(cat "$PKG_DIR"/../VERSION)

SRC_TAR=build/meson-dist/capd-${TAG}.tar.xz

meson ${build_src}
meson dist -C ${build_src}

cd "$PKG_DIR"/..

# Build
docker build -f "$PKG_DIR"/Dockerfile-deb -t ${BUILD_IMG} --build-arg SRC_TAR=${SRC_TAR} .

# Copy .deb to dist/
docker run --rm -v "$PWD"/dist:/dist ${BUILD_IMG} /bin/sh -c "cp ${CAPD_DEB} /dist"

# Test
docker run --rm -v "$PWD"/dist:/dist ubuntu:18.04 /bin/sh -c "dpkg -i /dist/${CAPD_DEB}; dpkg -L capd"
