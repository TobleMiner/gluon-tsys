CP?=cp -r
LN?=ln -s
FPIC?=-fPIC

all: librespondd

librespondd:
	$(CC) -c ${CFLAGS} $(FPIC) -o librespondd.o librespondd.c -Wall
	$(CC) -shared $(FPIC) -Wl,-soname,librespondd.so.0 -o librespondd.so.0 librespondd.o -lc

install:
	@echo Running install target
	$(CP) librespondd.h /usr/include/librespondd-0
	$(CP) librespondd.so.0 /usr/lib
	$(LN) /usr/lib/respondd.so.0 /usr/lib/respondd.so

clean:
	rm librespondd.so.0 respondd.o
