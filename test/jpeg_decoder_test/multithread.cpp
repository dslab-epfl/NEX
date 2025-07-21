#include <pthread.h>
#include <assert.h>
#include <bits/stdint-uintn.h>
#include <sys/time.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <iomanip>
#include <stdint.h>
#include <unistd.h>
#include <cstring>
#include <drivers/jpegdecoder/jpegdecoder_driver.h>

#define JPEG_SIZE 221184170
double get_real_time()
{
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return now.tv_sec + now.tv_nsec / 1000000000.0;
}

uint64_t get_microseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

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

// Maximum size of the decode image, needed to allocate DRAM ont he FPGA
static constexpr uint32_t MaxDecodeSize = 1 << 26; // 64 MiB
// read a jpeg image
auto loadImage(const std::string &path) {
  auto numBytes = std::ifstream(path, std::ios::binary | std::ios::ate).tellg();
  std::vector<uint8_t, aligned_allocator<uint8_t>> rawBytes(numBytes, 0);
  std::ifstream(path, std::ios::binary).read(reinterpret_cast<char*>(rawBytes.data()), numBytes);
  return rawBytes;
}

void dump_decoded(void* dst, const char* filename) {
  FILE *f = fopen(filename, "wb");
  if (f) {
    fwrite(dst, 1, JPEG_SIZE, f);
    fclose(f);
  } else {
    fprintf(stderr, "ERROR: Could not write file\n");
  }
}

int launch_decode(uint8_t* image_data, uint32_t image_size, void* dst) {

  printf("--- Starting app ---\n");
  // uint64_t start = get_microseconds(); // Virtual time
  // "Decode" the image using the JPEG decoder driver
  decode_jpeg(image_data, image_size, dst);
  return 0;
}

int finished_decode(){
  return decode_done();
}

int cnt = 0;
#define NUM_IMGS 10

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int decode_finished = 0;  // Protected by 'mutex'
volatile int decode_consumed = 1;  // Protected by 'mutex'
volatile uint8_t* buffer_dst = 0;  // Protected by 'mutex'

volatile int dummy;
void *process_thread_func(void *arg) {
    uint64_t start = get_microseconds();
    int cnt = 0;
    printf("process_thread_func running\n");
    while (cnt < NUM_IMGS) {
        pthread_mutex_lock(&mutex);
        while (!decode_finished) {
          pthread_mutex_unlock(&mutex);
          // usleep(100);
          for(int i = 0; i < 100; i++) {
            for(int j = 0; j < 100; j++) {
              dummy += i + j;
            }
          }
          pthread_mutex_lock(&mutex);
        }
        decode_finished = 0;
        pthread_mutex_unlock(&mutex);
        // Post-processing code here
        // usleep(10000);
        int compute_sum = 0;
        for (int i = 0; i < 256; i++) {
            compute_sum += ((uint8_t*)buffer_dst)[i];
        }

        // printf("Sum of decoded image: %d\n", compute_sum);

        // dump_decoded(dst, "./decoded_output.jpg");
        // Notify decode thread that processing is done
        pthread_mutex_lock(&mutex);
        decode_consumed = 1;
        pthread_mutex_unlock(&mutex);
        cnt ++;
    }
    uint64_t end = get_microseconds();
    double* res = (double*)calloc(1, sizeof(double));
    res[0] = end - start;
    return (void *) res;
}

void *launch_decode_thread_func(void* arg) {
    
    uint8_t *dst1 = (uint8_t*)malloc(JPEG_SIZE);
    uint8_t *dst2 = (uint8_t*)malloc(JPEG_SIZE);
    uint8_t *arr_dst[2] = {dst1, dst2};
    int arr_idx = 0;

    uint8_t *inputHostBuffer = (uint8_t*)malloc(JPEG_SIZE);
    int cnt = 0;
    uint64_t start = get_microseconds();
    while (cnt < NUM_IMGS) {
        // auto jpegImage = loadImage(std::string("./test/data/") + std::to_string(cnt) + ".jpg");
        auto jpegImage = loadImage(std::string("./test/data/") + "10" + ".jpg");
        memcpy(inputHostBuffer, jpegImage.data(), jpegImage.size());

        launch_decode(inputHostBuffer, jpegImage.size(), arr_dst[arr_idx]);

        while(!finished_decode()){
            // usleep(100);
            for(int i = 0; i < 100; i++) {
            for(int j = 0; j < 100; j++) {
              dummy += i + j;
            }
          }
        }

        pthread_mutex_lock(&mutex);
        while (!decode_consumed) {
          // printf("Waiting for data to be consumed\n");
          pthread_mutex_unlock(&mutex);
          usleep(100);
          pthread_mutex_lock(&mutex);
        }
        decode_consumed = 0;
        // pthread_mutex_unlock(&mutex);
        decode_finished = 1;
        printf("Decoded image finished %d \n", cnt);
        buffer_dst = arr_dst[arr_idx];
        arr_idx = 1 - arr_idx;
        pthread_mutex_unlock(&mutex);
        cnt++;
    }  
    uint64_t end = get_microseconds();
    double* res = (double*)calloc(1, sizeof(double));
    res[0] = end - start;
    return (void *) res;
}

int main() {
    initialize_jpeg_decoder();

    pthread_t decode_thread, postproc_thread;

    pthread_create(&decode_thread, NULL, launch_decode_thread_func, NULL);
    pthread_create(&postproc_thread, NULL, process_thread_func, NULL);

    double *res1;
    double *res2;
    pthread_join(decode_thread, (void **)(&res1));
    pthread_join(postproc_thread, (void **)(&res2));
    printf("Decode Thread Virtual CPU time: %f microseconds\n", res1[0]);
    printf("Process Thread Virtual CPU time: %f microseconds\n", res2[0]);
    free(res1);
    free(res2);
    return 0;
}
