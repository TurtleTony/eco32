//
// Created by tony on 27.06.21.
//

#ifndef ECO32_LOAD_H
#define ECO32_LOAD_H


#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>

#include "eof.h"
#include "ecoEndian.h"
#include "queue.h"
#include "globalSymbolTable.h"

#define MAX_MEMSIZE		    512	    /* maximum memory size in MB */
#define DEFAULT_MEMSIZE		4	    /* default memory size in MB */
#define DEFAULT_LIB_PATH	"."    /* default folder to lookup libraries */

#define PAGE_ALIGN(x)		    (((x) + 0x0FFF) & ~0x0FFF)


extern unsigned char *memory;

typedef struct linkUnit {
    char *name;
    char *strs;
    struct symbol **symbols;
    struct linkUnit *next;
} LinkUnit;

typedef struct reloc {
    unsigned int loc;		/* where to relocate */
    struct symbol *symbol;         /* Symbol values to be placed @ loc */
    struct reloc *next;
} Reloc;


void loadExecutable(char *execFileName, unsigned int ldOff);
void loadLinkUnit(char *name, unsigned int expectedMagic, FILE *inputFile, char *inputPath, unsigned int ldOff);
void loadLibrary(char *name, unsigned int ldOff);

void parseEofHeader(EofHeader *hdr, unsigned int expectedMagic, FILE *inputFile, char *inputPath);
void parseStrings(char **strs, unsigned int ostrs, unsigned int sstrs, FILE *inputFile, char *inputPath);
void parseSegment(SegmentRecord *segmentRecord, unsigned int osegs, unsigned int nsegs, FILE *inputFile, char *inputPath, char *strs);
void parseSymbols(LinkUnit *linkUnit, unsigned int osyms, unsigned int nsyms, FILE *inputFile, char *inputPath,
                  char *strs, unsigned int ldOff);
void parseRelocation(RelocRecord *relocRecord, unsigned int orels, unsigned int relno, FILE *inputFile, char *inputPath);

LinkUnit *newLinkUnit(char *name);
Reloc *newReloc();

void writeBinary(char *fileName);

char *basename(char *path);

void printUsage(char *arg0);
void error(char *fmt, ...);
void warning(char *fmt, ...);
void *safeAlloc(unsigned int size);
void safeFree(void *p);
#ifdef DEBUG
void debugPrintf(char *fmt, ...);
#endif


#endif //ECO32_LOAD_H
