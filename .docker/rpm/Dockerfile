FROM amazonlinux:2022

LABEL maintainer="Mark Meredith <mark.meredith@netl.doe.gov>"

RUN yum -y update \
    && yum install -y \
    gcc \
    meson \
    openssl-devel \
    rpm-build \
    rpmlint \
    systemd \
    && yum clean all

ARG TAG

WORKDIR /root

RUN mkdir -p rpmbuild/SOURCES
COPY build-src/meson-dist/capd-${TAG}.tar.xz ./rpmbuild/SOURCES
COPY ./ ./

RUN env SRC_TAR="${SRC_TAR}" rpmbuild -v -ba debian/capd.spec

CMD [ "/bin/bash" ]
