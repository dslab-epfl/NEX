#include <drivers/jpegdecoder/jpeg_decoder_driver.h>
#include <drivers/sched/regs.h>
#include <drivers/driver_common.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

void tick_nex() {
  __asm__ volatile("ud2");
}

static void *JPEG_DECODER_MMIO_BASE = NULL;
static void *MMIO_BASE = NULL;
uintptr_t initialize_jpeg_decoder(void) {
  MMIO_BASE = (void *)driver_initialize();

  char* device = getenv("JPEG_DEVICE");
  int device_num = 0;
  if(device == NULL){
      printf("device not set, defaults to 0\n");
  }else{
    printf("device: %s\n", device);
    device_num = atoi(device);
  }
  JPEG_DECODER_MMIO_BASE = (MMIO_BASE + (10*JPEG_DECODER+device_num)*4096);

  SchedRegs* sched_reg = (SchedRegs *)MMIO_BASE;
  
  printf("jpeg driver sched_reg->lock %lx\n", sched_reg->lock);
  sched_ctrl_lock(sched_reg);

  sched_reg->ctrl = 0x1000;
  tick_nex();

  sched_ctrl_unlock(sched_reg);
  printf("jpeg driver sched_reg->unlock %lx\n", sched_reg->lock);

  return (uintptr_t)MMIO_BASE;
}

void deinit_jpeg_decoder() {
   SchedRegs* sched_reg = (SchedRegs *)MMIO_BASE;
   sched_ctrl_lock(sched_reg);
   sched_reg->ctrl = 0x2000;
   tick_nex();
   sched_ctrl_unlock(sched_reg);
}


uintptr_t initialize_jpeg_dma(void) {
  // Placeholder for any initialization code needed for the DMA
  return 0;
}

uintptr_t initialize_jpeg_dma_on_device(int i){
  char shm_dma_region[200] = {0};
  sprintf(shm_dma_region, "jpeg_nex_dma_%d", i);
  return open_shm_for_nex(shm_dma_region);
}

void decode_jpeg(uintptr_t dma_base, uintptr_t image, size_t size, void *dst) {
  printf("Decoding JPEG\n");
  // Simulating Register MMIO address
  JpegRegs *regs = (JpegRegs *)JPEG_DECODER_MMIO_BASE;

  printf("Addr: %ld\n", image);
  // Write src address
  regs->src = (uint64_t)(image-dma_base);
  tick_nex();
  printf("Dst: %p\n", dst);
  // Write destination address
  regs->dst = (uint64_t)(dst-dma_base);
  tick_nex();
  printf("Size: %lu\n", size | CTRL_REG_START_BIT);
  // Start decoding
  regs->ctrl = size | CTRL_REG_START_BIT;
  tick_nex();
}

int decode_done() {
  JpegRegs *regs = (JpegRegs *)JPEG_DECODER_MMIO_BASE;
  regs->ctrl = 0XFFFFFFFF;
  tick_nex();
  int ret = regs->ctrl & CTRL_REG_DONE_BIT;
  return ret;
}

void decode_jpeg_on_device(int dev_id, uintptr_t dma_base, uintptr_t image, size_t size, void *dst) {
  printf("Decoding JPEG\n");
  // Simulating Register MMIO address
  JpegRegs *regs = (JpegRegs *)(MMIO_BASE+(10*JPEG_DECODER+dev_id)*4096);

  printf("Addr: %lu\n", image);
  // Write src address
  regs->src = (uint64_t)(image-dma_base);
  tick_nex();

  printf("Dst: %p\n", dst);
  // // Write destination address
  regs->dst = (uint64_t)((uintptr_t)dst-dma_base);
  tick_nex();

  printf("Size: %lu\n", size | CTRL_REG_START_BIT);
  // // Start decoding
  regs->ctrl = size | CTRL_REG_START_BIT;
  tick_nex();

}

int decode_done_on_device(int dev_id) {
  JpegRegs *regs = (JpegRegs *)(MMIO_BASE+(10*JPEG_DECODER+dev_id)*4096);
  //to indicate a read
  regs->ctrl = 0XFFFFFFFF;
  tick_nex();
  int ret = regs->ctrl & CTRL_REG_DONE_BIT;
  return ret;
}