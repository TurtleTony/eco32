/*
 * load.c -- ECO32 loader
 * Created by tony on 27.06.21.
 */

#include "load.h"


int main(int argc, char *argv[]) {
    unsigned int ldOff = 0;
    unsigned int memorySizeMB = DEFAULT_MEMSIZE;

    char *execFileName = NULL;
    char *binFileName = NULL;

    int c, option_index;
    char *endPtr;
#ifdef DEBUG
    debugPrintf("Parsing command-line options");
#endif
    static struct option long_options[] = {
    };

    // Get-opt keeps this easily expandable for the future
    while ((c = getopt_long_only(argc, argv, "l:m:?", long_options, &option_index)) != -1) {
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
            case 'l':
                ldOff = strtoul(optarg, &endPtr, 0);
                if (*endPtr != '\0') {
                    error("option '-l' has illegal load offset");
                }
                break;
            case 'm':
                memorySizeMB = strtoul(optarg, &endPtr, 0);
                if (*endPtr != '\0' ||
                    memorySizeMB <= 0 ||
                    memorySizeMB > MAX_MEMSIZE) {
                    error("option '-m' has illegal memory size");
                }
                break;
            case '?':
                printUsage(argv[0]);
                return 1;
            default:
                return 1;
        }
    }

    int fileCount = argc - optind;
    if (fileCount != 2) {
        // Require exactly two file names

        printUsage(argv[0]);
        return 1;
    }

    execFileName = argv[optind];
    binFileName = argv[optind + 1];

#ifdef DEBUG
    debugPrintf("Initializing %u MB of pseudo-randomized memory", memorySizeMB);
#endif
    unsigned int memorySizeBytes = memorySizeMB << 20;

    memory = safeAlloc(memorySizeBytes);
    for (int i = 0; i < memorySizeBytes; i++) {
        memory[i] = rand();
    }

    loadExecutable(execFileName, ldOff);


//    writeBinary(binFileName);
    return 0;
}


void loadExecutable(char *execFileName, unsigned int ldOff) {
#ifdef DEBUG
    debugPrintf("Loading executable '%s' with offset '0x%08X'", execFileName, ldOff);
#endif
    FILE *execFile;

}


void printUsage(char *myself) {
    printf("usage: %s [options] <executable> <binary>\n", myself);
    printf("valid options are:\n");
    printf("    -l <n>         load with n bytes offset\n");
    printf("    -m <n>         set maximum memory size to <n> MB\n");
    exit(1);
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

