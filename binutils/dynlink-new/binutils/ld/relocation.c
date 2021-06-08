//
// Created by tony on 29.05.21.
//

#include "relocation.h"


void relocate(Segment *gotSegment) {
    Module *module = firstModule();

    while (module != NULL) {
#ifdef DEBUG
        debugPrintf("  module '%s'", module->name);
#endif
        for (int i = 0; i < module->nrels; i++) {
            Reloc *reloc = &module->rels[i];
            // Where to relocate
            Segment *relocSegment = &module->segs[reloc->seg];
            unsigned char *relocTarget = relocSegment->data + reloc->loc;
            unsigned int relocAddress = relocSegment->addr + reloc->loc;
            // Debugging strings
            char *relocType, *relocSymType, *relocSymRef;

            unsigned int refAddress, mask;
            unsigned int pc = relocAddress + 4;

            // What to relocate
            if (reloc->typ & RELOC_SYM) {
                // Symbol relocation
                refAddress = module->syms[reloc->ref]->val;
                relocSymType = "SYM";
                relocSymRef = module->syms[reloc->ref]->name;
            } else if ((reloc->typ & ~RELOC_SYM) == RELOC_GA_H16
                    || (reloc->typ & ~RELOC_SYM) == RELOC_GA_L16) {
                // Got address relocation
                refAddress = gotSegment->addr;
                relocSymType = "GOT";
                relocSymRef = safeAlloc(16);
                sprintf(relocSymRef, "@ 0x%08X", gotSegment->addr);
            } else {
                // Segment relocation
                refAddress = module->segs[reloc->ref].addr;
                relocSymType = "SEG";
                relocSymRef = module->segs[reloc->ref].name;
            }

#ifdef DEBUG
            debugPrintf("    %s @ 0x%08X (vaddr @ 0x%08X), data: 0x%08X",
                        relocSegment->name, reloc->loc, relocAddress, read4FromEco(relocTarget));
#endif
            int refValue = refAddress + reloc->add;
            Symbol *symbol;

            // How to relocate
            switch (reloc->typ & ~RELOC_SYM) {
                case RELOC_H16:
                    if (picMode) {
                        error("illegal H16 relocation in PIC mode");
                    }
                    refValue >>= 16;
                    mask = 0x0000FFFF;
                    relocType = "H16";
                    break;
                case RELOC_L16:
                    if (picMode) {
                        error("illegal L16 relocation in PIC mode");
                    }
                    mask = 0x0000FFFF;
                    relocType = "L16";
                    break;
                case RELOC_R16:
                    //TODO: throw error if on link unit extern symbol
                    refValue -= pc;
                    refValue /= 4;

                    if (((refValue >> 16) != 0) &&
                        ((refValue >> 16) & 0x3FFF) != 0x3FFF
                    ) {
                        error("relocation jump address out of R16 range: %d", refValue);
                    }

                    mask = 0x0000FFFF;
                    relocType = "R16";
                    break;
                case RELOC_R26:
                    //TODO: throw error if on link unit extern symbol
                    refValue -= pc;
                    refValue /= 4;

                    if ((refValue >> 26) != 0 &&
                        ((refValue >> 26) & 0x0F) != 0x0F
                    ) {
                        error("relocation jump address out of R26 range: %X", refValue);
                    }

                    mask = 0x03FFFFFF;
                    relocType = "R26";
                    break;
                case RELOC_W32:
                    mask = 0xFFFFFFFF;
                    relocType = "W32";
                    break;
                case RELOC_GA_H16:
                    refValue -= relocAddress;
                    refValue >>= 16;

                    mask = 0x0000FFFF;
                    relocType = "GA_H16";
                    break;
                case RELOC_GA_L16:
                    refValue -= relocAddress;

                    mask = 0x0000FFFF;
                    relocType = "GA_L16";
                    break;
                case RELOC_GR_H16:
                    refValue -= gotSegment->addr;
                    refValue >>= 16;

                    mask = 0x0000FFFF;
                    relocType = "GR_H16";
                    break;
                case RELOC_GR_L16:
                    refValue -= gotSegment->addr;

                    mask = 0x0000FFFF;
                    relocType = "GR_L16";
                    break;
                case RELOC_GP_L16:
                    symbol = module->syms[reloc->ref];
                    refValue = getOffsetFromGot(symbol);

                    write4ToEco(&gotSegment->data[refValue], symbol->val);

                    mask = 0x0000FFFF;
                    relocType = "GP_L16";
                    break;
                default:
                    error("unknown relocation type '%d'", reloc->typ);
            }
            write4ToEco(relocTarget, (refValue & mask) | (read4FromEco(relocTarget) & ~mask));
#ifdef DEBUG
            debugPrintf("      %s with %s %s + 0x%08X: 0x%08X",
                        relocType, relocSymType, relocSymRef, reloc->add,
                        (refValue & mask) | (read4FromEco(relocTarget) & ~mask));
#endif
        }

        module = module->next;
    }
}
