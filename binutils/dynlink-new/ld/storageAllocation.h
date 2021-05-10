//
// Created by tony on 27.04.21.
//

#ifndef ECO32_STORAGEALLOCATION_H
#define ECO32_STORAGEALLOCATION_H


#include "fileHandler.h"
#include "eof.h"
#include "ld.h"
#include "segAndMod.h"


/**************************************************************/
/** Phase I **/


void addModuleSegmentsToGroups(struct module *mod);
void addTotalToGroup(TotalSegment *totalSegment, SegmentGroup *segmentGroup);
void addPartialToGroup(PartialSegment *partialSegment, SegmentGroup *segmentGroup);
void addPartialToTotal(PartialSegment *partialSegment, TotalSegment *totalSegment);


/**************************************************************/
/** Phase II **/


void allocateStorage(unsigned int codeBase, int dataPageAlign, char *endSymbolName);
unsigned int setTotalAddress(SegmentGroup *segmentGroup, unsigned int currentAddress);

#endif //ECO32_STORAGEALLOCATION_H
