//
// Created by tony on 27.04.21.
//

#ifndef ECO32_FILEHANDLER_H
#define ECO32_FILEHANDLER_H


#include <stdio.h>

#include "eof.h"
#include "ar.h"

#include "ecoEndian.h"
#include "ld.h"
#include "segAndMod.h"


/**************************************************************/
/** Parsing input object file **/


void readFile(char *inputPath);
void parseObjectFile(char *moduleName, unsigned int fileOffset, FILE *objectFile, char *inputPath);
void parseArchiveFile(FILE *archiveFile, char *inputPath);

void parseArchiveHeader(ArchHeader *hdr, FILE *inputFile, char *inputPath);
char *parseArchiveStrings(unsigned int ostrs, unsigned int sstrs, FILE *inputFile, char *inputPath);
ModuleRecord *parseArchiveModules(unsigned int omods, unsigned int nmods, FILE *inputFile, char *inputPath);
int moduleNeeded(ModuleRecord *moduleRecord, char *strs);

void parseEofHeader(EofHeader *hdr, unsigned int fileOffset, FILE *inputFile, char *inputPath);
void parseData(Module *module, unsigned int odata, unsigned int sdata, FILE *inputFile, char *inputPath);
void parseStrings(Module *module, unsigned int ostrs, unsigned int sstrs, FILE *inputFile, char *inputPath);

void parseSegments(Module *module, unsigned int osegs, unsigned int nsegs, FILE *inputFile, char *inputPath);
void parseSymbols(Module *module, unsigned int osyms, unsigned int nsyms, FILE *inputFile, char *inputPath);
void parseRelocations(Module *module, unsigned int orels, unsigned int nrels, FILE *inputFile, char *inputPath);

Module *firstModule();

/**************************************************************/
/** Writing output object file **/


void writeExecutable(char *outputPath, char *startSymbolName);

void writeDummyHeader(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath);

void writeData(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath);
void writeDataTotal(EofHeader *outFileHeader, TotalSegment *totalSeg, FILE *outputFile, char *outputPath);

void writeStrings(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath);
void writeStringsTotal(EofHeader *outFileHeader, TotalSegment *totalSeg, FILE *outputFile, char *outputPath);

void writeSegments(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath);
void writeSegmentsTotal(EofHeader *outFileHeader, TotalSegment *totalSeg, FILE *outputFile, char *outputPath);

void writeFinalHeader(unsigned int codeEntry, EofHeader *outFileHeader, FILE *outputFile, char *outputPath);


#endif //ECO32_FILEHANDLER_H
