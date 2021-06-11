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
    });

    fclose(file);
}


void putSymbolIntoGst(Symbol *moduleSymbol, unsigned int symbolNumber) {
#ifdef DEBUG
    debugPrintf("      Putting symbol '%s' into GST", moduleSymbol->name);
#endif
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

    moduleSymbol->mod->syms[symbolNumber] = kh_value(gst, k);
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


void resolveSymbolValues(void) {
    Symbol *entry;
#ifdef DEBUG
    Module *module;
    Segment *segment;
    int value;
#endif

    kh_foreach_value(gst, entry, {
#ifdef DEBUG
    module = entry->mod;
    segment = entry->seg != -1 ? &entry->mod->segs[entry->seg] : NULL;
    value = entry->val;
#endif
        // only change non-absolute symbols
        if (entry->seg != -1) {
            // Add segment base address to symbol value
            entry->val += entry->mod->segs[entry->seg].addr;
#ifdef DEBUG
            debugPrintf("  Resolving symbol '%s' (%s, %s) from 0x%08X to 0x%08X",
                        entry->name, module->name, segment->name, value, entry->val);
#endif
        }
#ifdef DEBUG
        else {
            debugPrintf("  Skipping absolute symbol '%s' (%s)", entry->name, module->name);
        }
#endif
    });
}

khash_t(globalSymbolTable) *getGst() {
    return gst;
}