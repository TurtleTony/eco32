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


/**************************************************************/
/** PARSING INPUTFILE **/
/**************************************************************/


void readFile(char *inputPath) {
    FILE *inputFile;
    unsigned int magicNumber;

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
    conv4FromEcoToNative((unsigned char *) &magicNumber);

    switch(magicNumber) {
        case EOF_R_MAGIC:
            if (strcmp(strstr(inputPath, ".o"), ".o") != 0) {
                warning("file extension for object files should be '.o'");
            }
#ifdef DEBUG
            debugPrintf("  Parsing object file '%s'", inputPath);
#endif
            parseObjectFile(basename(inputPath), 0, inputFile, inputPath);
            break;
        case ARCH_MAGIC:
            if (strcmp(strstr(inputPath, ".a"), ".a") != 0) {
                warning("file extension for archive files should be '.a'");
            }
#ifdef DEBUG
            debugPrintf("  Parsing archive file '%s'", inputPath);
#endif
            parseArchiveFile(inputFile, inputPath);
            break;
        case EOF_D_MAGIC:
            if (strcmp(strstr(inputPath, ".dso"), ".dso") != 0) {
                warning("file extension for dynamic library files should be '.dso'");
            }
#ifdef DEBUG
            debugPrintf("  Parsing dynamic library file '%s'", inputPath);
#endif
            parseDynamicLibraryFile(inputFile, inputPath);
            break;
        default:
            error("unknown magic number '%X' in input file '%s'", magicNumber, inputPath);
    }

    fclose(inputFile);
}


void parseObjectFile(char *moduleName, unsigned int fileOffset, FILE *objectFile, char *inputPath) {
    EofHeader hdr;
    Module *mod;

    parseEofHeader(&hdr, fileOffset, objectFile, inputPath);
    mod = newModule(moduleName);

    // Parse metadata first
    parseData(mod, fileOffset + hdr.odata, hdr.sdata, objectFile, inputPath);
    parseStrings(&mod->strs, fileOffset + hdr.ostrs, hdr.sstrs, objectFile, inputPath);

    parseSegments(mod, fileOffset + hdr.osegs, hdr.nsegs, objectFile, inputPath);
    parseSymbols(mod, fileOffset + hdr.osyms, hdr.nsyms, objectFile, inputPath);
    parseRelocations(mod, fileOffset + hdr.orels, hdr.nrels, objectFile, inputPath);
}


void parseArchiveFile(FILE *archiveFile, char *inputPath) {
    ArchHeader hdr;
    char *strs;
    ModuleRecord *mods;

    parseArchiveHeader(&hdr, archiveFile, inputPath);
    strs = parseArchiveStrings(hdr.ostrs, hdr.sstrs, archiveFile, inputPath);
    mods = parseArchiveModules(hdr.omods, hdr.nmods, archiveFile, inputPath);

    int moduleWasNeeded;
    do {
        moduleWasNeeded = 0;
        for (int i = 0; i < hdr.nmods; i++) {
            if (!moduleNeeded(&mods[i], strs)) continue;

            moduleWasNeeded = 1;

            char *moduleName = safeAlloc(strlen(strs + mods[i].name) + 1);
            strcpy(moduleName, strs + mods[i].name);
#ifdef DEBUG
            debugPrintf("   Parsing module '%s' from archive file '%s'", moduleName, inputPath);
#endif
            parseObjectFile(moduleName, hdr.odata + mods[i].offs, archiveFile, inputPath);

            /* No need to scan symbol directory again, already parsed */
            mods[i].nsym = 0;
        }
    } while (moduleWasNeeded);
    safeFree(mods);
    safeFree(strs);
}


Library *firstLibrary = NULL;
Library *lastLibrary = NULL;

