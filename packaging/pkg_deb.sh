#!/bin/bash

IFS=$'\n\t'
set -xeou pipefail

build_src=build-src
BUILD_IMG=capd_deb_img
PKG_DIR=$(dirname "$0")

TAG=$(cat "$PKG_DIR"/../VERSION)

CAPD_DEB=capd_inst.deb
SRC_TAR=build/meson-dist/capd-${TAG}.tar.xz

rm -rf ${build_src}
meson ${build_src}
meson dist -C ${build_src}

cd "$PKG_DIR"/..

# Build
env DOCKER_BUILDKIT=1 docker build -f "$PKG_DIR"/Dockerfile-deb -t ${BUILD_IMG} --build-arg SRC_TAR="${SRC_TAR}" .

# Copy .deb to host/
docker run --rm -v "$PWD":/host --user "$( id -u ):$( id -g )" ${BUILD_IMG} /bin/sh -c "cp /${CAPD_DEB} /host"

# Test
docker run --rm -v "$PWD":/host ubuntu:18.04 /bin/sh -c "dpkg -i /host/${CAPD_DEB}; dpkg -L capd"
