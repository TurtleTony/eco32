#
# Makefile for application development
#

CC = gcc
#CFLAGS = -g -Wall
CFLAGS = -Wall

all:		test1

        run:		test1
./app

        app:		test1.o
        $(CC) $(CFLAGS) -L../lib -Wl,-rpath=../lib -o test1 test1.o -lmylib

        app.o:		app.c
        $(CC) $(CFLAGS) -I../include -o test1.o -c test1.c

        clean:
rm -f *~ test1.o test1