void parseDynamicLibraryFile(FILE *libraryFile, char *inputPath) {
    EofHeader hdr;
    char *strs;

    Library *library = safeAlloc(sizeof(Library));

    library->name = basename(inputPath);

    if (firstLibrary == NULL) {
        firstLibrary = library;
        lastLibrary = library;
    } else {
        lastLibrary->next = library;
        lastLibrary = library;
    }

    parseEofHeader(&hdr, 0, libraryFile, inputPath);
    parseStrings(&strs, hdr.ostrs, hdr.sstrs, libraryFile, inputPath);

    parseDynamicLibrarySymbols(strs, hdr.osyms, hdr.nsyms, libraryFile, inputPath);
}


int moduleNeeded(ModuleRecord *moduleRecord, char *strs) {
    char *fsym = strs + moduleRecord->fsym;

    for (int i = 0; i < moduleRecord->nsym; i++) {
        Symbol *entry = getSymbolFromGst(fsym);
        if (entry != NULL && (entry->attr & (SYM_ATTR_U | SYM_ATTR_X)) != 0) {
            /* Symbol is undefined */
            return 1;
        }
        while (*fsym++ != '\0') ;
    }

    return 0;
}


void parseArchiveHeader(ArchHeader *hdr, FILE *inputFile, char *inputPath) {
    if (fseek(inputFile, 0, SEEK_SET) != 0) {
        error("cannot seek archive header in input file '%s'", inputPath);
    }
    if (fread(hdr, sizeof(ArchHeader), 1, inputFile) != 1) {
        error("cannot read archive header in input file '%s'", inputPath);
    }
    conv4FromEcoToNative((unsigned char *) &hdr->magic);
    conv4FromEcoToNative((unsigned char *) &hdr->omods);
    conv4FromEcoToNative((unsigned char *) &hdr->nmods);
    conv4FromEcoToNative((unsigned char *) &hdr->odata);
    conv4FromEcoToNative((unsigned char *) &hdr->sdata);
    conv4FromEcoToNative((unsigned char *) &hdr->ostrs);
    conv4FromEcoToNative((unsigned char *) &hdr->sstrs);
    if (hdr->magic != ARCH_MAGIC) {
        error("wrong magic number in input file '%s'", inputPath);
    }
}


char *parseArchiveStrings(unsigned int ostrs, unsigned int sstrs, FILE *inputFile, char *inputPath) {
    if (fseek(inputFile, ostrs, SEEK_SET) != 0) {
        error("cannot seek string space in input file '%s'", inputPath);
    }

    char *strings = safeAlloc(sstrs);

    if (fread(strings, sstrs, 1, inputFile) != 1) {
        error("cannot read string space in input file '%s'", inputPath);
    }
    return strings;
}


ModuleRecord *parseArchiveModules(unsigned int omods, unsigned int nmods, FILE *inputFile, char *inputPath) {
    ModuleRecord *moduleRecords = safeAlloc(nmods * sizeof(ModuleRecord));

    if (fseek(inputFile, omods, SEEK_SET) != 0) {
        error("cannot seek module table in input file '%s'", inputPath);
    }

    for (int i = 0; i < nmods; i++) {
        if (fread(&moduleRecords[i], sizeof(ModuleRecord), 1, inputFile) != 1) {
            error("cannot read module %d in input file '%s'", i, inputPath);
        }
        conv4FromEcoToNative((unsigned char *) &moduleRecords[i].name);
        conv4FromEcoToNative((unsigned char *) &moduleRecords[i].offs);
        conv4FromEcoToNative((unsigned char *) &moduleRecords[i].size);
        conv4FromEcoToNative((unsigned char *) &moduleRecords[i].fsym);
        conv4FromEcoToNative((unsigned char *) &moduleRecords[i].nsym);
    }

    return moduleRecords;
}


