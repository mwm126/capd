#!/usr/bin/bash

IFS=$'\n\t'
set -eou pipefail

TOPDIR=$(dirname "$0")
TAG=$(cat "$TOPDIR"/VERSION)

VERSION=${TAG:1}

CAPD_DEB=capd_${VERSION}-1_amd64.deb
CAPD_RPM=capd-${TAG}-1.x86_64.rpm
CAPD_DEB_IMG=cap_deb_img
CAPD_RPM_IMG=cap_rpm_img

DOCKER_DPKG_DIR="$TOPDIR"/.docker/deb
DOCKER_RPM_DIR="$TOPDIR"/.docker/rpm

GH_TITLE="capd-${VERSION}"
GH_NOTES="Build RPM and DPKG"

build_src=build-src

SRC_TAR=${build_src}/meson-dist/capd-${TAG}.tar.xz
export DOCKER_BUILDKIT=1

function usage() {
    echo "Usage:"
    echo "       ./build.sh             display usage"
    echo
    echo "       ./build.sh deb         to build capd-*.deb"
    echo "       ./build.sh rpm         to build capd-*.rpm"
    echo
    echo "       ./build.sh deb test    to test capd-*.deb"
    echo "       ./build.sh rpm test    to test capd-*.rpm"
    echo
    echo "       ./build.sh deb upload  to upload capd-*.deb"
    echo "       ./build.sh rpm upload  to upload capd-*.rpm"
    exit 0
}

function main() {

    if [ $# -eq 0 ]; then
        usage
    fi

    cd "$TOPDIR"

    if [ "$1" == "deb" ]; then
        meson "${build_src}"
        meson dist -C "${build_src}" --include-subprojects
        build_dpkg
        echo "*** Successful dpkg build ***"
        test_deb
        echo "*** Successful dpkg test ***"
        exit 0
    fi

    if [ "$1" == "rpm" ]; then
        meson "${build_src}"
        meson dist -C "${build_src}" --include-subprojects
        build_rpm

        test_rpm

        docker run --rm -v "$PWD":/host amazonlinux:2022 \
               /bin/sh -c "rpm -i /host/${CAPD_RPM}; rpm -ql capd"
        exit 0
    fi

    usage

    if [ "${2:-}" == "upload" ]; then
        gh release upload "${TAG}" "${CAPD_RPM}" "${CAPD_DEB}"
    else
        echo
        echo "Build successful. Run again with 'upload' to upload assets to Github."
    fi
}

function build_dpkg() {
    CAPD_PKG="${CAPD_DEB}"
    CAPD_PKG_PATH="/workdir/${CAPD_DEB}"
    docker build \
           -t ${CAPD_DEB_IMG} \
           --build-arg SRC_TAR="${SRC_TAR}" \
           --build-arg TAG="${TAG}" \
           -f "$DOCKER_DPKG_DIR/Dockerfile" .

    docker run --rm -v "$PWD":/host ${CAPD_DEB_IMG} \
           /bin/sh -c "cp ${CAPD_PKG_PATH} /host"
    # workaround root:root permissions of CAPD_PKG
    rm -f tmp.pkg
    cp "${CAPD_PKG}" tmp.pkg
    rm -f "${CAPD_PKG}"
    mv tmp.pkg "${CAPD_PKG}"
}

function build_rpm() {
    CAPD_PKG="${CAPD_RPM}"
    DOCKER_IMG="${CAPD_RPM_IMG}"
    CAPD_PKG_PATH="/root/rpmbuild/RPMS/x86_64/${CAPD_RPM}"
    docker build \
           -t ${CAPD_RPM_IMG} \
           --build-arg SRC_TAR="${SRC_TAR}" \
           --build-arg TAG="${TAG}" \
           -f "$DOCKER_RPM_DIR/Dockerfile" .

    docker run --rm -v "$PWD":/host ${DOCKER_IMG} \
           /bin/sh -c "cp ${CAPD_PKG_PATH} /host"
    # workaround root:root permissions of CAPD_PKG
    rm -f tmp.pkg
    cp "${CAPD_PKG}" tmp.pkg
    rm -f "${CAPD_PKG}"
    mv tmp.pkg "${CAPD_PKG}"
}

function test_deb() {
    TEST_IMG=dpkg-test
    docker build \
           -t ${TEST_IMG} \
           --build-arg CAPD_DEB="${CAPD_DEB}" \
           -f "$DOCKER_DPKG_DIR/Dockerfile-test" .
    echo "Successful install of dpkg"

    NAME=test_container
    rm cidfile
    docker run --cidfile cidfile \
           --detach \
           -p 127.0.0.1:6666:62201/tcp \
           "${TEST_IMG}" \
           /bin/sh -c "/usr/bin/capd -v 2"

    echo "This is my data" > /dev/udp/127.0.0.1/6666
    sleep 1
    docker exec "$(cat cidfile)" /bin/sh -c "cat /var/log/capd.log"
    docker kill "$(cat cidfile)"
    echo "Successful test of dpkg"
}

main "$@"
