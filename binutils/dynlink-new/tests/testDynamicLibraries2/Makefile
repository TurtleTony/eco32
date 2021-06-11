#
# Makefile for using a dynamic library
#

all:
		$(MAKE) -C libdev all
		$(MAKE) -C appdev all

run:		all
		$(MAKE) -C testdev run

clean:
		$(MAKE) -C testdev clean
		$(MAKE) -C libdev clean
		rm -rf include
		rm -rf lib
		rm -f *~