void parseEofHeader(EofHeader *hdr, unsigned int fileOffset, FILE *inputFile, char *inputPath) {
    if (fseek(inputFile, fileOffset, SEEK_SET) != 0) {
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
    if (hdr->magic != EOF_R_MAGIC && hdr->magic != EOF_D_MAGIC) {
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


void parseStrings(char **strs, unsigned int ostrs, unsigned int sstrs, FILE *inputFile, char *inputPath) {
    if (fseek(inputFile, ostrs, SEEK_SET) != 0) {
        error("cannot seek string space in input file '%s'", inputPath);
    }

    *strs = safeAlloc(sstrs);

    if (fread(*strs, sstrs, 1, inputFile) != 1) {
        error("cannot read string space in input file '%s'", inputPath);
    }
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

void parseSegments(Module *module, unsigned int osegs, unsigned int nsegs, FILE *inputFile, char *inputPath) {
    Segment *seg;
    SegmentRecord segmentRecord;

    module->nsegs = nsegs;
    module->segs = safeAlloc(nsegs * sizeof(Segment));

    if (fseek(inputFile, osegs, SEEK_SET) != 0) {
        error("cannot seek segment table in input file '%s'", inputPath);
    }

#ifdef DEBUG
    debugPrintf("    Parsing segments in module '%s'", module->name);
#endif
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
#ifdef DEBUG
        char attr[10];
        showSegmentAttr(seg->attr, attr);
        debugPrintf("      %s size: 0x%08X, attr: [%s]", seg->name, seg->size, attr);
#endif
    }
}

#ifdef DEBUG
void showSymbolAttr(unsigned int attr, char *res) {
    res[0] = (attr & SYM_ATTR_U) ? 'U' : 'D';
    res[1] = '\0';
}
#endif

void parseSymbols(Module *module, unsigned int osyms, unsigned int nsyms, FILE *inputFile, char *inputPath) {
    Symbol *moduleSymbol;
    SymbolRecord symbolRecord;

    module->nsyms = nsyms;
    module->syms = safeAlloc(nsyms * sizeof(Symbol *));

    if (fseek(inputFile, osyms, SEEK_SET) != 0) {
        error("cannot seek symbol table in input file '%s'", inputPath);
    }

#ifdef DEBUG
    debugPrintf("    Parsing symbols in module '%s'", module->name);
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
        moduleSymbol->name = module->strs + symbolRecord.name;
        moduleSymbol->val = symbolRecord.val;
        moduleSymbol->mod = module;
        moduleSymbol->seg = symbolRecord.seg;
        moduleSymbol->attr = symbolRecord.attr;

        // Symbol name resolution
        putSymbolIntoGst(moduleSymbol, i);
#ifdef DEBUG
        char attr[10];
        showSymbolAttr(moduleSymbol->attr, attr);
        debugPrintf("        %s : 0x%08X, (%s) [%s]", moduleSymbol->name, moduleSymbol->val,
                    moduleSymbol->seg != -1 ? module->segs[moduleSymbol->seg].name : "absolute", attr);
#endif
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

        if (!picMode && (((reloc->typ & ~RELOC_SYM) == RELOC_GA_L16) || (reloc->typ & ~RELOC_SYM) == RELOC_GA_H16)) {
#ifdef DEBUG
            debugPrintf("    Found PIC relocation: Entering PIC mode");
#endif
            picMode = 1;
        }

        if ((reloc->typ & ~RELOC_SYM) == RELOC_W32) {
            w32Count++;
        }

        if ((reloc->typ & ~RELOC_SYM) == RELOC_GP_L16) {
            if (!(reloc->typ & RELOC_SYM)) {
                error("got pointer relocation has to reference symbol");
            }

            putEntryIntoGot(module->syms[reloc->ref]);
        }
    }
}


void parseDynamicLibrarySymbols(char *strs, unsigned int osyms, unsigned int nsyms, FILE *inputFile, char *inputPath) {
    Symbol *moduleSymbol;
    SymbolRecord symbolRecord;

    if (fseek(inputFile, osyms, SEEK_SET) != 0) {
        error("cannot seek symbol table in input file '%s'", inputPath);
    }

#ifdef DEBUG
    debugPrintf("    Parsing symbols in library '%s'", inputPath);
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
        moduleSymbol->val = symbolRecord.val;
        moduleSymbol->mod = NULL;
        moduleSymbol->seg = symbolRecord.seg;
        moduleSymbol->attr = (symbolRecord.attr | SYM_ATTR_X);

        // Symbol name resolution (only add exported symbols)
        if (!(moduleSymbol->attr & SYM_ATTR_U)) {
            putSymbolIntoGst(moduleSymbol, i);
        }
#ifdef DEBUG
        char attr[10];
        showSymbolAttr(moduleSymbol->attr, attr);
        debugPrintf("        %s : 0x%08X, (%s) [%s]", moduleSymbol->name, moduleSymbol->val,
                    moduleSymbol->seg != -1 ? "some segment" : "absolute", attr);
#endif
    }
}


/**************************************************************/
/** WRITING OUTPUTFILE **/
/**************************************************************/


void writeExecutable(char *outputPath, char *startSymbolName, Segment *gotSegment) {
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
    writeSymbols(&outFileHeader, &outFileOffset, outputFile, outputPath);

    if (picMode) {
        writeRelocations(gotSegment, &outFileHeader, &outFileOffset, outputFile, outputPath);
    }

    writeFinalHeader(0, startSymbol->val, &outFileHeader, outputFile, outputPath);

    fclose(outputFile);
}


void writeLibrary(char *outputPath, Segment *gotSegment) {
    FILE *outputFile;

    EofHeader outFileHeader = {};
    unsigned int outFileOffset = 0;

    if (!picMode) {
        error("trying to create shared library without pic");
    }

    outputFile = fopen(outputPath, "w");
    if (outputFile == NULL) {
        error("cannot open output file '%s'", outputPath);
    }

    writeDummyHeader(&outFileHeader, &outFileOffset, outputFile, outputPath);

    writeData(&outFileHeader, &outFileOffset, outputFile, outputPath);
    writeStrings(&outFileHeader, &outFileOffset, outputFile, outputPath);

    writeSegments(&outFileHeader, &outFileOffset, outputFile, outputPath);
    writeSymbols(&outFileHeader, &outFileOffset, outputFile, outputPath);

    writeRelocations(gotSegment, &outFileHeader, &outFileOffset, outputFile, outputPath);

    writeFinalHeader(1, 0, &outFileHeader, outputFile, outputPath);

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

    writeStringsSymbols(outFileHeader, outputFile, outputPath);
    writeStringsLibs(outFileHeader, outputFile, outputPath);

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


void writeStringsSymbols(EofHeader *outFileHeader, FILE *outputFile, char *outputPath) {
    Symbol *entry;
    kh_foreach_value(getGst(), entry, {
        entry->nameOffs = outFileHeader->sstrs;
        unsigned int size = strlen(entry->name) + 1;

        if (fwrite(entry->name, size, 1, outputFile) != 1) {
            error("cannot write symbol string to file '%s'", outputPath);
        }
        outFileHeader->sstrs += size;
    });
}


void writeStringsLibs(EofHeader *outFileHeader, FILE *outputFile, char *outputPath) {
    Library *library = firstLibrary;

    outFileHeader->olibs = outFileHeader->sstrs;

    while (library != NULL) {
        unsigned int size = strlen(library->name) + 1;

        if (fwrite(library->name, size, 1, outputFile) != 1) {
            error("cannot write library name %s to file '%s'", library->name, outputPath);
        }
        outFileHeader->sstrs += size;
        outFileHeader->nlibs++;

        library = library->next;
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


void writeSymbols(EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile,
                   char *outputPath) {
    outFileHeader->osyms = *outFileOffset;
    outFileHeader->nsyms = 0;

    Symbol *entry;
    SymbolRecord symbolRecord;
    kh_foreach_value(getGst(), entry, {
        if (!entry->isReferenced && (entry->attr & SYM_ATTR_X)) {
            continue;
        }

        entry->outputNumber = outFileHeader->nsyms;

        symbolRecord.name = entry->nameOffs;
        symbolRecord.val = entry->val;
        symbolRecord.seg = entry->seg;
        if (entry->attr & SYM_ATTR_X) {
            symbolRecord.attr = SYM_ATTR_U;
        } else {
            symbolRecord.attr = entry->attr;
        }

        conv4FromNativeToEco((unsigned char *) &symbolRecord.name);
        conv4FromNativeToEco((unsigned char *) &symbolRecord.val);
        conv4FromNativeToEco((unsigned char *) &symbolRecord.seg);
        conv4FromNativeToEco((unsigned char *) &symbolRecord.attr);

        if (fwrite(&symbolRecord, sizeof(SymbolRecord), 1, outputFile) != 1) {
            error("cannot write symbol %s to file '%s'", entry->name, outputPath);
        }

        outFileHeader->nsyms++;
    });

    *outFileOffset += sizeof(SymbolRecord) * outFileHeader->nsyms;
}


void writeSegmentsTotal(EofHeader *outFileHeader, TotalSegment *totalSeg, FILE *outputFile, char *outputPath) {
    while (totalSeg != NULL) {
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


void writeW32Relocation(unsigned int loc, Symbol *symbol, EofHeader *outFileHeader, FILE *outputFile, char *outputPath) {
    RelocRecord relocRecord;
    relocRecord.loc = loc;
    relocRecord.seg = -1;
    relocRecord.typ = RELOC_W32 | RELOC_SYM;
    relocRecord.ref = symbol->outputNumber;
    relocRecord.add = 0;

    conv4FromNativeToEco((unsigned char *) &relocRecord.loc);
    conv4FromNativeToEco((unsigned char *) &relocRecord.seg);
    conv4FromNativeToEco((unsigned char *) &relocRecord.typ);
    conv4FromNativeToEco((unsigned char *) &relocRecord.ref);
    conv4FromNativeToEco((unsigned char *) &relocRecord.add);

    if (fwrite(&relocRecord, sizeof(RelocRecord), 1, outputFile) != 1) {
        error("cannot write loader relocation to file '%s'", outputPath);
    }

    outFileHeader->nrels++;
}


void writeERW32Relocation(unsigned int loc, EofHeader *outFileHeader, FILE *outputFile, char *outputPath) {
    RelocRecord relocRecord;
    relocRecord.loc = loc;
    relocRecord.seg = -1;
    relocRecord.typ = RELOC_ER_W32;
    relocRecord.ref = -1;
    relocRecord.add = 0;

    conv4FromNativeToEco((unsigned char *) &relocRecord.loc);
    conv4FromNativeToEco((unsigned char *) &relocRecord.seg);
    conv4FromNativeToEco((unsigned char *) &relocRecord.typ);
    conv4FromNativeToEco((unsigned char *) &relocRecord.ref);
    conv4FromNativeToEco((unsigned char *) &relocRecord.add);

    if (fwrite(&relocRecord, sizeof(RelocRecord), 1, outputFile) != 1) {
        error("cannot write loader relocation to file '%s'", outputPath);
    }

    outFileHeader->nrels++;
}


void writeRelocations(Segment *gotSegment, EofHeader *outFileHeader, unsigned int *outFileOffset, FILE *outputFile, char *outputPath) {
    outFileHeader->orels = *outFileOffset;
    outFileHeader->nrels = 0;

    for (int i = 0; i < gotSegment->size; i+=4) {
        writeERW32Relocation(gotSegment->addr + i, outFileHeader, outputFile, outputPath);
    }

    for (int i = 0; i < w32Count; i ++) {
        writeERW32Relocation(w32Addresses[i], outFileHeader, outputFile, outputPath);
    }

    *outFileOffset += sizeof(RelocRecord) * outFileHeader->nrels;
}


void writeFinalHeader(int isLibrary, unsigned int codeEntry, EofHeader *outFileHeader, FILE *outputFile, char *outputPath) {
    if (fseek(outputFile, 0, SEEK_SET) != 0) {
        error("cannot seek final header in file '%s'", outputPath);
    }

    outFileHeader->magic = isLibrary ? EOF_D_MAGIC : EOF_X_MAGIC;
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
    conv4FromNativeToEco((unsigned char *) &outFileHeader->olibs);
    conv4FromNativeToEco((unsigned char *) &outFileHeader->nlibs);

    if (fwrite(outFileHeader, sizeof(EofHeader), 1, outputFile) != 1) {
        error("cannot write output file final header to file '%s'", outputPath);
    }
}
