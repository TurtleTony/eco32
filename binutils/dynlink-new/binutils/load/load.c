/*
 * load.c -- ECO32 loader
 * Created by tony on 27.06.21.
 */

#include "load.h"


unsigned char *memory;

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


    execFile = fopen(execFileName, "r");
    if (execFile == NULL) {
        error("cannot open exectuable file '%s'", execFileName);
    }

    char *name = basename(execFileName);
    loadLinkUnit(name, EOF_X_MAGIC, execFile, execFileName, ldOff);
    fclose(execFile);
}

void loadLinkUnit(char *name, unsigned int expectedMagic, FILE *inputFile, char *inputPath, unsigned int ldOff) {
#ifdef DEBUG
    debugPrintf("  Loading link unit '%s' from file '%s'", name, inputPath);
#endif
    EofHeader eofHeader;
    parseEofHeader(&eofHeader, expectedMagic, inputFile, inputPath);

    char *strs;

    parseStrings(&strs, eofHeader.ostrs, eofHeader.sstrs, inputFile, inputPath);

#ifdef DEBUG
    debugPrintf("    Scanning %d libraries specified in link unit '%s'", eofHeader.nlibs, name);
#endif
    char *libStrs = strs + eofHeader.olibs;

    for (int i = 0; i < eofHeader.nlibs; i++) {
        char *libName = libStrs;
#ifdef DEBUG
        debugPrintf("      Adding library '%s' to queue", libName);
#endif
        enqueue(libName);

        while (*libStrs++ != '\0') ;
    }
}


void parseEofHeader(EofHeader *hdr, unsigned int expectedMagic, FILE *inputFile, char *inputPath) {
    if (fseek(inputFile, 0, SEEK_SET) != 0) {
        error("cannot seek object file header in input file '%s'", inputPath);
    }
    if (fread(hdr, sizeof(EofHeader), 1, inputFile) != 1) {
        error("cannot read object file header in input file '%s'", inputPath);
    }
    conv4FromEcoToNative((unsigned char *) &hdr->magic);
    conv4FromEcoToNative((unsigned char *) &hdr->osegs);
    conv4FromEcoToNative((unsigned char *) &hdr->nsegs);
    conv4FromEcoToNative((unsigned char *) &hdr->osyms);
    conv4FromEcoToNative((unsigned char *) &hdr->nsyms);
    conv4FromEcoToNative((unsigned char *) &hdr->orels);
    conv4FromEcoToNative((unsigned char *) &hdr->nrels);
    conv4FromEcoToNative((unsigned char *) &hdr->odata);
    conv4FromEcoToNative((unsigned char *) &hdr->sdata);
    conv4FromEcoToNative((unsigned char *) &hdr->ostrs);
    conv4FromEcoToNative((unsigned char *) &hdr->sstrs);
    conv4FromEcoToNative((unsigned char *) &hdr->entry);
    conv4FromEcoToNative((unsigned char *) &hdr->olibs);
    conv4FromEcoToNative((unsigned char *) &hdr->nlibs);
    if (hdr->magic != expectedMagic) {
        error("unexpected magic number '0x%08X' in input file '%s', expected '0x%08X'", inputPath, hdr->magic, expectedMagic);
    }
}


void parseStrings(char **strs, unsigned int ostrs, unsigned int sstrs, FILE *inputFile, char *inputPath) {
    if (fseek(inputFile, ostrs, SEEK_SET) != 0) {
        error("cannot seek string space in input file '%s'", inputPath);
    }

    *strs = safeAlloc(sstrs);

    if (fread(*strs, sstrs, 1, inputFile) != 1) {
        error("cannot read string space in input file '%s'", inputPath);
    }
}



char *basename(char *path) {
    char *basename;
    basename = path + strlen(path);
    while (basename != path && *basename != '/') {
        basename--;
    }
    if (*basename == '/') {
        basename++;
    }

    return basename;
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

