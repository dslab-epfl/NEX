#ifndef JPEG_DECODER_DRIVER_H
#define JPEG_DECODER_DRIVER_H
#include <stddef.h>
#include "jpeg_decoder_regs.h"

#define JPEG_DECODER_MMIO_IMAGE_ADDR (JPEG_DECODER_MMIO_BASE + 0x00)
#define JPEG_DECODER_MMIO_IMAGE_SIZE (JPEG_DECODER_MMIO_BASE + 0x08)

uintptr_t initialize_jpeg_decoder(void);
void deinit_jpeg_decoder(void);
uintptr_t initialize_jpeg_dma_on_device(int i);
void decode_jpeg(uintptr_t dma_base, uintptr_t image, size_t size, void* dst);
int decode_done(void);

void decode_jpeg_on_device(int dev_id, uintptr_t dma_base, uintptr_t image, size_t size, void *dst);
int decode_done_on_device(int dev_id);

#endif
