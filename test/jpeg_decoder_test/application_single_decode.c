#include <assert.h>
#include <drivers/jpegdecoder/jpeg_decoder_driver.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

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

void _custom_sleep_us(uint64_t us) {
   uint64_t start = get_microseconds();
   while(get_microseconds() - start < us); 
}
#include <signal.h>

static volatile sig_atomic_t interruptReceived = 0;

// Simple SIGUSR1 handler: just set a flag
static void sigusr1_handler(int sig) {
    (void)sig; // unused
    interruptReceived = 1;
}

// Instead of busy-wait, block until a SIGUSR1 arrives
void custom_sleep_us(uint64_t us) {
  // If you truly need to “sleep” for a certain time, you must arrange
  // something (e.g., a timer) to send SIGUSR1 after ‘us’ microseconds.
  // Otherwise, this just blocks until the next SIGUSR1.
  // (void)us;  // ignoring the sleep duration in this example

  // Reset the flag and block until interrupt
  // interruptReceived = 0;
  // while (!interruptReceived) {
  _custom_sleep_us(us);
  //}
}

void load_image(uint8_t* image_data, uint32_t* image_size, const char* filename){
    FILE *f = fopen(filename, "rb");
    if (f) {
        long size;

        // Get size
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        rewind(f);

        // Read file data in
        *image_size = fread(image_data, 1, size, f);

        printf("Successfully loaded image with size: %d\n", *image_size);
        fclose(f);
    } else {
        printf("./jpeg src_image.jpg dst_image.ppm\n");
        assert(0);
    }
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
static uintptr_t dma_start;

int main() {

  struct sigaction sa;
  sa.sa_handler = sigusr1_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGUSR1, &sa, NULL);

  initialize_jpeg_decoder();

  dma_start = initialize_jpeg_dma_on_device(0);

  printf("--- Starting app ---\n");

  uint8_t *image_data = (uint8_t*)dma_start;
  uint32_t image_size;
  void *dst;
  char path[100]={0};

  double tic = get_real_time(); // Real time
  // clock_t t1 = clock();
  int img_idx = 0;
  int total = 42;
  int img_idx_list[total] = {0,1,10,13,14,15,16,2,20,21,22,23,24,25,26,27,29,3,32,33,34,36,37,39,4,40,41,42,43,44,45,46,47,48,49,5,51,52,6,7,8,9};
  while(1){
    sprintf(path, "./test/data/%d.jpg", img_idx_list[img_idx]);
    uint64_t start = get_microseconds(); // Virtual time
    // "Decode" the image using the JPEG decoder driver
    load_image(image_data, &image_size, path);
    dst = image_data + image_size;
    decode_jpeg(dma_start, (uintptr_t)image_data, image_size, dst);
    printf("Decoding image %d\n", img_idx_list[img_idx]);
    while (!decode_done()) {
      custom_sleep_us(100);
    }
    uint64_t end = get_microseconds();
    printf("Time taken %lu us\n", end - start);
    img_idx++;
    if(img_idx >= total){
      break;
    }
  }
  // clock_t t2 = clock();

  double toc = get_real_time();
  printf("Real latency %f\n", toc - tic);
  // dump_decoded(dst, "./test/data/dump.ppm");
  // free(dst);
  // printf("Clock time: %f\n", (t2 - t1) / (double)CLOCKS_PER_SEC);
  printf("--- App done ---\n");

  return 0;
}
