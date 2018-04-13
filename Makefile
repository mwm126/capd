capd: capd.c
	gcc -w -lrt -lcrypto -O3 -Wall -o capd capd.c /usr/lib/x86_64-linux-gnu/libcrypto.a -ldl -lpthread

test: test.c
	gcc -w -lrt -lcrypto -O3 -Wall -o test test.c /usr/lib/x86_64-linux-gnu/libcrypto.a -ldl -lpthread
