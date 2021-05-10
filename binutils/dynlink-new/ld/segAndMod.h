//
// Created by roo on 01.05.21.
//

#ifndef ECO32_SEGANDMOD_H
#define ECO32_SEGANDMOD_H

#define ATTR_APX    (SEG_ATTR_A | SEG_ATTR_P | SEG_ATTR_X)
#define ATTR_AP     (SEG_ATTR_A | SEG_ATTR_P)
#define ATTR_APW    (SEG_ATTR_A | SEG_ATTR_P | SEG_ATTR_W)
#define ATTR_AW     (SEG_ATTR_A | SEG_ATTR_W)


/**************************************************************/
/** Eof structs **/

typedef struct module {
    char *name;			/* module name */
    unsigned char *data;		/* data space */
    char *strs;			/* string space */
    int nsegs;			/* number of segments */
    struct segment *segs;		/* array of segments */
    int nsyms;			/* number of symbols */
    struct symbol **syms;		/* array of pointer to gst entries */
    int nrels;			/* number of relocations */
    struct reloc *rels;		/* array of relocations */
} Module;


typedef struct segment {
    char *name;			/* segment name */
    unsigned char *data;		/* segment data */
    unsigned int addr;		/* virtual start address */
    unsigned int size;		/* size of segment in bytes */
    unsigned int attr;		/* segment attributes */
} Segment;


typedef struct symbol {
    char *name;			/* symbol name */
    int val;			/* the symbol's value */
    Module *mod;        /* Module where symbol is located */
    int seg;			/* the symbol's segment, -1: absolute */
    unsigned int attr;		/* symbol attributes */
} Symbol;

typedef struct reloc {
    unsigned int loc;		/* where to relocate */
    int seg;			/* in which segment */
    int typ;			/* relocation type: one of RELOC_xxx */
    /* symbol flag RELOC_SYM may be set */
    int ref;			/* what is referenced */
    /* if symbol flag = 0: segment number */
    /* if symbol flag = 1: symbol number */
    int add;			/* additive part of value */
} Reloc;


/**************************************************************/
/** Segmentation structs **/


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


extern SegmentGroup apxGroup;
extern SegmentGroup apGroup;
extern SegmentGroup apwGroup;
extern SegmentGroup awGroup;

#endif //ECO32_SEGANDMOD_H
