#!/usr/bin/bash

IFS=$'\n\t'
set -xeou pipefail

build_src=build-src
BUILD_DEB_IMG=capd_deb_img
BUILD_RPM_IMG=capd_rpm_img
PKG_DIR=$(dirname "$0")

TAG=$(cat "$PKG_DIR"/VERSION)

CAPD_DEB=capd_22.2.17-1_amd64.deb
CAPD_RPM=capd-${TAG}-1.x86_64.rpm
SRC_TAR=build/meson-dist/capd-${TAG}.tar.xz

function build_src() {
    meson ${build_src}
    meson dist -C ${build_src} --include-subprojects
    PKG_NAME=capd_22.2.17
    SRC_TAR=capd-${TAG}.tar.xz
    cp "${build_src}/meson-dist/capd-${TAG}.tar.xz" .
}


function build_rpm() {

    # Build
    export DOCKER_BUILDKIT=0
    docker build -f "$PKG_DIR"/Dockerfile-rpm -t ${BUILD_RPM_IMG} --build-arg SRC_TAR="${SRC_TAR}" .

    # Copy .rpm to host/
    docker run --rm -v "$PWD":/host ${BUILD_RPM_IMG} /bin/sh -c "cp /root/rpmbuild/RPMS/x86_64/${CAPD_RPM} /host"

    # Test
    docker run --rm -v "$PWD":/host amazonlinux:2022 /bin/sh -c "rpm -i /host/${CAPD_RPM}; rpm -ql capd"
}

function build_deb() {

    # Build
    export DOCKER_BUILDKIT=0
    docker build \
           -f "$PKG_DIR"/Dockerfile-deb \
           -t ${BUILD_DEB_IMG} \
           --build-arg SRC_TAR="${SRC_TAR}" \
           --build-arg PKG_NAME="${PKG_NAME}" \
           .

    # Copy .deb to host/
    docker run --rm -v "$PWD":/host --user "$( id -u ):$( id -g )" ${BUILD_DEB_IMG} /bin/sh -c "cp ../${CAPD_DEB} /host"

    # Test
    docker run --rm -v "$PWD":/host ubuntu:18.04 /bin/sh -c "apt update; apt install -y /host/${CAPD_DEB}; dpkg -L capd"
};

if [ "$1" == "deb" ]; then
    cd "$PKG_DIR"
    build_src
    build_deb
elif [ "$1" == "rpm" ]; then
    cd "$PKG_DIR"
    build_src
    build_rpm
else
    echo "Usage: ./pkg.sh rpm      ot ./pkg.sh deb"

fi
