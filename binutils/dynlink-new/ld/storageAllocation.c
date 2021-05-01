//
// Created by tony on 27.04.21.
//

#include "storageAllocation.h"

/**************************************************************/
/** PHASE I **/
/**************************************************************/


void addModuleSegmentsToGroups(Module *mod) {
    for(int i = 0; i < mod->nsegs; i++) {
        Segment *seg = &mod->segs[i];
        PartialSegment *partial = safeAlloc(sizeof(PartialSegment));
        partial->mod = mod;
        partial->seg = seg;

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
    total = safeAlloc(sizeof(TotalSegment));

    total->name = partialSegment->seg->name;
    total->nameOffs = 1;
    total->dataOffs = 2;
    total->addr = 3;
    total->size = 4;
    total->attr = segmentGroup->attr;
    total->firstPart = NULL;
    total->lastPart = NULL;
    total->next = NULL;

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


/**************************************************************/
/** PHASE II **/
/**************************************************************/


void allocateStorage(unsigned int codeBase, int dataPageAlign) {
    unsigned int currentAddress = codeBase;
    currentAddress = setTotalAddress(&apxGroup, currentAddress);

    currentAddress = setTotalAddress(&apxGroup, currentAddress);
    currentAddress = setTotalAddress(&apGroup, currentAddress);

    if (dataPageAlign) {
        currentAddress = PAGE_ALIGN(currentAddress);
    }

    currentAddress = setTotalAddress(&apwGroup, currentAddress);
    currentAddress = setTotalAddress(&awGroup, currentAddress);
}


unsigned int setTotalAddress(SegmentGroup *segmentGroup, unsigned int currentAddress) {
    TotalSegment *total = segmentGroup->firstTotal;

    while(total != NULL) {
        total->addr = currentAddress;

        unsigned int totalSize = 0;
        PartialSegment *partial = total->firstPart;

        while (partial != NULL) {
            partial->seg->addr = currentAddress;
            unsigned int size = partial->seg->size;
            alignToWord(&size);

            currentAddress += size;
            totalSize += size;

            partial = partial->next;
        }

        total->size = totalSize;
        total = total->next;
    }

    return currentAddress;
}




