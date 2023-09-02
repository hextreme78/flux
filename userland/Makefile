MAKE = make

all: newlib/newlib.a \
	bin/test1 \
	bin/test2

newlib/newlib.a:
	$(MAKE) -C newlib

bin/test1:
	$(MAKE) -C test1

bin/test2:
	$(MAKE) -C test2

clean:
	$(MAKE) -C newlib clean
	$(MAKE) -C test1 clean
	$(MAKE) -C test2 clean

