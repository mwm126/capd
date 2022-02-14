#!/bin/bash

gcc -Wall -Wno-unused-result -lrt -O3 -o capd *.c -lcrypto
