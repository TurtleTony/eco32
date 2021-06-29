//
// Created by tony on 29.06.21.
//

#ifndef ECO32_GLOBALSYMBOLTABLE_H
#define ECO32_GLOBALSYMBOLTABLE_H


#include "khash.h"
#include "load.h"

typedef struct symbol {
    char *name;			/* symbol name */
    int val;			/* the symbol's value */
    int seg;			/* the symbol's segment, -1: absolute */
    unsigned int attr;		/* symbol attributes */
} Symbol;

KHASH_MAP_INIT_STR(globalSymbolTable, Symbol *);

void initGst(void);
void putSymbolIntoGst(Symbol *symbol);
Symbol *getSymbolFromGst(char *symbol);
void checkUndefinedSymbols(void);
void resolveSymbolValues(void);

#endif //ECO32_GLOBALSYMBOLTABLE_H
