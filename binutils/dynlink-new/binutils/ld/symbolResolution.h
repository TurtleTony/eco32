//
// Created by tony on 04.05.21.
//

#ifndef ECO32_SYMBOLRESOLUTION_H
#define ECO32_SYMBOLRESOLUTION_H


#define SYM_ATTR_X  0x100   /* link unit extern symbol */

#include "khash.h"
#include "segAndMod.h"
KHASH_MAP_INIT_STR(globalSymbolTable, Symbol *);

#include "ld.h"

void initGst(void);
void printMapFile(char *fileName);
void putSymbolIntoGst(Symbol *symbol, unsigned int symbolNumber);
Symbol *getSymbolFromGst(char *symbol);
void checkUndefinedSymbols(void);
void resolveSymbolValues(void);
khash_t(globalSymbolTable) *getGst();

#endif //ECO32_SYMBOLRESOLUTION_H
