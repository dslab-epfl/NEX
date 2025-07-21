#ifndef VTA_DRIVER_H
#define VTA_DRIVER_H
#include <stddef.h>
#include "regs.h"

#define VTA_MMIO_IMAGE_ADDR (VTA_MMIO_BASE + 0x00)
#define VTA_MMIO_IMAGE_SIZE (VTA_MMIO_BASE + 0x08)

void initialize_VTA(void);
void vta_start(const void* image, size_t size, void* dst);
int vta_done(void);
#endif
