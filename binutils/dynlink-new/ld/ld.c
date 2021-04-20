/*
 * ld.c -- ECO32 link editor
 * Created by tony on 20.04.21.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>

#include "eof.h"
#include "ld.h"

int main(int argc, char *argv[]) {

    char *infile = NULL;
    char *outfile = DEFAULT_OUT_FILE_NAME;

    Module *mod;

    int c;
    // Get-opt keeps this easily expandable for the future
    while ((c = getopt(argc, argv, "o:?")) != -1) {
        switch (c) {
            case 'o':
                outfile = optarg;
                break;
            case '?':
                printUsage(argv[0]);
                return 1;
            default:
                return 1;
        }
    }

    if (optind >= argc) {
        // Require an additional option for the infile
        printUsage(argv[0]);
        return 1;
    }

    infile = argv[optind];

    mod = readModule(infile);
    writeModule(mod, outfile);
    return 0;
}


/**************************************************************/
/** HELPER */
/**************************************************************/


void printUsage(char *arg0) {
    printf("usage: %s [-o <outfile>] <infile>\n", arg0);
}

void error(char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "!\n");
    va_end(ap);
    exit(1);
}

void *safeAlloc(unsigned int size) {
    void *p;

    p = malloc(size);
    if (p == NULL) {
        error("unable to allocate %d bytes", size);
    }
    return p;
}


void *safeFree(void *p) {
    if (p == NULL) {
        error("tried to free NULL pointer");
    }
    free(p);
}


/**************************************************************/


Module *newModule(char *name) {
    Module *mod;

    mod = safeAlloc(sizeof(Module));
    mod->name = name;
    mod->data = NULL;
    mod->strs = NULL;
    mod->nsegs = 0;
    mod->segs = NULL;
    mod->nsyms = 0;
    mod->syms = NULL;
    mod->nrels = 0;
    mod->rels = NULL;
    return mod;
}


/**************************************************************/


Module *readModule(char *infile) {
    FILE *inFile;
    EofHeader hdr;
    Module *mod;

    inFile = fopen(infile, "r");
    if (inFile == NULL) {
        error("cannot open input file '%s'", infile);
    }
    return NULL;
}

void writeModule(Module *module, char *outfile) {

}


/**************************************************************/
/** CONVERSION METHODS */
/**************************************************************/

uint32_t read4FromEco(unsigned char *p) {
    return (uint32_t) p[0] << 24 |
           (uint32_t) p[1] << 16 |
           (uint32_t) p[2] <<  8 |
           (uint32_t) p[3] <<  0;
}


void write4ToEco(unsigned char *p, uint32_t data) {
    p[0] = data >> 24;
    p[1] = data >> 16;
    p[2] = data >>  8;
    p[3] = data >>  0;
}


void conv4FromEcoToNative(unsigned char *p) {
    uint32_t data;

    data = read4FromEco(p);
    * (uint32_t *) p = data;
}


void conv4FromNativeToEco(unsigned char *p) {
    uint32_t data;

    data = * (uint32_t *) p;
    write4ToEco(p, data);
}


/**************************************************************/

