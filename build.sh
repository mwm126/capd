#!/bin/bash

# gcc -L/usr/local/opt/openssl/lib  -I/usr/local/opt/openssl/include -lcrypto -O3 -Wall -o capd capd.c
# gcc -lssl -lcrypto -O3 -Wall -o capd capd.c
gcc -w -lrt -lcrypto -O3 -Wall -o capd capd.c /usr/lib/x86_64-linux-gnu/libcrypto.a -ldl -lpthread
