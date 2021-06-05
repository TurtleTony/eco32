//
// Created by tony on 04.05.21.
//

#ifndef ECO32_GLOBALOFFSETTABLE_H
#define ECO32_SYMBOLRESOLUTION_H


#include "khash.h"
#include "segAndMod.h"
KHASH_MAP_INIT_INT64(globalOffsetTable, unsigned int);

#include "ld.h"

void initGot(void);
void putEntryIntoGot(Symbol *symbol);
unsigned int getEntryFromGot(Symbol *symbol);


#endif //ECO32_GLOBALOFFSETTABLE_H
