/*
 * ld.c -- ECO32 link editor
 * Created by tony on 20.04.21.
 */

#include "ld.h"

int main(int argc, char *argv[]) {

    char *infile = NULL;
    char *outfile = DEFAULT_OUT_FILE_NAME;
    int pageAlignData = 1;
    unsigned int codeBaseAddress = DEFAULT_CODE_BASE;

    int c;
    char *endPtr;
    // Get-opt keeps this easily expandable for the future
    while ((c = getopt(argc, argv, "dc:o:?")) != -1) {
        switch (c) {
            case 'd':
                pageAlignData = 0;
                break;
            case 'c':
                codeBaseAddress = strtoul(optarg, &endPtr, 0);
                if (*endPtr != '\0') {
                    error("option '-c' has an invalid address");
                }
                break;
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

    int fileCount = argc - optind;
    if (fileCount == 0) {
        // Require at least one infile
        printUsage(argv[0]);
        return 1;
    }

    for (int i = 0; i < fileCount; i++) {
        Module *mod = readModule(argv[optind]);
        allocateModuleStorage(mod, codeBaseAddress, pageAlignData);
    }


    Module *executableModule = newModule("main");
    writeModule(executableModule, outfile);
    return 0;
}


/**************************************************************/
/** HELPER */
/**************************************************************/


void alignAddress(unsigned int *addr) {
    while(*addr % 4) {
        (*addr)++;
    }
}


void printUsage(char *arg0) {
    fprintf(stderr, "usage: %s\n", arg0);
    fprintf(stderr, "         [-d]             data directly after code\n");
    fprintf(stderr, "         [-c <addr>]      set code base address\n");
    fprintf(stderr, "         [-o <outfile>]   set output file name\n");
    fprintf(stderr, "         <infile> ...     input file names\n");
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


void safeFree(void *p) {
    if (p == NULL) {
        error("tried to free NULL pointer");
    }
    free(p);
}
