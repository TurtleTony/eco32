//
// Created by tony on 29.06.21.
//

#include "globalSymbolTable.h"


khash_t(globalSymbolTable) *gst;

void initGst(void) {
    gst = kh_init(globalSymbolTable);
}


Symbol *putSymbolIntoGst(Symbol *symbol) {
#ifdef DEBUG
    debugPrintf("      - %-16s 0x%08X", symbol->name, symbol->val);
#endif
    int ret;
    khiter_t k;
    // Put symbol into table as key
    k = kh_put(globalSymbolTable, gst, symbol->name, &ret);
    // Get pointer to value
    Symbol *tableEntry = kh_value(gst, k);

    // Check whether key already existed or not
    switch (ret) {
        // Symbol already in gst
        case 0:
            if ((symbol->attr & SYM_ATTR_U) == 0) {
                // Symbol is defined
                if ((tableEntry->attr & SYM_ATTR_U) == 0) {
                    // Gst entry is also defined
                    warning("Skipping symbol '%s'; is already defined", symbol->name);
                }
                // Override library table entry with module symbol

                tableEntry->seg = symbol->seg;
                tableEntry->val = symbol->val;
                tableEntry->attr = symbol->attr;
            }
            break;
            // Symbol not yet in bucket; put into gst
        case 1:
            kh_value(gst, k) = symbol;
            break;
        default:
            error("Error writing symbol %s into gst", symbol->name);
    }

    return kh_value(gst, k);
}


Symbol *getSymbolFromGst(char *symbolName) {
    khint_t k = kh_get(globalSymbolTable, gst, symbolName);
    return kh_value(gst, k);
}


void checkUndefinedSymbols(void) {
    Symbol *entry;

    unsigned int undefined = 0;
    kh_foreach_value(gst, entry, {
        if ((entry->attr & SYM_ATTR_U) != 0) {
            undefined++;
            warning("Undefined symbol '%s' in gst after name resolution", entry->name);
        }
    });

    if (undefined) {
        error("%d undefined symbol(s) in gst", undefined);
    }
}
