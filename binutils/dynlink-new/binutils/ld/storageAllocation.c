//
// Created by tony on 27.04.21.
//

#include "storageAllocation.h"


SegmentGroup apxGroup = {ATTR_APX, NULL, NULL};
SegmentGroup apGroup = {ATTR_AP, NULL, NULL};
SegmentGroup apwGroup = {ATTR_APW, NULL, NULL};
SegmentGroup awGroup = {ATTR_AW, NULL, NULL};

Segment *newSegment(char *name, unsigned int size, unsigned int attr) {
    Segment *segment = safeAlloc(sizeof(Segment));
    segment->name = name;
    segment->data = safeAlloc(size);
    segment->addr = 0;
    segment->size = size;
    segment->attr = attr;

    return segment;
}


PartialSegment *newPartial(Module *module, Segment *segment) {
    PartialSegment *partial = safeAlloc(sizeof(PartialSegment));
    partial->mod = module;
    partial->seg = segment;
    partial->npad = 0;
    partial->next = NULL;

    return partial;
}


TotalSegment *newTotal(char *name, unsigned int attr) {
    TotalSegment *total = safeAlloc(sizeof(TotalSegment));

    total->name = name;
    total->nameOffs = 0;
    total->dataOffs = 0;
    total->addr = 0;
    total->size = 0;
    total->attr = attr;
    total->firstPart = NULL;
    total->lastPart = NULL;
    total->next = NULL;

    return total;
}

// Initialize a fake "linker module" to associate this partial with
// This is necessary because there is no "real" module for the got
Module *linkerModule;

void initLinkerModule() {
    linkerModule = safeAlloc(sizeof(Module));
    linkerModule->name = "linker";
    linkerModule->data = NULL;
    linkerModule->strs = NULL;
    linkerModule->nsegs = 0;
    linkerModule->segs = NULL;
    linkerModule->nsyms = 1;
    linkerModule->syms = safeAlloc(sizeof(Symbol *));
    linkerModule->nrels = 0;
    linkerModule->rels = NULL;
}
/**************************************************************/
/** PHASE I **/
/**************************************************************/


void addModuleSegmentsToGroups(Module *mod) {
#ifdef DEBUG
    debugPrintf("  Building module '%s'", mod->name);
#endif
    for(int i = 0; i < mod->nsegs; i++) {
        Segment *seg = &mod->segs[i];
#ifdef DEBUG
        debugPrintf("    Building partial segment '%s' in module '%s'", seg->name, mod->name);
#endif
        PartialSegment *partial = newPartial(mod, seg);

        switch (seg->attr) {
            case ATTR_APX:
                addPartialToGroup(partial, &apxGroup);
                break;
            case ATTR_AP:
                addPartialToGroup(partial, &apGroup);
                break;
            case ATTR_APW:
                addPartialToGroup(partial, &apwGroup);
                break;
            case ATTR_AW:
                addPartialToGroup(partial, &awGroup);
                break;
            default:
                error("Unknown segment type '%d'!", seg->attr);
        }
    }
}


void addPartialToGroup(PartialSegment *partialSegment, SegmentGroup *segmentGroup) {
    TotalSegment *total = segmentGroup->firstTotal;

    while (total != NULL) {
        if (strcmp(partialSegment->seg->name, total->name) == 0) {
            // Segment match
            addPartialToTotal(partialSegment, total);
            return;
        }
        total = total->next;
    }

    // If no matching total was found
    total = newTotal(partialSegment->seg->name, segmentGroup->attr);

    addTotalToGroup(total, segmentGroup);

    addPartialToTotal(partialSegment, total);
}


void addPartialToTotal(PartialSegment *partialSegment, TotalSegment *totalSegment) {
    if (totalSegment->firstPart == NULL) {
        totalSegment->firstPart = partialSegment;
    } else {
        totalSegment->lastPart->next = partialSegment;
    }
    totalSegment->lastPart = partialSegment;
}


void addTotalToGroup(TotalSegment *totalSegment, SegmentGroup *segmentGroup) {
    if (segmentGroup->firstTotal == NULL) {
        segmentGroup->firstTotal = totalSegment;
    } else {
        segmentGroup->lastTotal->next = totalSegment;
    }
    segmentGroup->lastTotal = totalSegment;
}

Segment *buildGotSegment(void) {
    Segment *gotSegment = newSegment(".got", gotSize(), ATTR_APW);

    PartialSegment *partial = newPartial(linkerModule, gotSegment);
    addPartialToGroup(partial, &apwGroup);

    return gotSegment;
}


/**************************************************************/
/** PHASE II **/
/**************************************************************/


void allocateStorage(unsigned int codeBase, int dataPageAlign, char *endSymbolName) {
    unsigned int currentAddress = codeBase;

#ifdef DEBUG
    debugPrintf("  Allocating APX group @ 0x%08X", currentAddress);
#endif
    currentAddress = setTotalAddress(&apxGroup, currentAddress);
#ifdef DEBUG
    debugPrintf("  Allocating AP  group @ 0x%08X", currentAddress);
#endif
    currentAddress = setTotalAddress(&apGroup, currentAddress);

    if (dataPageAlign) {
#ifdef DEBUG
        debugPrintf("  Aligning data page from 0x%08X to 0x%08X", currentAddress, PAGE_ALIGN(currentAddress));
#endif
        currentAddress = PAGE_ALIGN(currentAddress);
    }

#ifdef DEBUG
    debugPrintf("  Allocating APW group @ 0x%08X", currentAddress);
#endif
    currentAddress = setTotalAddress(&apwGroup, currentAddress);
#ifdef DEBUG
    debugPrintf("  Allocating AW  group @ 0x%08X", currentAddress);
#endif
    currentAddress = setTotalAddress(&awGroup, currentAddress);

#ifdef DEBUG
    debugPrintf("  Setting endSymbol '%s' to 0x%08X", endSymbolName, currentAddress);
#endif
    // Put final address into global symbol table
    // i.e. the heap needs to know this
    Symbol *endSymbol = safeAlloc(sizeof(Symbol));
    endSymbol->name = endSymbolName;
    endSymbol->val = currentAddress;
    endSymbol->mod = linkerModule;
    endSymbol->seg = -1;
    endSymbol->attr = ~SYM_ATTR_U;

    putSymbolIntoGst(endSymbol, 0);
}


unsigned int setTotalAddress(SegmentGroup *segmentGroup, unsigned int currentAddress) {
    TotalSegment *total = segmentGroup->firstTotal;

    while(total != NULL) {
#ifdef DEBUG
        debugPrintf("    total segment '%s' @ 0x%08X", total->name, currentAddress);
#endif
        total->addr = currentAddress;

        unsigned int totalSize = 0;
        PartialSegment *partial = total->firstPart;

        while (partial != NULL) {
            partial->seg->addr = currentAddress;
            unsigned int size = partial->seg->size;
            alignToWord(&size);

            currentAddress += size;
            totalSize += size;

            partial->npad = size - partial->seg->size;
#ifdef DEBUG
            debugPrintf("      partial segment @ 0x%08X, size: 0x%08X + %u", partial->seg->addr, partial->seg->size, partial->npad);
#endif
            partial = partial->next;
        }

        total->size = totalSize;
        total = total->next;
    }

    return currentAddress;
}
