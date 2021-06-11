#
# Makefile for object file I/O
#

BUILD=../../build

all:
		$(BUILD)/bin/as -pic -o test1.o test1.s
		$(BUILD)/bin/dof -a test1.o >test1.dump
		$(BUILD)/bin/ld -l -m test2.map -c 0xC0000000 -o test2.dso test1.o
		$(BUILD)/bin/dof -s test2.dso >test2.dump

clean:
		rm -f *~ test1.o test1.dump
		rm -f test2.dso test2.dump test2.map
