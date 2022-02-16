#!/bin/bash

gcc -std=gnu99 -Wall -lrt -O3 -o capd *.c -lcrypto -DCAPD_VERSION=$(cat VERSION)
