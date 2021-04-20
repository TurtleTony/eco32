//
// Created by tony on 20.04.21.
//

#ifndef ECO32_LD_H
#define ECO32_LD_H


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>

#include "eof.h"

#define DEFAULT_OUT_FILE_NAME "a.out"

typedef struct module {
    char *name;			/* module name */
    unsigned char *data;		/* data space */
    char *strs;			/* string space */
    int nsegs;			/* number of segments */
    struct segment *segs;		/* array of segments */
    int nsyms;			/* number of symbols */
    struct symbol *syms;		/* array of symbols */
    int nrels;			/* number of relocations */
    struct reloc *rels;		/* array of relocations */
} Module;

typedef struct segment {
    char *name;			/* segment name */
    unsigned char *data;		/* segment data */
    unsigned int addr;		/* virtual start address */
    unsigned int size;		/* size of segment in bytes */
    unsigned int attr;		/* segment attributes */
    /* used for output only */
    unsigned int nameOffs;	/* offset in string space */
    unsigned int dataOffs;	/* offset in segment data */
} Segment;


typedef struct symbol {
    char *name;			/* symbol name */
    int val;			/* the symbol's value */
    int seg;			/* the symbol's segment, -1: absolute */
    unsigned int attr;		/* symbol attributes */
    /* used for output only */
    unsigned int nameOffs;	/* offset in string space */
} Symbol;


typedef struct reloc {
    unsigned int loc;		/* where to relocate */
    int seg;			/* in which segment */
    int typ;			/* relocation type: one of RELOC_xxx */
    /* symbol flag RELOC_SYM may be set */
    int ref;			/* what is referenced */
    /* if symbol flag = 0: segment number */
    /* if symbol flag = 1: symbol number */
    int add;			/* additive part of value */
} Reloc;


/**************************************************************/


Module *readModule(char *infile);
Module *newModule(char *name);
void writeModule(Module *module, char *outfile);


/**************************************************************/


void printUsage(char *arg0);
void error(char *fmt, ...);
void *safeAlloc(unsigned int size);
void *safeFree(void *p);


/**************************************************************/


uint32_t read4FromEco(unsigned char *p);
void write4ToEco(unsigned char *p, uint32_t data);
void conv4FromEcoToNative(unsigned char *p);
void conv4FromNativeToEco(unsigned char *p);

#endif //ECO32_LD_H
