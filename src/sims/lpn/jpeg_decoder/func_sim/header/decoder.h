#include <stdint.h>

#ifndef DECODE_H
#define DECODE_H

#ifdef __cplusplus
extern "C" {
#endif

void decode_image(uint8_t* buf, uint32_t len, uint8_t* dst);

#ifdef __cplusplus
}
#endif

#endif 