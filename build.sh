#!/usr/bin/bash

IFS=$'\n\t'
set -eou pipefail

TOPDIR=$(dirname "$0")
TAG=$(cat "$TOPDIR"/VERSION)

if [ $# -eq 0 ]; then
    gcc -o capd src/*.c \
        -lcrypto -lrt -std=c2x -Wall -O3 -D_DEFAULT_SOURCE -DCAPD_VERSION="$TAG"
    echo "Built:  capd"
    exit 0
fi

VERSION=${TAG:1}

GH_TITLE="CAP daemon Release ${TAG}"
GH_NOTES="Build RPM and DPKG"

build_src=build-src

SRC_TAR=${build_src}/meson-dist/capd-${TAG}.tar.xz
export DOCKER_BUILDKIT=1


function main() {
    if [ "$1" == "deb" ]; then
        CAPD_DEB=capd_${VERSION}-1_amd64.deb
        CAPD_PKG="${CAPD_DEB}"
        DOCKER_IMG=capd_deb_img
        DOCKER_DIR="$TOPDIR"/.docker/deb
        CAPD_PKG_PATH="/workdir/${CAPD_DEB}"
    elif [ "$1" == "rpm" ]; then
        CAPD_RPM=capd-${TAG}-1.x86_64.rpm
        CAPD_PKG="${CAPD_RPM}"
        DOCKER_IMG=capd_rpm_img
        DOCKER_DIR="$TOPDIR"/.docker/rpm
        CAPD_PKG_PATH="/root/rpmbuild/RPMS/x86_64/${CAPD_RPM}"
    else
        echo "Usage:"
        echo "       ./build.sh       to build plain capd binary"
        echo "       ./build.sh deb   to build capd-*.deb"
        echo "       ./build.sh rpm   to build capd-*.rpm"
        echo
        echo "       ./build.sh deb upload  to upload DEB as Github Release artifact"
        echo "       ./build.sh rpm upload  to upload RPM as Github Release artifact"
        exit 0
    fi

    cd "$TOPDIR"
    build_pkg
    test_pkg "$1"

    if [ "${2:-}" == "upload" ]; then
        github_upload "${CAPD_PKG}"
    else
        echo
        echo "Build successful. Run again with 'upload' to upload assets to Github."
    fi
}


function build_pkg() {
    meson "${build_src}"
    meson dist -C "${build_src}" --include-subprojects

    docker build \
           -t ${DOCKER_IMG} \
           --build-arg SRC_TAR="${SRC_TAR}" \
           --build-arg TAG="${TAG}" \
           -f "$DOCKER_DIR/Dockerfile" .

    docker run --rm -v "$PWD":/host ${DOCKER_IMG} \
           /bin/sh -c "cp ${CAPD_PKG_PATH} /host"
}


function test_pkg {
    if [ "$1" == "deb" ]; then
        docker run --rm -v "$PWD":/host ubuntu:18.04 \
               /bin/sh -c "apt update; apt install -y /host/${CAPD_DEB}; dpkg -L capd"

    elif [ "$1" == "rpm" ]; then
        docker run --rm -v "$PWD":/host amazonlinux:2022 \
               /bin/sh -c "rpm -i /host/${CAPD_RPM}; rpm -ql capd"
    fi
}


function github_upload {
    gh release create "${TAG}" \
       --draft \
       -n "${GH_NOTES}" -t "${GH_TITLE}" \
       "$1"
}


main "$@"
