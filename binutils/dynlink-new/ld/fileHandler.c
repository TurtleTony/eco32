//
// Created by tony on 27.04.21.
//

#include "fileHandler.h"

Module *firstMod = NULL;

Module *newModule(char *name) {
    Module *mod;
    static Module *previous = NULL;

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
    mod->next = NULL;

    if (previous != NULL) {
        previous->next = mod;
    } else {
        firstMod = mod;
    }

    previous = mod;

    return mod;
}

Module *firstModule() {
    return firstMod;
}


/**************************************************************/
/** PARSING INPUTFILE **/
/**************************************************************/


void readFile(char *inputPath) {
    FILE *inputFile;
    unsigned int magicNumber;

    EofHeader hdr;
    Module *mod;

    inputFile = fopen(inputPath, "r");
    if (inputFile == NULL) {
        error("cannot open input file '%s'", inputPath);
    }

    if (fseek(inputFile, 0, SEEK_SET) != 0) {
        error("cannot seek magic number in input file '%s'", inputPath);
    }

    if (fread(&magicNumber, sizeof(unsigned int), 1, inputFile) != 1) {
        error("cannot read magic number in input file '%s'", inputPath);
    }

    switch(magicNumber) {
        case EOF_MAGIC:
            parseHeader(&hdr, inputFile, inputPath);
            mod = newModule(inputPath);

            // Parse metadata first
            parseData(mod, hdr.odata, hdr.sdata, inputFile, inputPath);
            parseStrings(mod, hdr.ostrs, hdr.sstrs, inputFile, inputPath);

            parseSegments(mod, hdr.osegs, hdr.nsegs, inputFile, inputPath);
            parseSymbols(mod, hdr.osyms, hdr.nsyms, inputFile, inputPath);
            parseRelocations(mod, hdr.orels, hdr.nrels, inputFile, inputPath);

            fclose(inputFile);
            break;
        case ARCH_MAGIC:
            //TODO: Implement read archive
            break;
        default:
            error("unknown magic number in input file '%s'", inputPath);
    }
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
    Symbol *moduleSymbol;
    SymbolRecord symbolRecord;

    module->nsyms = nsyms;
    module->syms = safeAlloc(nsyms * sizeof(Symbol *));

    if (fseek(inputFile, osyms, SEEK_SET) != 0) {
        error("cannot seek symbol table in input file '%s'", inputPath);
    }

    for (int i = 0; i < nsyms; i++) {
        if (fread(&symbolRecord, sizeof(SymbolRecord), 1, inputFile) != 1) {
            error("cannot read symbol %d in input file '%s'", i, inputPath);
        }
        conv4FromEcoToNative((unsigned char *) &symbolRecord.name);
        conv4FromEcoToNative((unsigned char *) &symbolRecord.val);
        conv4FromEcoToNative((unsigned char *) &symbolRecord.seg);
        conv4FromEcoToNative((unsigned char *) &symbolRecord.attr);

        moduleSymbol = safeAlloc(sizeof(Symbol));
        moduleSymbol->name = module->strs + symbolRecord.name;
        moduleSymbol->val = symbolRecord.val;
        moduleSymbol->mod = module;
        moduleSymbol->seg = symbolRecord.seg;
        moduleSymbol->attr = symbolRecord.attr;

        // Symbol name resolution
        putSymbolIntoGst(moduleSymbol, i);
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


/**************************************************************/
/** WRITING OUTPUTFILE **/
/**************************************************************/


void writeExecutable(char *outputPath, char *startSymbolName) {
    FILE *outputFile;

    EofHeader outFileHeader = {};

    unsigned int outFileOffset = 0;

    Symbol *startSymbol = getSymbolFromGst(startSymbolName);
    if (startSymbol == NULL) {
        error("undefined start symbol '%s'", startSymbolName);
    }

    outputFile = fopen(outputPath, "w");
    if (outputFile == NULL) {
        error("cannot open output file '%s'", outputPath);
    }

    writeDummyHeader(&outFileHeader, &outFileOffset, outputFile, outputPath);
    writeData(&outFileHeader, &outFileOffset, outputFile, outputPath);
    writeStrings(&outFileHeader, &outFileOffset, outputFile, outputPath);

    writeSegments(&outFileHeader, &outFileOffset, outputFile, outputPath);

    writeFinalHeader(startSymbol->val, &outFileHeader, outputFile, outputPath);

    fclose(outputFile);
}


void writeDummyHeader(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath) {
    if (fwrite(&outFileHeader, sizeof(EofHeader), 1, outputFile) != 1) {
        error("cannot write output file dummy header to file '%s'", outputPath);
    }
    /* update file offset */
    *outFileOffset += sizeof(EofHeader);
}


void writeData(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath) {
    outFileHeader->odata = *outFileOffset;
    outFileHeader->sdata = 0;

    // All present segments
    writeDataTotal(outFileHeader, apxGroup.firstTotal, outputFile, outputPath);
    writeDataTotal(outFileHeader, apGroup.firstTotal, outputFile, outputPath);
    writeDataTotal(outFileHeader, apwGroup.firstTotal, outputFile, outputPath);

    *outFileOffset += outFileHeader->sdata;
}


void writeDataTotal(EofHeader *outFileHeader, TotalSegment *totalSeg, FILE *outputFile, char *outputPath){
    static unsigned char padding[3] = { 0, 0, 0 };

    while (totalSeg != NULL) {
        totalSeg->dataOffs = outFileHeader->sdata;

        PartialSegment *partial = totalSeg->firstPart;
        while (partial != NULL) {
            unsigned int size = partial->seg->size;
            unsigned int npad = partial->npad;

            if (size != 0) {
                if (fwrite(partial->seg->data, size, 1, outputFile) != 1) {
                    error("cannot write data to file '%s'", outputPath);
                }

                if (npad != 0) {
                    // Word align
                    if (fwrite(padding, 1, partial->npad, outputFile) != partial->npad) {
                        error("cannot write data padding to file '%s'", outputPath);
                    }
                }
            }
            outFileHeader->sdata += size + npad;

            partial = partial->next;
        }

        totalSeg = totalSeg->next;
    }
}


void writeStrings(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile,
                  char *outputPath) {
    outFileHeader->ostrs = *outFileOffset;
    outFileHeader->sstrs = 0;

    writeStringsTotal(outFileHeader, apxGroup.firstTotal, outputFile, outputPath);
    writeStringsTotal(outFileHeader, apGroup.firstTotal, outputFile, outputPath);
    writeStringsTotal(outFileHeader, apwGroup.firstTotal, outputFile, outputPath);
    writeStringsTotal(outFileHeader, awGroup.firstTotal, outputFile, outputPath);

    *outFileOffset += outFileHeader->sstrs;
}


void writeStringsTotal(EofHeader *outFileHeader, TotalSegment *totalSeg, FILE *outputFile, char *outputPath) {
    while (totalSeg != NULL){
        totalSeg->nameOffs = outFileHeader->sstrs;

        unsigned int size = strlen(totalSeg->name) + 1;

        if (fwrite(totalSeg->name, size, 1, outputFile) != 1) {
            error("cannot write segment string to file '%s'", outputPath);
        }

        outFileHeader->sstrs += size;

        totalSeg = totalSeg->next;
    }
}


void writeSegments(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile,
                   char *outputPath) {
    outFileHeader->osegs = *outFileOffset;
    outFileHeader->nsegs = 0;

    writeSegmentsTotal(outFileHeader, apxGroup.firstTotal, outputFile, outputPath);
    writeSegmentsTotal(outFileHeader, apGroup.firstTotal, outputFile, outputPath);
    writeSegmentsTotal(outFileHeader, apwGroup.firstTotal, outputFile, outputPath);
    writeSegmentsTotal(outFileHeader, awGroup.firstTotal, outputFile, outputPath);

    *outFileOffset += sizeof(SegmentRecord) * outFileHeader->nsegs;
}


void writeSegmentsTotal(EofHeader *outFileHeader, TotalSegment *totalSeg, FILE *outputFile, char *outputPath){
    while (totalSeg != NULL){
        SegmentRecord segmentRecord;
        segmentRecord.name = totalSeg->nameOffs;
        segmentRecord.offs = totalSeg->dataOffs;
        segmentRecord.addr = totalSeg->addr;
        segmentRecord.size = totalSeg->size;
        segmentRecord.attr = totalSeg->attr;

        conv4FromNativeToEco((unsigned char *) &segmentRecord.name);
        conv4FromNativeToEco((unsigned char *) &segmentRecord.offs);
        conv4FromNativeToEco((unsigned char *) &segmentRecord.addr);
        conv4FromNativeToEco((unsigned char *) &segmentRecord.size);
        conv4FromNativeToEco((unsigned char *) &segmentRecord.attr);

        if (fwrite(&segmentRecord, sizeof(SegmentRecord), 1, outputFile) != 1) {
            error("cannot write segment %d to file '%s'", outFileHeader->nsegs, outputPath);
        }

        outFileHeader->nsegs++;
        totalSeg = totalSeg->next;
    }
}


void writeFinalHeader(unsigned int codeEntry, EofHeader *outFileHeader, FILE *outputFile, char *outputPath) {
    if (fseek(outputFile, 0, SEEK_SET) != 0) {
        error("cannot seek final header in file '%s'", outputPath);
    }

    outFileHeader->magic = EOF_MAGIC;
    outFileHeader->entry = codeEntry;

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
