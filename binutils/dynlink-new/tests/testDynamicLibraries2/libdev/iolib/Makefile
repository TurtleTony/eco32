#
# Makefile for library development
#

BUILD = ../../../../build

CC = $(BUILD)/bin/lcc
CFLAGS = -A -I../biolib -pic
LD = $(BUILD)/bin/ld

all:		types.h stdarg.h iolib.h libiolib.dso
		mkdir -p ../../include
		cp types.h ../../include
		cp stdarg.h ../../include
		cp iolib.h ../../include
		mkdir -p ../../lib
		cp libiolib.dso ../../lib

libiolib.dso:	iolib.o
		$(LD) -o libiolib.dso -l iolib.o

iolib.o:	iolib.c iolib.h
		$(CC) $(CFLAGS) -o iolib.o -c iolib.c

clean:
		rm -f *~ iolib.o libiolib.dso
