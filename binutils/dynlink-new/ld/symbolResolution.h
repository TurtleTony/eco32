//
// Created by tony on 04.05.21.
//

#ifndef ECO32_SYMBOLRESOLUTION_H
#define ECO32_SYMBOLRESOLUTION_H


#include "khash.h"
#include "segAndMod.h"
KHASH_MAP_INIT_STR(globalSymbolTable, Symbol *);

#include "ld.h"

khash_t(globalSymbolTable) *initGst();
void printMapFile(char *fileName, khash_t(globalSymbolTable) *gst);
void symbolNameResolution(khash_t(globalSymbolTable) *gst, Symbol *moduleSymbol, unsigned int symbolNumber);


#endif //ECO32_SYMBOLRESOLUTION_H
