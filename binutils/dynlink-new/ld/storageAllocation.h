//
// Created by tony on 27.04.21.
//

#ifndef ECO32_STORAGEALLOCATION_H
#define ECO32_STORAGEALLOCATION_H


#include "eofHandler.h"
#include "eof.h"
#include "ld.h"

#define ATTR_APX    (SEG_ATTR_A | SEG_ATTR_P | SEG_ATTR_X)
#define ATTR_AP     (SEG_ATTR_A | SEG_ATTR_P)
#define ATTR_APW    (SEG_ATTR_A | SEG_ATTR_P | SEG_ATTR_W)
#define ATTR_AW     (SEG_ATTR_A | SEG_ATTR_W)


typedef struct partialSegment {
    Module *mod;				/* module where part comes from */
    Segment *seg;				/* which segment in the module */
    unsigned int npad;		/* number of padding bytes */
    struct partialSegment *next;		/* next part in total segment */
} PartialSegment;


typedef struct totalSegment {
    char *name;	            			/* name of total segment */
    unsigned int nameOffs;		        /* name offset in string space */
    unsigned int dataOffs;	        	/* data offset in data space */
    unsigned int addr;		        	/* virtual address */
    unsigned int size;		        	/* size in bytes */
    unsigned int attr;		        	/* attributes */
    struct partialSegment *firstPart;	/* first partial segment in total */
    struct partialSegment *lastPart;	/* last partial segment in total */
    struct totalSegment *next;  		/* next total in segment group */
} TotalSegment;


typedef struct segmentGroup {
    unsigned int attr;			    /* attributes of this group */
    TotalSegment *firstTotal;		/* first total segment in group */
    TotalSegment *lastTotal;		/* last total segment in group */
} SegmentGroup;

SegmentGroup apxGroup = {ATTR_APX, NULL, NULL};
SegmentGroup apGroup = {ATTR_AP, NULL, NULL};
SegmentGroup apwGroup = {ATTR_APW, NULL, NULL};
SegmentGroup awGroup = {ATTR_AW, NULL, NULL};


/**************************************************************/
/** Phase I **/


void addModuleSegmentsToGroups(Module *mod);
void addTotalToGroup(TotalSegment *totalSegment, SegmentGroup *segmentGroup);
void addPartialToGroup(PartialSegment *partialSegment, SegmentGroup *segmentGroup);
void addPartialToTotal(PartialSegment *partialSegment, TotalSegment *totalSegment);


/**************************************************************/
/** Phase II **/


void allocateStorage(unsigned int codeBase, int dataPageAlign);
unsigned int setTotalAddress(SegmentGroup *segmentGroup, unsigned int currentAddress);

#endif //ECO32_STORAGEALLOCATION_H
