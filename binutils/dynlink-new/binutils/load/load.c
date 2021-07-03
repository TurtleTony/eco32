/*
 * load.c -- ECO32 loader
 * Created by tony on 27.06.21.
 */

#include "load.h"


Reloc *listHead = NULL;

unsigned char *memory;

char *libraryPath = DEFAULT_LIB_PATH;

unsigned int lowestAddress = -1; /* First virtual memory address */
unsigned int highestAddress = -1; /* Highest used memory address */

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
    while ((c = getopt_long_only(argc, argv, "l:m:d:?", long_options, &option_index)) != -1) {
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
                if (ldOff != PAGE_ALIGN(ldOff)) {
                    warning("load offset is not page aligned. This is likely a mistake");
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
            case 'd':
                libraryPath = optarg;
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

#ifdef DEBUG
    debugPrintf("Initializing hash tables");
#endif
    // Initialize gst hash table
    initGst();

    loadExecutable(execFileName, ldOff); // This in-turn loads library dependencies recursively

#ifdef DEBUG
    debugPrintf("Handling postponed relocations");
#endif
    Reloc *reloc = listHead;

    while(reloc != NULL) {
        unsigned char *dataPointer = memory + reloc->loc;
#ifdef DEBUG
        debugPrintf("  0x%08X --> 0x%08X",
                        reloc->loc, reloc->symbol->val);
#endif
        write4ToEco(dataPointer, reloc->symbol->val);

        reloc = reloc->next;
    }

    writeBinary(binFileName);
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

    char *libName;
    while((libName = dequeue()) != NULL) {
#ifdef DEBUG
        debugPrintf("  Dequeueing library '%s' with page aligned offset 0x%08X", libName, PAGE_ALIGN(highestAddress));
#endif
        loadLibrary(libName, PAGE_ALIGN(highestAddress));
    }

    fclose(execFile);
}


#ifdef DEBUG
void showSegmentAttr(unsigned int attr, char *res) {
    res[0] = (attr & SEG_ATTR_A) ? 'A' : '-';
    res[1] = (attr & SEG_ATTR_P) ? 'P' : '-';
    res[2] = (attr & SEG_ATTR_W) ? 'W' : '-';
    res[3] = (attr & SEG_ATTR_X) ? 'X' : '-';
    res[4] = '\0';
}
#endif
void loadLinkUnit(char *name, unsigned int expectedMagic, FILE *inputFile, char *inputPath, unsigned int ldOff) {
#ifdef DEBUG
    debugPrintf("  Loading link unit '%s' from file '%s' @ 0x%08X", name, inputPath, ldOff);
#endif
    LinkUnit *linkUnit = newLinkUnit(name);

    EofHeader eofHeader;
    parseEofHeader(&eofHeader, expectedMagic, inputFile, inputPath);

    linkUnit->symbols = safeAlloc(sizeof(Symbol *) * eofHeader.nsyms);

    char *strs;

    parseStrings(&strs, eofHeader.ostrs, eofHeader.sstrs, inputFile, inputPath);
    linkUnit->strs = strs;

#ifdef DEBUG
    debugPrintf("    Scanning %d libraries specified in link unit '%s'", eofHeader.nlibs, name);
#endif
    char *libStrs = strs + eofHeader.olibs;

    for (int i = 0; i < eofHeader.nlibs; i++) {
        char *libName = libStrs;
        enqueue(libName);

        while (*libStrs++ != '\0') ;
    }

#ifdef DEBUG
    debugPrintf("    Loading %d segments from link unit '%s'", eofHeader.nsegs, name);
#endif
    SegmentRecord segmentRecord;
    RelocRecord relocRecord;
    unsigned char *dataPointer;

    for (int i = 0; i < eofHeader.nsegs; i++) {
        parseSegment(&segmentRecord, eofHeader.osegs, i, inputFile, inputPath, strs);

        if (segmentRecord.attr & SEG_ATTR_A) {
            // Allocate memory
            if (lowestAddress == -1) {
                /* Because programs might not be linked at address 0x0 we take the base address of the first segment
                 * (which has to be the lowest) and subtract that from all other address calculations in that link unit.
                 * This way all address calculations are moved down to zero, allowing for easier management */
                lowestAddress = segmentRecord.addr + ldOff;
#ifdef DEBUG
                debugPrintf("    Setting lowest linked address to 0x%08X", lowestAddress);
#endif
            }
            /* This is used to easily access the correct place in the virtual memory */
            dataPointer = memory + (segmentRecord.addr + ldOff - lowestAddress);
            highestAddress = segmentRecord.addr + ldOff + segmentRecord.size;

#ifdef DEBUG

            char attr[10];
            showSegmentAttr(segmentRecord.attr, attr);
            debugPrintf("      %s @ 0x%08X, size: 0x%08X, attr: [%s]",
                        strs + segmentRecord.name, dataPointer - memory, segmentRecord.size, attr);
#endif
            if (segmentRecord.attr & SEG_ATTR_P) {
                if (fseek(inputFile, eofHeader.odata + segmentRecord.offs, SEEK_SET) < 0) {
                    error("cannot seek to data of segment '%s' in file '%s'",
                          strs + segmentRecord.name, inputPath);
                }
                if (fread(dataPointer, 1, segmentRecord.size, inputFile) != segmentRecord.size) {
                    error("cannot read data of segment '%s' in file '%s'",
                          strs + segmentRecord.name, inputPath);
                }
            } else {
                memset(dataPointer, 0, segmentRecord.size);
            }
        }
    }

    parseSymbols(linkUnit, eofHeader.osyms, eofHeader.nsyms, inputFile, inputPath, strs, ldOff);

#ifdef DEBUG
    debugPrintf("    Parsing %d relocations", eofHeader.nrels);
#endif
    // load relocations
    for(int i = 0; i < eofHeader.nrels; i++) {
        parseRelocation(&relocRecord, eofHeader.orels, i, inputFile, inputPath);

        if (relocRecord.typ & RELOC_ER_W32) {
            dataPointer = memory + ldOff + relocRecord.loc - lowestAddress;
#ifdef DEBUG
            debugPrintf("      0x%08X --> + 0x%08X", dataPointer - memory, ldOff);
#endif
            uint32_t mem = read4FromEco(dataPointer);
            mem += ldOff;
            write4ToEco(dataPointer, mem);
        } else if (relocRecord.typ & RELOC_W32) {
            Reloc *reloc = newReloc();
            reloc->loc = ldOff + relocRecord.loc - lowestAddress;
            reloc->symbol = linkUnit->symbols[relocRecord.ref];
#ifdef DEBUG
            debugPrintf("      0x%08X --> %s -- postponed", reloc->loc, reloc->symbol->name);
#endif
        } else {
            error("unhandled relocation type '%d'", relocRecord.typ);
        }
    }
}


void loadLibrary(char *name, unsigned int ldOff) {
    // Search lib file by name in path
    char *inputPath = safeAlloc(strlen(libraryPath) + strlen(name));
    sprintf(inputPath, "%s/%s", libraryPath, name);

    FILE *libFile = fopen(inputPath, "r");
    if (libFile == NULL) {
        error("cannot open library file '%s'", inputPath);
    }

    loadLinkUnit(name, EOF_D_MAGIC, libFile, inputPath, ldOff);

    fclose(libFile);
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


void parseSegment(SegmentRecord *segmentRecord, unsigned int osegs, unsigned int segno, FILE *inputFile, char *inputPath, char *strs) {
    if (fseek(inputFile, osegs + segno * sizeof(SegmentRecord), SEEK_SET) != 0) {
        error("cannot seek segment %d in input file '%s'", segno, inputPath);
    }

    if (fread(segmentRecord, sizeof(SegmentRecord), 1, inputFile) != 1) {
        error("cannot read segment %d in input file '%s'", segno, inputPath);
    }
    conv4FromEcoToNative((unsigned char *) &segmentRecord->name);
    conv4FromEcoToNative((unsigned char *) &segmentRecord->offs);
    conv4FromEcoToNative((unsigned char *) &segmentRecord->addr);
    conv4FromEcoToNative((unsigned char *) &segmentRecord->size);
    conv4FromEcoToNative((unsigned char *) &segmentRecord->attr);
}

void parseSymbols(LinkUnit *linkUnit, unsigned int osyms, unsigned int nsyms, FILE *inputFile, char *inputPath,
                  char *strs, unsigned int ldOff) {
    Symbol *moduleSymbol;
    SymbolRecord symbolRecord;

    if (fseek(inputFile, osyms, SEEK_SET) != 0) {
        error("cannot seek symbol table in input file '%s'", inputPath);
    }

#ifdef DEBUG
    debugPrintf("    Parsing %d symbols", nsyms);
#endif
    for (int i = 0; i < nsyms; i++) {
        if (fread(&symbolRecord, sizeof(SymbolRecord), 1, inputFile) != 1) {
            error("cannot read symbol %d in input file '%s'", i, inputPath);
        }
        conv4FromEcoToNative((unsigned char *) &symbolRecord.name);
        conv4FromEcoToNative((unsigned char *) &symbolRecord.val);
        conv4FromEcoToNative((unsigned char *) &symbolRecord.seg);
        conv4FromEcoToNative((unsigned char *) &symbolRecord.attr);

        moduleSymbol = safeAlloc(sizeof(Symbol));
        moduleSymbol->name = strs + symbolRecord.name;
        moduleSymbol->val = symbolRecord.seg == -1 ? symbolRecord.val : (ldOff + symbolRecord.val);
        moduleSymbol->seg = symbolRecord.seg;
        moduleSymbol->attr = symbolRecord.attr;

        // Add symbol into link-unit shared table
        linkUnit->symbols[i] = putSymbolIntoGst(moduleSymbol);
    }
}


void parseRelocation(RelocRecord *relocRecord, unsigned int orels, unsigned int relno, FILE *inputFile, char *inputPath) {
    if (fseek(inputFile, orels + relno * sizeof(RelocRecord), SEEK_SET) != 0) {
        error("cannot seek relocation %d in input file '%s'", relno, inputPath);
    }

    if (fread(relocRecord, sizeof(RelocRecord), 1, inputFile) != 1) {
        error("cannot read relocation %d in input file '%s'", relno, inputPath);
    }
    conv4FromEcoToNative((unsigned char *) &relocRecord->loc);
    conv4FromEcoToNative((unsigned char *) &relocRecord->seg);
    conv4FromEcoToNative((unsigned char *) &relocRecord->typ);
    conv4FromEcoToNative((unsigned char *) &relocRecord->ref);
    conv4FromEcoToNative((unsigned char *) &relocRecord->add);
}


LinkUnit *newLinkUnit(char *name) {
    LinkUnit *linkUnit;

    linkUnit = safeAlloc(sizeof(LinkUnit));
    linkUnit->name = name;
    linkUnit->strs = NULL;

    return linkUnit;
}


Reloc *newReloc() {
    Reloc *reloc;
    reloc = safeAlloc(sizeof(Reloc));
    reloc->loc = 0;
    reloc->symbol = NULL;
    reloc->next = NULL;

    if (listHead == NULL) {
        listHead = reloc;
    } else {
        Reloc *last = listHead;
        while (1) {
            if (last->next == NULL) {
                last->next = reloc;
                break;
            }
            last = last->next;
        }
    }

    return reloc;
}


void writeBinary(char *fileName) {
    FILE *binaryFile;
    unsigned int size;

#ifdef DEBUG
    debugPrintf("Writing output binary file");
#endif
    binaryFile = fopen(fileName, "w");
    if (binaryFile == NULL) {
        error("cannot open binary file '%s'", fileName);
    }

    size = highestAddress - lowestAddress;
    if (size) {
#ifdef DEBUG
        debugPrintf("  with size %d", size);
#endif
        if (fwrite(memory, 1, size, binaryFile) != size) {
            error("cannot write to binary file '%s'", fileName);
        }
    }

    fclose(binaryFile);
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
    printf("    -d <n>         path to search .dso library files\n");
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

