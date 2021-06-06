//
// Created by tony on 29.05.21.
//

#include "relocation.h"

void relocate(Segment *gotSegment) {
    Module *module = firstModule();

    while (module != NULL) {
        for (int i = 0; i < module->nrels; i++) {
            Reloc *reloc = &module->rels[i];
            // Where to relocate
            unsigned char *target = module->segs[reloc->seg].data + reloc->loc;
            unsigned int addr = module->segs[reloc->seg].addr + reloc->loc;

            unsigned int baseAddr, mask;
            unsigned int pc = addr + 4;

            // What to relocate
            if (reloc->typ & RELOC_SYM) {
                // Symbol relocation
                baseAddr = module->syms[reloc->ref]->val;
            } else if(reloc->ref == -1) {
                // Got address relocation
                baseAddr = gotSegment->addr;
            } else {
                // Segment relocation
                baseAddr = module->segs[reloc->ref].addr;
            }

            int value = baseAddr + reloc->add;

            // How to relocate
            switch (reloc->typ & ~RELOC_SYM) {
                case RELOC_H16:
                    value >>= 16;
                case RELOC_L16:
                    mask = 0x0000FFFF;
                    break;
                case RELOC_R16:
                    value -= pc;
                    value /= 4;

                    if (((value >> 16) != 0) &&
                        ((value >> 16) & 0x3FFF) != 0x3FFF
                    ) {
                        error("relocation jump address out of R16 range: %d", value);
                    }

                    mask = 0x0000FFFF;
                    break;
                case RELOC_R26:
                    value -= pc;
                    value /= 4;

                    if ((value >> 26) != 0 &&
                        ((value >> 26) & 0x0F) != 0x0F
                    ) {
                        error("relocation jump address out of R26 range: %x", value);
                    }

                    mask = 0x03FFFFFF;
                    break;
                case RELOC_W32:
                    mask = 0xFFFFFFFF;
                    break;
                case RELOC_GA_H16:
                    value -= pc;
                    value >>= 16;

                    mask = 0x0000FFFF;
                    break;
                case RELOC_GA_L16:
                    value -= pc;

                    mask = 0x0000FFFF;
                    break;
                case RELOC_GR_H16:
                    value -= gotSegment->addr;
                    value >>= 16;

                    mask = 0x0000FFFF;
                    break;
                case RELOC_GR_L16:
                    value -= gotSegment->addr;

                    mask = 0x0000FFFF;
                    break;
                case RELOC_GP_L16:

                    break;
                default:
                    error("unknown relocation type '%d'", reloc->typ);
            }
            write4ToEco(target, (value & mask) | (read4FromEco(target) & ~mask));

        }

        module = module->next;
    }
}