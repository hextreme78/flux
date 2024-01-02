TARGET = test2
TOOLCHAIN_PREFIX = riscv64-flux

CC = $(TOOLCHAIN_PREFIX)-gcc
CFLAGS = \
	 -Wall \
	 -std=c99

all:
	$(CC) $(CFLAGS) $(TARGET).c -o ../bin/$(TARGET)

clean:
	rm ../bin/$(TARGET)

