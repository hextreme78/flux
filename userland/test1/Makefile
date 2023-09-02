TOOLCHAIN_PREFIX = riscv64-unknown-elf

CC = $(TOOLCHAIN_PREFIX)-gcc

all: ../bin/test1

../bin/test1:
	$(CC) -std=c99 -Wall -fno-pie -no-pie -static test1.c -o ../bin/test1 -L../newlib -l:newlib.a

clean:
	rm ../bin/test1
