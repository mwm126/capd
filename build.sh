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

GH_TITLE="capd-${VERSION}"
GH_NOTES="Build RPM and DPKG"

build_src=build-src

SRC_TAR=${build_src}/meson-dist/capd-${TAG}.tar.xz
export DOCKER_BUILDKIT=1


function main() {
    if [ "$1" != "pkg" ]; then
        echo "Usage:"
        echo "       ./build.sh             to build plain capd binary"
        echo "       ./build.sh pkg         to build capd-*.deb and capd-*.rpm"
        echo
        echo "       ./build.sh pkg upload  to upload *.deb,*.rpm as Github Release artifacts"
        exit 0
    fi

    cd "$TOPDIR"
    meson "${build_src}"
    meson dist -C "${build_src}" --include-subprojects

    CAPD_DEB=capd_${VERSION}-1_amd64.deb
    CAPD_PKG="${CAPD_DEB}"
    DOCKER_IMG=capd_deb_img
    DOCKER_DIR="$TOPDIR"/.docker/deb
    CAPD_PKG_PATH="/workdir/${CAPD_DEB}"
    build_pkg
    docker run --rm -v "$PWD":/host ubuntu:18.04 \
           /bin/sh -c "apt update; apt install -y /host/${CAPD_DEB}; dpkg -L capd"

    CAPD_RPM=capd-${TAG}-1.x86_64.rpm
    CAPD_PKG="${CAPD_RPM}"
    DOCKER_IMG=capd_rpm_img
    DOCKER_DIR="$TOPDIR"/.docker/rpm
    CAPD_PKG_PATH="/root/rpmbuild/RPMS/x86_64/${CAPD_RPM}"
    build_pkg
    docker run --rm -v "$PWD":/host amazonlinux:2022 \
           /bin/sh -c "rpm -i /host/${CAPD_RPM}; rpm -ql capd"

    if [ "${2:-}" == "upload" ]; then
        gh release create "${TAG}" \
           -n "${GH_NOTES}" -t "${GH_TITLE}" \
           "${CAPD_RPM}" "${CAPD_DEB}"

    else
        echo
        echo "Build successful. Run again with 'upload' to upload assets to Github."
    fi
}


function build_pkg() {
    docker build \
           -t ${DOCKER_IMG} \
           --build-arg SRC_TAR="${SRC_TAR}" \
           --build-arg TAG="${TAG}" \
           -f "$DOCKER_DIR/Dockerfile" .

    docker run --rm -v "$PWD":/host ${DOCKER_IMG} \
           /bin/sh -c "cp ${CAPD_PKG_PATH} /host"
    # workaround root:root permissions of CAPD_PKG
    rm -f tmp.pkg
    cp "${CAPD_PKG}" tmp.pkg
    rm -f "${CAPD_PKG}"
    mv tmp.pkg "${CAPD_PKG}"
}


main "$@"
