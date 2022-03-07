FROM ubuntu:18.04

LABEL maintainer="Mark Meredith <mark.meredith@netl.doe.gov>"

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get -qq update \
    && apt-get -qq -y install --no-install-recommends \
    build-essential=12.4ubuntu1 \
    cdbs=0.* \
    debhelper=11.* \
    devscripts=2.* \
    dh-make=2.* \
    dh-systemd=11.*  \
    equivs=2.* \
    fakeroot=1.* \
    libssl-dev=1.* \
    pkg-config=0.29.* \
    python3-pip=9.* \
    python3.8-venv=3.8.* \
    && apt-get clean && rm -rf /var/lib/apt/lists/* \
    && python3.8 -m pip install --no-cache-dir \
    meson==0.* \
    ninja==1.*

WORKDIR /workdir

ARG TAG
ARG SRC_TAR

COPY ${SRC_TAR} .

RUN tar xf $(basename ${SRC_TAR})
WORKDIR /workdir/capd-${TAG}
RUN dpkg-buildpackage -us -uc -b

CMD [ "/bin/bash" ]
