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

    // Check whether key already existed or not
    switch (ret) {
        // Symbol already in got
        case 0:
        // Symbol not yet in bucket; put into got
        case 1:
            kh_value(got, k) = 0; //TODO: placeholder
            // TODO: increment size etc.
            gotEntries++;
            break;
        default:
            error("Error writing symbol %s into got", symbol->name);
    }
}


unsigned int getEntryFromGot(Symbol *symbol) {
    khint_t k = kh_get(globalOffsetTable, got, (u_int64_t) symbol);
    return kh_value(got, k);
}
