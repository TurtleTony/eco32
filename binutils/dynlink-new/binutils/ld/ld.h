//
// Created by tony on 20.04.21.
//

#ifndef ECO32_LD_H
#define ECO32_LD_H


#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "symbolResolution.h"
#include "globalOffsetTable.h"

#include "fileHandler.h"
#include "storageAllocation.h"
#include "relocation.h"

#define DEFAULT_OUT_FILE_NAME   "a.out"
#define DEFAULT_CODE_BASE	    0x1000

#define PAGE_ALIGN(x)		    (((x) + 0x0FFF) & ~0x0FFF)

#define DEFAULT_START_SYMBOL	"_start"
#define DEFAULT_END_SYMBOL      "_end"

int picMode = 0;

/**************************************************************/
/** Helper methods **/


void alignToWord(unsigned int *addr);
void printUsage(char *arg0);
void error(char *fmt, ...);
void warning(char *fmt, ...);
void *safeAlloc(unsigned int size);
void safeFree(void *p);
#ifdef DEBUG
void debugPrintf(char *fmt, ...);
#endif

#endif //ECO32_LD_H
