//
// Created by tony on 27.04.21.
//

#ifndef ECO32_ECOENDIAN_H
#define ECO32_ECOENDIAN_H


#include <stdint.h>


/**************************************************************/
/** ECO32 converter methods **/


uint32_t read4FromEco(unsigned char *p);
void write4ToEco(unsigned char *p, uint32_t data);
void conv4FromEcoToNative(unsigned char *p);
void conv4FromNativeToEco(unsigned char *p);


#endif //ECO32_ECOENDIAN_H
