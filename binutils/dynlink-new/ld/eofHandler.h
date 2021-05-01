//
// Created by tony on 27.04.21.
//

#ifndef ECO32_EOFHANDLER_H
#define ECO32_EOFHANDLER_H


#include <stdio.h>

#include "eof.h"

#include "ecoEndian.h"
#include "ld.h"
#include "storageAllocation.h"

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
/** Module methods **/


Module *newModule(char *name);
Module *readModule(char *inputPath);
void writeExecutable(char *outputPath, unsigned int codeEntry);


/**************************************************************/
/** Parsing input object file **/


void parseHeader(EofHeader *hdr, FILE *inputFile, char *inputPath);
void parseData(Module *module, unsigned int odata, unsigned int sdata, FILE *inputFile, char *inputPath);
void parseStrings(Module *module, unsigned int ostrs, unsigned int sstrs, FILE *inputFile, char *inputPath);

void parseSegments(Module *module, unsigned int osegs, unsigned int nsegs, FILE *inputFile, char *inputPath);
void parseSymbols(Module *module, unsigned int osyms, unsigned int nsyms, FILE *inputFile, char *inputPath);
void parseRelocations(Module *module, unsigned int orels, unsigned int nrels, FILE *inputFile, char *inputPath);


/**************************************************************/
/** Writing output object file **/


void writeDummyHeader(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath);

void writeData(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath);
void writeDataTotal(EofHeader *outFileHeader, TotalSegment *totalSeg, FILE *outputFile, char *outputPath);

void writeStrings(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath);
void writeStringsTotal(EofHeader *outFileHeader, TotalSegment *totalSeg, FILE *outputFile, char *outputPath);

void writeSegments(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath);
void writeSegmentsTotal(EofHeader *outFileHeader, TotalSegment *totalSeg, FILE *outputFile, char *outputPath);

void writeSymbols(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath);
void writeRelocations(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath);

void writeFinalHeader(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath);


#endif //ECO32_EOFHANDLER_H
