/*
 * ld.c -- ECO32 link editor
 * Created by tony on 20.04.21.
 */

#include "ld.h"


int picMode = 0;
int w32Count = 0;
int createLibrary = 0;

int main(int argc, char *argv[]) {

    char *outfile = DEFAULT_OUT_FILE_NAME;
    int pageAlignData = 1; // Default true
    unsigned int codeBaseAddress = DEFAULT_CODE_BASE;
    char *startSymbol = DEFAULT_START_SYMBOL;
    char *endSymbol = DEFAULT_END_SYMBOL;

    int c, option_index;
    char *endPtr, *mapFileName;
#ifdef DEBUG
    debugPrintf("Parsing command-line options");
#endif
    static struct option long_options[] = {
            {"shared",  no_argument, &createLibrary, 1},
            {"pic",     no_argument, &picMode, 1}
    };

    // Get-opt keeps this easily expandable for the future
    while ((c = getopt_long_only(argc, argv, "dc:s:e:o:m:?", long_options, &option_index)) != -1) {
#ifdef DEBUG
        if (c == 0) {
            // flag option
            if (optarg) {
                debugPrintf("  -%s %s", long_options[option_index].name, optarg);
            } else {
                debugPrintf("  -%s", long_options[option_index].name);
            }
        } else {
            if (optarg) {
                debugPrintf("  -%c %s", c, optarg);
            } else {
                debugPrintf("  -%c", c);
            }
        }
#endif
        switch (c) {
            case 0:
                // set flag option
                break;
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

    if (createLibrary) {
        if (!picMode) {
#ifdef DEBUG
            debugPrintf("Force -pic when -shared is set");
#endif
            picMode = 1;
        }
        // Default codebase to 0 for libraries
        codeBaseAddress = 0;
    }
    int fileCount = argc - optind;
    if (fileCount == 0) {
        // Require at least one infile

        printUsage(argv[0]);
        return 1;
    }

#ifdef DEBUG
    debugPrintf("Initializing hash tables");
#endif
    // Initialize gst hash table
    initGst();

    // Initialize global offset table
    initGot();

#ifdef DEBUG
    debugPrintf("Reading files");
#endif
    // Build module table
    for (int i = 0; i < fileCount; i++) {
        readFile(argv[optind + i]);
    }

    // Storage allocation
    initLinkerModule();
    Module *module = firstModule();

    Segment *gotSegment = NULL;
    // Add got segment first
    if (picMode) {
#ifdef DEBUG
        debugPrintf("Building got segment");
#endif
        gotSegment = buildGotSegment();
#ifdef DEBUG
        debugPrintf("Built got segment with size 0x%08X", gotSegment->size);
#endif
    }

#ifdef DEBUG
    debugPrintf("Building segment groups from modules");
#endif
    // Pass 1: Build module segments into segment groups
    while(module != NULL) {
        addModuleSegmentsToGroups(module);

        module = module->next;
    }

#ifdef DEBUG
    debugPrintf("Allocating storage");
#endif
    // Pass 2: Compute addresses and sizes
    allocateStorage(codeBaseAddress, pageAlignData, endSymbol);

#ifdef DEBUG
    debugPrintf("Checking for undefined symbols");
#endif
    // Symbol value resolution
    checkUndefinedSymbols();
#ifdef DEBUG
    debugPrintf("Resolving symbols");
#endif
    resolveSymbolValues();

    if (mapFileName != NULL) {
#ifdef DEBUG
        debugPrintf("Printing map file to %s", mapFileName);
#endif
        printMapFile(mapFileName);
    }

#ifdef DEBUG
    debugPrintf("Relocate");
#endif
    relocate(gotSegment);

#ifdef DEBUG
    debugPrintf("Write output file '%s'", outfile);
#endif
    if (createLibrary) {
        writeLibrary(outfile, gotSegment);
    } else {
        writeExecutable(outfile, startSymbol, gotSegment);
    }
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
    fprintf(stderr, "         [-shared]        create a library file\n");
    fprintf(stderr, "         [-pic]           create a pic object\n");
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

#ifdef DEBUG
void debugPrintf(char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    printf("\x1b[33m");
    vprintf(fmt, ap);
    printf("\x1b[0m\n");
    va_end(ap);
}
#endif