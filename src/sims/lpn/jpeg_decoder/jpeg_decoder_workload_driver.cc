#include <fcntl.h>
#include <linux/vfio.h>
#include <sys/mman.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <thread>

#include "include/jpeg_regs.hh"
#include "include/vfio.hh"

#define DEBUG 0

int main(int argc, char *argv[]) {
  if (argc != 5) {
    std::cerr << "usage: jpeg_decoder_workload PCI-DEVICE DMA-SRC DMA-SRC-LEN "
                 "DMA-DST\n";
    return EXIT_FAILURE;
  }

  int vfio_fd = vfio_init(argv[1]);
  if (vfio_fd < 0) {
    std::cerr << "vfio init failed" << std::endl;
    return 1;
  }

  void *bar0;
  size_t reg_len;
  if (vfio_map_region(vfio_fd, 0, &bar0, &reg_len)) {
    std::cerr << "vfio_map_region for bar 0 failed" << std::endl;
    return 1;
  }

#if DEBUG
  void *bar1;
  if (vfio_map_region(vfio_fd, 1, &bar1, &reg_len)) {
    std::cerr << "vfio_map_region for bar 1 failed" << std::endl;
    return 1;
  }
#endif

  // required for DMA
  if (vfio_busmaster_enable(vfio_fd)) {
    std::cerr << "vfio busmiaster enable faled" << std::endl;
    return 1;
  }

  volatile JPEGRegs &jpeg_decoder_regs =
      *static_cast<volatile JPEGRegs *>(bar0);
#if DEBUG
  volatile VerilatorRegs &verilator_regs =
      *static_cast<volatile VerilatorRegs *>(bar1);
#endif

  if (jpeg_decoder_regs.isBusy) {
    std::cerr << "error: jpeg decoder is unexpectedly busy\n";
    return 1;
  }

  uintptr_t dma_src_addr = std::stoul(argv[2], nullptr, 0);
  uint32_t dma_src_len = std::stoul(argv[3], nullptr, 0);
  uintptr_t dma_dst_addr = std::stoul(argv[4], nullptr, 0);

  // submit image to decode
  std::cout << "info: submitting image to jpeg decoder\n";
#if DEBUG
  verilator_regs.tracing_active = true;
#endif
  std::chrono::steady_clock::time_point begin =
      std::chrono::steady_clock::now();
  jpeg_decoder_regs.src = dma_src_addr;
  jpeg_decoder_regs.dst = dma_dst_addr;

  // invoke accelerator
  jpeg_decoder_regs.ctrl = dma_src_len | CTRL_REG_START_BIT;

  // wait until decoding finished
  while (jpeg_decoder_regs.isBusy) {
    // usleep(100);
    std::this_thread::yield();
  }

  // report duration
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

#if DEBUG
  verilator_regs.tracing_active = false;
#endif

  std::cout << "duration: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin)
                   .count()
            << " ns\n";

  return 0;
}