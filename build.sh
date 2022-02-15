#!/bin/bash

gcc -std=gnu99 -Wall -Wno-unused-result -lrt -O3 -o capd *.c -lcrypto -DCAPD_VERSION=$(cat VERSION)
