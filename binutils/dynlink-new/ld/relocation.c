//
// Created by tony on 29.05.21.
//

#include "relocation.h"

void relocateModules(void) {
    Module *module = firstModule();

    while (module != NULL) {
        for (int i = 0; i < module->nrels; i++) {
            Reloc *reloc = &module->rels[i];
            // Where to relocate
            unsigned char *target = module->segs[reloc->seg].data + reloc->loc;
            unsigned int addr = module->segs[reloc->seg].addr + reloc->loc;

            unsigned int baseAddr, mask;

            // What to relocate
            if (reloc->typ & RELOC_SYM) {
                // Symbol relocation
                baseAddr = module->syms[reloc->ref]->val;
            } else {
                // Segment relocation
                baseAddr = module->segs[reloc->ref].addr;
            }

            uint32_t value = baseAddr + reloc->add;

            // How to relocate
            switch (reloc->typ & ~RELOC_SYM) {
                case RELOC_H16:
                    value >>= 16;
                case RELOC_L16:
                    mask = 0x0000FFFF;
                    write4ToEco(target, (value & mask) | (read4FromEco(target) & ~mask));
                    break;
                case RELOC_R16:
                    value -= (addr + 4) / 4;
                    value /= 4;

                    if (((value >> 16) != 0) &&
                        ((value >> 16) & 0xFFFF) != 0xFFFF
                    ) {
                        error("relocation jump address out of range: %d", value);
                    }

                    mask = 0x0000FFFF;
                    write4ToEco(target, (value & mask) | (read4FromEco(target) & ~mask));
                    break;
                case RELOC_R26:
                    value -= (addr + 4);
                    value /= 4;

                    if ((value >> 26) != 0 &&
                        ((value >> 26) & 0x3F) != 0x3F
                    ) {
                        error("relocation jump address out of range: %x", value);
                    }

                    mask = 0x03FFFFFF;
                    write4ToEco(target, (value & mask) | (read4FromEco(target) & ~mask));
                    break;
                case RELOC_W32:
                    write4ToEco(target, value);
                    break;
                default:
                    error("unknown relocation type '%d'", reloc->typ);
            }

        }

        module = module->next;
    }
}