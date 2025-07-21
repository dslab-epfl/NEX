#include <bits/stdint-uintn.h>
#include <fcntl.h>
#include <linux/vfio.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <unistd.h>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <fstream>
#include <memory>
#include <vector>
#include <iomanip>
#include <stdint.h>
#include <cstring>

#include <drivers/jpegdecoder/jpegdecoder_driver.h>
#define DEBUG 0

// stolen from Vitis examples
template <typename T> struct aligned_allocator {
  using value_type = T;
  aligned_allocator() {}
  aligned_allocator(const aligned_allocator &) {}
  template <typename U> aligned_allocator(const aligned_allocator<U> &) {}
  T *allocate(std::size_t num) {
    void *ptr = nullptr;
    {
      if (posix_memalign(&ptr, 4096, num * sizeof(T)))
        throw std::bad_alloc();
    }
    return reinterpret_cast<T *>(ptr);
  }
  void deallocate(T *p, std::size_t num) {
    free(p);
  }
};

void wait_for_decode() {
  while (!decode_done()) {
    usleep(10);
    // std::this_thread::yield();
  }
}

int launch_decode(void* image_data, uint32_t image_size, void* dst) {
  decode_jpeg(image_data, image_size, dst);
  return 0;
}

auto loadImage(const std::string &path) {
  auto numBytes = std::ifstream(path, std::ios::binary | std::ios::ate).tellg();
  std::vector<uint8_t, aligned_allocator<uint8_t>> rawBytes(numBytes, 0);
  std::ifstream(path, std::ios::binary).read(reinterpret_cast<char*>(rawBytes.data()), numBytes);
  return rawBytes;
}

int main(int argc, char *argv[]) {
  initialize_jpeg_decoder();
  int img_idx = 0;
  char path[100]={0};
  void* inputHostBuffer = malloc(1024*1024*48);
  void* outputHostBuffer = malloc(1024*1024*48);
  
  //sprintf(path, "./test/data/%d.jpg", img_idx_list[0]);
  //auto jpegImage = loadImage(path);
  std::chrono::steady_clock::time_point begin =
  std::chrono::steady_clock::now();
  int img_idx_list[10] = {1, 5, 6, 8, 9, 13, 14, 15, 16, 20};
  while(1){
    sprintf(path, "./test/data/%d.jpg", img_idx_list[img_idx]);
    img_idx++;
    printf("path %s\n", path);
    auto jpegImage = loadImage(path);
    std::cout << "img sizes " << jpegImage.size() << std::endl;
    std::memcpy(inputHostBuffer, reinterpret_cast<uint32_t*>(jpegImage.data()), jpegImage.size());

    launch_decode(inputHostBuffer, jpegImage.size(), outputHostBuffer);
    wait_for_decode();

    if(img_idx >= 2){
      break;
    }
  }

  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

  std::cout << "info: submitting image to jpeg decoder\n";
#if DEBUG
  verilator_regs.tracing_active = true;
#endif
  

  // wait until decoding finished

  // report duration

#if DEBUG
  verilator_regs.tracing_active = false;
#endif

  std::cout << "duration: "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin)
                   .count()
            << " ns\n";

  return 0;
}
