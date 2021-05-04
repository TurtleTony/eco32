//
// Created by tony on 27.04.21.
//

#ifndef ECO32_EOFHANDLER_H
#define ECO32_EOFHANDLER_H


#include <stdio.h>

#include "eof.h"

#include "ecoEndian.h"
#include "ld.h"
#include "segAndMod.h"
#include "khash.h"


/**************************************************************/
/** Parsing input object file **/


Module *readModule(char *inputPath, khash_t(globalSymbolTable) *gst);

void parseHeader(EofHeader *hdr, FILE *inputFile, char *inputPath);
void parseData(Module *module, unsigned int odata, unsigned int sdata, FILE *inputFile, char *inputPath);
void parseStrings(Module *module, unsigned int ostrs, unsigned int sstrs, FILE *inputFile, char *inputPath);

void parseSegments(Module *module, unsigned int osegs, unsigned int nsegs, FILE *inputFile, char *inputPath);
void parseSymbols(Module *module, unsigned int osyms, unsigned int nsyms, khash_t(globalSymbolTable) *gst, FILE *inputFile, char *inputPath);
void parseRelocations(Module *module, unsigned int orels, unsigned int nrels, FILE *inputFile, char *inputPath);


/**************************************************************/
/** Writing output object file **/


void writeExecutable(char *outputPath, unsigned int codeEntry);

void writeDummyHeader(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath);

void writeData(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath);
void writeDataTotal(EofHeader *outFileHeader, TotalSegment *totalSeg, FILE *outputFile, char *outputPath);

void writeStrings(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath);
void writeStringsTotal(EofHeader *outFileHeader, TotalSegment *totalSeg, FILE *outputFile, char *outputPath);

void writeSegments(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath);
void writeSegmentsTotal(EofHeader *outFileHeader, TotalSegment *totalSeg, FILE *outputFile, char *outputPath);

void writeFinalHeader(unsigned int codeEntry, EofHeader *outFileHeader, FILE *outputFile, char *outputPath);


#endif //ECO32_EOFHANDLER_H
