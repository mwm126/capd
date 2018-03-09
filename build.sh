#!/bin/bash

gcc -L/usr/local/opt/openssl/lib  -I/usr/local/opt/openssl/include -lcrypto -O3 -Wall -o capd capd.c
