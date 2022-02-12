#!/bin/bash

gcc -Wall -Wno-unused-result -lrt -O0 -o capd *.c -lcrypto
