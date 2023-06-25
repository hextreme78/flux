TOOLCHAIN_PREFIX = riscv64-unknown-linux-gnu

CC = $(TOOLCHAIN_PREFIX)-gcc
CFLAGS = -fno-pie -no-pie -static -nostdlib

all:
	$(CC) $(CFLAGS) test1.c -o test1
	$(CC) $(CFLAGS) test2.c -o test2

clean:
	rm test1 test2
