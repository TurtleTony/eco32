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

    // TODO: turn into loop, parsing multiple input files
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
    return mod;
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

    EofHeader outFileHeader;
    unsigned int outFileOffset = 0;

    outputFile = fopen(outputPath, "w");
    if (outputFile == NULL) {
        error("cannot open output file '%s'", outputPath);
    }

    writeDummyHeader(&outFileHeader, &outFileOffset, outputFile, outputPath);
    writeData(module, &outFileHeader, &outFileOffset, outputFile, outputPath);
    writeStrings(module, &outFileHeader, &outFileOffset, outputFile, outputPath);

    writeSegments(module, &outFileHeader, &outFileOffset, outputFile, outputPath);
    writeSymbols(module, &outFileHeader, &outFileOffset, outputFile, outputPath);
    writeRelocations(module, &outFileHeader, &outFileOffset, outputFile, outputPath);

    writeFinalHeader(module, &outFileHeader, &outFileOffset, outputFile, outputPath);

    fclose(outputFile);
}


void writeDummyHeader(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath) {
    if (fwrite(&outFileHeader, sizeof(EofHeader), 1, outputFile) != 1) {
        error("cannot write output file dummy header to file '%s'", outputPath);
    }
    /* update file offset */
    *outFileOffset += sizeof(EofHeader);
}


void writeData(Module *module, EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile,
               char *outputPath) {
    Segment *seg;

    outFileHeader->odata = *outFileOffset;
    outFileHeader->sdata = 0;

    for (int i = 0; i < module->nsegs; i++) {
        seg = &module->segs[i];
        seg->dataOffs = outFileHeader->sdata;
        if (seg->attr & SEG_ATTR_P) {
            if (fwrite(seg->data, seg->size, 1, outputFile) != 1) {
                error("cannot write data to file '%s'", outputPath);
            }
            outFileHeader->sdata += seg->size;
        }
    }

    *outFileOffset += outFileHeader->sdata;
}


void writeStrings(Module *module, EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile,
                  char *outputPath) {
    Segment *seg;
    Symbol *sym;

    unsigned int size;

    outFileHeader->ostrs = *outFileOffset;
    outFileHeader->sstrs = 0;

    for (int i = 0; i < module->nsegs; i++) {
        seg = &module->segs[i];
        seg->nameOffs = outFileHeader->sstrs;
        size = strlen(seg->name) + 1;
        if (fwrite(seg->name, size, 1, outputFile) != 1) {
            error("cannot write segment string to file '%s'", outputPath);
        }
        outFileHeader->sstrs += size;
    }

    for (int j = 0; j < module->nsyms; j++) {
        sym = &module->syms[j];
        sym->nameOffs = outFileHeader->sstrs;
        size = strlen(sym->name) + 1;
        if (fwrite(sym->name, size, 1, outputFile) != 1) {
            error("cannot write symbol string to file '%s'", outputPath);
        }
        outFileHeader->sstrs += size;
    }

    *outFileOffset += outFileHeader->sstrs;
}


void writeSegments(Module *module, EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile,
                   char *outputPath) {
    Segment *seg;
    SegmentRecord segmentRecord;

    outFileHeader->osegs = *outFileOffset;
    outFileHeader->nsegs = module->nsegs;

    for (int i = 0; i < module->nsegs; i++) {
        seg = &module->segs[i];
        segmentRecord.name = seg->nameOffs;
        segmentRecord.offs = seg->dataOffs;
        segmentRecord.addr = seg->addr;
        segmentRecord.size = seg->size;
        segmentRecord.attr = seg->attr;

        conv4FromNativeToEco((unsigned char *) &segmentRecord.name);
        conv4FromNativeToEco((unsigned char *) &segmentRecord.offs);
        conv4FromNativeToEco((unsigned char *) &segmentRecord.addr);
        conv4FromNativeToEco((unsigned char *) &segmentRecord.size);
        conv4FromNativeToEco((unsigned char *) &segmentRecord.attr);

        if (fwrite(&segmentRecord, sizeof(SegmentRecord), 1, outputFile) != 1) {
            error("cannot write segment %d to file '%s'", i, outputPath);
        }

        *outFileOffset += sizeof(SegmentRecord);
    }
}


