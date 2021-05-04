//
// Created by tony on 04.05.21.
//

#include "symbolResolution.h"


khash_t(globalSymbolTable) *gst;

void initGst(void) {
    gst = kh_init(globalSymbolTable);
}


void printMapFile(char *fileName) {
    FILE *file;
    file = fopen(fileName, "w");
    if (file == NULL) {
        error("cannot open map file '%s'", fileName);
    }

    Symbol *entry;
    kh_foreach_value(gst, entry, {
        fprintf(file,
                "%-24s  0x%08X  %-12s  %s\n",
                entry->name,
                entry->val,
                entry->seg == -1 ?
                "*ABS*" : entry->mod->segs[entry->seg].name,
                entry->mod->name
        );
    })
}


void putSymbolIntoGst(Symbol *moduleSymbol, unsigned int symbolNumber) {
    int ret;
    khiter_t k;
    // Put symbol into table as key
    k = kh_put(globalSymbolTable, gst, moduleSymbol->name, &ret);
    // Get pointer to value
    Symbol *tableEntry = kh_value(gst, k);

    // Check whether key already existed or not
    switch (ret) {
        // Symbol already in gst
        case 0:
            if ((moduleSymbol->attr & SYM_ATTR_U) == 0) {
                // Module symbol is defined
                if ((tableEntry->attr & SYM_ATTR_U) == 0) {
                    // Gst entry is also defined
                    error("symbol '%s' in module '%s' defined more than once\n"
                          "       (previous definition is in module '%s')",
                          moduleSymbol->name, moduleSymbol->mod->name, tableEntry->mod->name);
                }
                tableEntry->mod = moduleSymbol->mod;
                tableEntry->seg = moduleSymbol->seg;
                tableEntry->val = moduleSymbol->val;
                tableEntry->attr = moduleSymbol->attr;
            }
            break;
            // Symbol not yet in bucket; put into gst
        case 1:
            kh_value(gst, k) = moduleSymbol;
            break;
        default:
            error("Error writing symbol %s into gst", moduleSymbol->name);
    }

    moduleSymbol->mod->syms[symbolNumber] = tableEntry;
}