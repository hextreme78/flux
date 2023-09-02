TOOLCHAIN_PREFIX = riscv64-unknown-elf

CC = $(TOOLCHAIN_PREFIX)-gcc

all: ../bin/test2

../bin/test2:
	$(CC) -std=c99 -Wall test2.c -o ../bin/test2 -L../newlib -l:newlib.a -static

clean:
	rm ../bin/test2