void writeSymbols(Module *module, EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile,
                  char *outputPath) {
    Symbol *sym;
    SymbolRecord symbolRecord;

    outFileHeader->osyms = *outFileOffset;
    outFileHeader->nsyms = module->nsyms;

    for (int i = 0; i < module->nsyms; i++) {
        sym = &module->syms[i];
        symbolRecord.name = sym->nameOffs;
        symbolRecord.val = sym->val;
        symbolRecord.seg = sym->seg;
        symbolRecord.attr = sym->attr;

        conv4FromNativeToEco((unsigned char *) &symbolRecord.name);
        conv4FromNativeToEco((unsigned char *) &symbolRecord.val);
        conv4FromNativeToEco((unsigned char *) &symbolRecord.seg);
        conv4FromNativeToEco((unsigned char *) &symbolRecord.attr);

        if (fwrite(&symbolRecord, sizeof(SymbolRecord), 1, outputFile) != 1) {
            error("cannot write symbol %d to file '%s'", i, outputPath);
        }

        *outFileOffset += sizeof(SymbolRecord);
    }
}


void writeRelocations(Module *module, EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile,
                      char *outputPath) {
    Reloc *reloc;
    RelocRecord relocRecord;

    outFileHeader->orels = *outFileOffset;
    outFileHeader->nrels = module->nrels;

    for (int i = 0; i < module->nrels; i++) {
        reloc = &module->rels[i];
        relocRecord.loc = reloc->loc;
        relocRecord.seg = reloc->seg;
        relocRecord.typ = reloc->typ;
        relocRecord.ref = reloc->ref;
        relocRecord.add = reloc->add;

        conv4FromNativeToEco((unsigned char *) &relocRecord.loc);
        conv4FromNativeToEco((unsigned char *) &relocRecord.seg);
        conv4FromNativeToEco((unsigned char *) &relocRecord.typ);
        conv4FromNativeToEco((unsigned char *) &relocRecord.ref);
        conv4FromNativeToEco((unsigned char *) &relocRecord.add);

        if (fwrite(&relocRecord, sizeof(RelocRecord), 1, outputFile) != 1) {
            error("cannot write relocation %d to file '%s'", i, outputPath);
        }

        *outFileOffset += sizeof(RelocRecord);
    }
}


void writeFinalHeader(Module *module, EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile,
                      char *outputPath) {
    if (fseek(outputFile, 0, SEEK_SET) != 0) {
        error("cannot seek final header in file '%s'", outputPath);
    }

    outFileHeader->magic = EOF_MAGIC;
    outFileHeader->entry = 0;

    conv4FromNativeToEco((unsigned char *) &outFileHeader->magic);
    conv4FromNativeToEco((unsigned char *) &outFileHeader->osegs);
    conv4FromNativeToEco((unsigned char *) &outFileHeader->nsegs);
    conv4FromNativeToEco((unsigned char *) &outFileHeader->osyms);
    conv4FromNativeToEco((unsigned char *) &outFileHeader->nsyms);
    conv4FromNativeToEco((unsigned char *) &outFileHeader->orels);
    conv4FromNativeToEco((unsigned char *) &outFileHeader->nrels);
    conv4FromNativeToEco((unsigned char *) &outFileHeader->odata);
    conv4FromNativeToEco((unsigned char *) &outFileHeader->sdata);
    conv4FromNativeToEco((unsigned char *) &outFileHeader->ostrs);
    conv4FromNativeToEco((unsigned char *) &outFileHeader->sstrs);
    conv4FromNativeToEco((unsigned char *) &outFileHeader->entry);

    if (fwrite(outFileHeader, sizeof(EofHeader), 1, outputFile) != 1) {
        error("cannot write output file final header to file '%s'", outputPath);
    }
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
