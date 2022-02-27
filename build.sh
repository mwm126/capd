#!/bin/bash

gcc -o capd src/*.c \
    -lcrypto -lrt -std=c2x -Wall -O3 -D_DEFAULT_SOURCE -DCAPD_VERSION="$(cat VERSION)"
