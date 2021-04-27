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

#include "eofHandler.h"

#define DEFAULT_OUT_FILE_NAME   "a.out"
#define DEFAULT_CODE_BASE	    0x1000


/**************************************************************/
/** Helper methods **/


void printUsage(char *arg0);
void error(char *fmt, ...);
void *safeAlloc(unsigned int size);
void safeFree(void *p);


#endif //ECO32_LD_H
