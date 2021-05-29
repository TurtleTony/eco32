/*
 * ld.c -- ECO32 link editor
 * Created by tony on 20.04.21.
 */

#include "ld.h"
#include "khash.h"
#include <string.h>

int main(int argc, char *argv[]) {

    char *outfile = DEFAULT_OUT_FILE_NAME;
    int pageAlignData = 1; // Default true
    unsigned int codeBaseAddress = DEFAULT_CODE_BASE;
    char *startSymbol = DEFAULT_START_SYMBOL;
    char *endSymbol = DEFAULT_END_SYMBOL;

    int c;
    char *endPtr, *mapFileName;
    // Get-opt keeps this easily expandable for the future
    while ((c = getopt(argc, argv, "dc:s:e:o:m:?")) != -1) {
        switch (c) {
            case 'd':
                pageAlignData = 0;
                break;
            case 'c':
                codeBaseAddress = strtoul(optarg, &endPtr, 0);
                if (*endPtr != '\0') {
                    error("option '-c' has an invalid address");
                }
                unsigned int codeBaseAligned = codeBaseAddress;
                alignToWord(&codeBaseAligned);
                if (codeBaseAddress != codeBaseAligned) {
                    error("code address must be word aligned!");
                }
                break;
            case 'o':
                outfile = optarg;
                break;
            case 'm':
                mapFileName = optarg;
                break;
            case 's':
                startSymbol = optarg;
                break;
            case 'e':
                endSymbol = optarg;
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

    // Initialize gst hash table
    initGst();

    // Build module table
    for (int i = 0; i < fileCount; i++) {
        readFile(argv[optind + i]);
    }

    // Storage allocation
    Module *module = firstModule();

    // Pass 1: Build module segments into segment groups
    while(module->next != NULL) {
        addModuleSegmentsToGroups(module);

        module = module->next;
    }

    // Pass 2: Compute addresses and sizes
    allocateStorage(codeBaseAddress, pageAlignData, endSymbol);

    // Symbol value resolution
    checkUndefinedSymbols();
    symbolValueResolution();

    if (mapFileName != NULL) {
        printMapFile(mapFileName);
    }

    relocateModules();

    writeExecutable(outfile, startSymbol);
    return 0;
}


/**************************************************************/
/** HELPER */
/**************************************************************/


void alignToWord(unsigned int *addr) {
    while(*addr % 4) {
        (*addr)++;
    }
}


void printUsage(char *arg0) {
    fprintf(stderr, "usage: %s\n", arg0);
    fprintf(stderr, "         [-d]             data directly after code\n");
    fprintf(stderr, "         [-c <addr>]      set code base address\n");
    fprintf(stderr, "         [-s <symbol>]    set code start symbol\n");
    fprintf(stderr, "         [-e <symbol>]    set bss end symbol\n");
    fprintf(stderr, "         [-o <outfile>]   set output file name\n");
    fprintf(stderr, "         [-m <mapfile>]   set map file name\n");
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


void warning(char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "warning: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "!\n");
    va_end(ap);
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
