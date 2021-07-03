//
// Created by tony on 04.05.21.
//

#include "globalOffsetTable.h"

static int gotEntries = 0;

khash_t(globalOffsetTable) *got;

void initGot(void) {
    got = kh_init(globalOffsetTable);
}


void putEntryIntoGot(Symbol *symbol) {
    int ret;
    khiter_t k;
    // Put symbol into table as key
    k = kh_put(globalOffsetTable, got, (u_int64_t) symbol, &ret);

    if (ret == 1) {
        // New entry
        kh_value(got, k) = gotEntries++ * 4;
    } else if (ret == -1) {
        error("Error writing symbol %s into got", symbol->name);
    }
}


unsigned int getOffsetFromGot(Symbol *symbol) {
    khint_t k = kh_get(globalOffsetTable, got, (u_int64_t) symbol);
    return kh_value(got, k);
}


unsigned int gotSize() {
    return gotEntries * 4;
}


khash_t(globalOffsetTable) *getGot() {
    return got;
}