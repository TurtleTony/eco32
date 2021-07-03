//
// Created by tony on 29.05.21.
//

#ifndef ECO32_RELOCATION_H
#define ECO32_RELOCATION_H


#include "segAndMod.h"
#include "fileHandler.h"


typedef struct loadTimeW32 {
    unsigned int loc;   /* where to relocate */
    int sym;            /* 1 = symbol, 0 = segment */
    union {
        Symbol *symbol;
        Segment *segment;
    } entry;
} LoadTimeW32;

extern int w32Count;
extern LoadTimeW32 *loadTimeW32s;
void relocate(Segment *gotSegment);


#endif //ECO32_RELOCATION_H
