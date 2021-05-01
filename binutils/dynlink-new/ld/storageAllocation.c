//
// Created by tony on 27.04.21.
//
#include "storageAllocation.h"
#include "eof.h"


SegmentGroup apxGroup = {ATTR_APX, NULL, NULL};
SegmentGroup apGroup = {ATTR_AP, NULL, NULL};
SegmentGroup apwGroup = {ATTR_APW, NULL, NULL};
SegmentGroup awGroup = {ATTR_AW, NULL, NULL};

void allocateModuleStorage(Module *mod, unsigned int codeBase, int dataPageAlign) {
    // Pass 1: Build module segments into segment groups
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

    // Pass 2: Compute addresses and sizes
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
