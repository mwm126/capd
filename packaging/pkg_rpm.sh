#!/bin/bash

IFS=$'\n\t'
set -xeou pipefail

build_src=build-src
BUILD_IMG=capd_rpm_img
PKG_DIR=$(dirname "$0")

TAG=$(cat "$PKG_DIR"/../VERSION)

CAPD_RPM=capd-${TAG}-1.x86_64.rpm
SRC_TAR=build/meson-dist/capd-${TAG}.tar.xz

rm -rf ${build_src}
meson ${build_src}
meson dist -C ${build_src}

cd "$PKG_DIR"/..

# Build
env DOCKER_BUILDKIT=1 docker build -f "$PKG_DIR"/Dockerfile-rpm -t ${BUILD_IMG} --build-arg SRC_TAR="${SRC_TAR}" .

# Copy .rpm to host/
docker run --rm -v "$PWD":/host ${BUILD_IMG} /bin/sh -c "cp /root/rpmbuild/RPMS/x86_64/${CAPD_RPM} /host"

# Test
docker run --rm -v "$PWD":/host amazonlinux:2022 /bin/sh -c "rpm -i /host/${CAPD_RPM}; rpm -ql capd"