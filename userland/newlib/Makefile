TOOLCHAIN_PREFIX = riscv64-unknown-elf
TARGET = newlib.a

CC = $(TOOLCHAIN_PREFIX)-gcc
AR = $(TOOLCHAIN_PREFIX)-ar

SOURCELIST = \
	$(wildcard \
		src/*.[c|S] \
	)
OBJECTLIST = \
	$(filter %.o, $(patsubst \
		%.c, \
		%.o, \
		$(SOURCELIST) \
	)) \
	$(filter %.o, $(patsubst \
		%.S, \
		%.o, \
		$(SOURCELIST) \
	))

CFLAGS = \
	-Iinclude \
	-std=c99 \
	-c \
	--pedantic-errors \
	-Wall \
	-O2

ARFLAGS = rcs

all: $(TARGET)

$(TARGET): $(OBJECTLIST)
	$(AR) $(ARFLAGS) $(TARGET) $(OBJECTLIST)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTLIST) $(TARGET)

