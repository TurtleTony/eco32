//
// Created by tony on 27.04.21.
//

#include "ecoEndian.h"

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