/*
 * ld.c -- ECO32 link editor
 * Created by tony on 20.04.21.
 */

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


void safeFree(void *p) {
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


Module *readModule(char *inputPath) {
    FILE *inputFile;
    EofHeader hdr;
    Module *mod;

    inputFile = fopen(inputPath, "r");
    if (inputFile == NULL) {
        error("cannot open input file '%s'", inputPath);
    }

    parseHeader(&hdr, inputFile, inputPath);
    mod = newModule(inputPath);

    // Parse metadata first
    parseData(mod, hdr.odata, hdr.sdata, inputFile, inputPath);
    parseStrings(mod, hdr.ostrs, hdr.sstrs, inputFile, inputPath);

    parseSegments(mod, hdr.osegs, hdr.nsegs, inputFile, inputPath);
    parseSymbols(mod, hdr.osyms, hdr.nsyms, inputFile, inputPath);
    parseRelocations(mod, hdr.orels, hdr.nrels, inputFile, inputPath);

    fclose(inputFile);
    return NULL;
}

void parseHeader(EofHeader *hdr, FILE *inputFile, char *inputPath) {
    if (fseek(inputFile, 0, SEEK_SET) != 0) {
        error("cannot seek header in input file '%s'", inputPath);
    }
    if (fread(hdr, sizeof(EofHeader), 1, inputFile) != 1) {
        error("cannot read header in input file '%s'", inputPath);
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
    if (hdr->magic != EOF_MAGIC) {
        error("wrong magic number in input file '%s'", inputPath);
    }
}

void parseData(Module *module, unsigned int odata, unsigned int sdata, FILE *inputFile, char *inputPath) {
    if (fseek(inputFile, odata, SEEK_SET) != 0) {
        error("cannot seek data space in input file '%s'", inputPath);
    }

    module->data = safeAlloc(sdata);

    if (fread(module->data, sdata, 1, inputFile) != 1) {
        error("cannot read data space in input file '%s'", inputPath);
    }
}

void parseStrings(Module *module, unsigned int ostrs, unsigned int sstrs, FILE *inputFile, char *inputPath) {
    if (fseek(inputFile, ostrs, SEEK_SET) != 0) {
        error("cannot seek string space in input file '%s'", inputPath);
    }

    module->strs = safeAlloc(sstrs);

    if (fread(module->strs, sstrs, 1, inputFile) != 1) {
        error("cannot read string space in input file '%s'", inputPath);
    }
}


void parseSegments(Module *module, unsigned int osegs, unsigned int nsegs, FILE *inputFile, char *inputPath) {
    Segment *seg;
    SegmentRecord segmentRecord;

    module->nsegs = nsegs;
    module->segs = safeAlloc(nsegs * sizeof(Segment));

    if (fseek(inputFile, osegs, SEEK_SET) != 0) {
        error("cannot seek segment table in input file '%s'", inputPath);
    }

    for (int i = 0; i < nsegs; i++) {
        seg = &module->segs[i];
        if (fread(&segmentRecord, sizeof(SegmentRecord), 1, inputFile) != 1) {
            error("cannot read segment %d in input file '%s'", i, inputPath);
        }
        conv4FromEcoToNative((unsigned char *) &segmentRecord.name);
        conv4FromEcoToNative((unsigned char *) &segmentRecord.offs);
        conv4FromEcoToNative((unsigned char *) &segmentRecord.addr);
        conv4FromEcoToNative((unsigned char *) &segmentRecord.size);
        conv4FromEcoToNative((unsigned char *) &segmentRecord.attr);
        seg->name = module->strs + segmentRecord.name;
        seg->data = module->data + segmentRecord.offs;
        seg->addr = segmentRecord.addr;
        seg->size = segmentRecord.size;
        seg->attr = segmentRecord.attr;
    }
}


void parseSymbols(Module *module, unsigned int osyms, unsigned int nsyms, FILE *inputFile, char *inputPath) {
    Symbol *sym;
    SymbolRecord symbolRecord;

    module->nsyms = nsyms;
    module->syms = safeAlloc(nsyms * sizeof(Symbol));

    if (fseek(inputFile, osyms, SEEK_SET) != 0) {
        error("cannot seek symbol table in input file '%s'", inputPath);
    }

    for (int i = 0; i < nsyms; i++) {
        sym = &module->syms[i];
        if (fread(&symbolRecord, sizeof(SymbolRecord), 1, inputFile) != 1) {
            error("cannot read symbol %d in input file '%s'", i, inputPath);
        }
        conv4FromEcoToNative((unsigned char *) &symbolRecord.name);
        conv4FromEcoToNative((unsigned char *) &symbolRecord.val);
        conv4FromEcoToNative((unsigned char *) &symbolRecord.seg);
        conv4FromEcoToNative((unsigned char *) &symbolRecord.attr);

        sym->name = module->strs + symbolRecord.name;
        sym->val = symbolRecord.val;
        sym->seg = symbolRecord.seg;
        sym->attr = symbolRecord.attr;
    }
}


void parseRelocations(Module *module, unsigned int orels, unsigned int nrels, FILE *inputFile, char *inputPath) {
    Reloc *reloc;
    RelocRecord relocRecord;

    module->nrels = nrels;
    module->rels = safeAlloc(nrels * sizeof(Reloc));

    if (fseek(inputFile, orels, SEEK_SET) != 0) {
        error("cannot seek symbol table in input file '%s'", inputPath);
    }

    for (int i = 0; i < nrels; i++) {
        reloc = &module->rels[i];
        if (fread(&relocRecord, sizeof(RelocRecord), 1, inputFile) != 1) {
            error("cannot read relocation %d in input file '%s'", i, inputPath);
        }
        conv4FromEcoToNative((unsigned char *) &relocRecord.loc);
        conv4FromEcoToNative((unsigned char *) &relocRecord.seg);
        conv4FromEcoToNative((unsigned char *) &relocRecord.typ);
        conv4FromEcoToNative((unsigned char *) &relocRecord.ref);
        conv4FromEcoToNative((unsigned char *) &relocRecord.add);

        reloc->loc = relocRecord.loc;
        reloc->seg = relocRecord.seg;
        reloc->typ = relocRecord.typ;
        reloc->ref = relocRecord.ref;
        reloc->add = relocRecord.add;
    }
}


void writeModule(Module *module, char *outputPath) {
    FILE *outputFile;

    outputFile = fopen(outputPath, "w");
    if (outputFile == NULL) {
        error("cannot open output file '%s'", outputPath);
    }

    /** TODO :
     *  Open output file
     *  Write Header
     *  Write Data
     *  Write Strings
     *  Write Symbols
     *  Write Relocs
     *  Write Final Header
    */

    fclose(outputFile);
}


/**************************************************************/
/** CONVERSION METHODS */
/**************************************************************/


uint32_t read4FromEco(unsigned char *p) {
    return (uint32_t) p[0] << 24 |
           (uint32_t) p[1] << 16 |
           (uint32_t) p[2] << 8 |
           (uint32_t) p[3] << 0;
}


void write4ToEco(unsigned char *p, uint32_t data) {
    p[0] = data >> 24;
    p[1] = data >> 16;
    p[2] = data >> 8;
    p[3] = data >> 0;
}


void conv4FromEcoToNative(unsigned char *p) {
    uint32_t data;

    data = read4FromEco(p);
    *(uint32_t *) p = data;
}


void conv4FromNativeToEco(unsigned char *p) {
    uint32_t data;

    data = *(uint32_t *) p;
    write4ToEco(p, data);
}


/**************************************************************/
