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

void printUsage(char *arg0);
void error(char *fmt, ...);
void *safeAlloc(unsigned int size);
void safeFree(void *p);


#endif //ECO32_LD_H
