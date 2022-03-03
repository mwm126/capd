#!/bin/bash

IFS=$'\n\t'
set -xeou pipefail

DOCKER_BUILDKIT=0
build_src=build-src
CAPD_RPM=capd-1-1.x86_64.rpm
BUILD_IMG=capd_rpm_img
PKG_DIR=$(dirname "$0")

TAG=$(cat "$PKG_DIR"/../VERSION)

SRC_TAR=build/meson-dist/capd-${TAG}.tar.xz

meson ${build_src}
meson dist -C ${build_src}

# Build
docker build -f "$PKG_DIR"/Dockerfile-rpm -t ${BUILD_IMG} --build-arg SRC_TAR=${SRC_TAR} .

# Copy .rpm to dist/
docker run --rm -v "$PWD"/dist:/dist ${BUILD_IMG} /bin/sh -c "cp rpmbuild/RPMS/x86_64/${CAPD_RPM} /dist"

# Test
docker run --rm -v "$PWD"/dist:/dist ubuntu:18.04 /bin/sh -c "rpm -i /dist/${CAPD_RPM}; rpm -qL capd"
