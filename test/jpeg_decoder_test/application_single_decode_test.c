#include <assert.h>
#include <drivers/jpegdecoder/jpeg_decoder_driver.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
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

void custom_sleep_us(uint64_t us) {
   uint64_t start = get_microseconds();
   while(get_microseconds() - start < us); 
}

void load_image(uint8_t** image_data, uint32_t* image_size, const char* filename){
    FILE *f = fopen(filename, "rb");
    if (f) {
        long size;

        // Get size
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        rewind(f);

        // Read file data in
        *image_data = (uint8_t *)malloc(size);
        *image_size = fread(*image_data, 1, size, f);

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

int main() {

  initialize_jpeg_decoder();

  printf("--- Starting app ---\n");

  uint8_t *image_data = NULL;
  uint32_t image_size;
  void *dst = calloc(JPEG_SIZE, 1);
  char path[100]={0};

  double tic = get_real_time(); // Real time
  // clock_t t1 = clock();
  int img_idx = 0;
  int img_idx_list[36] = {1,13,14,15,16,20,21,22,23,24,25,26,27,29,32,33,34,36,37,39,40,41,42,43,44,45,46,47,48,49,5,51,52,6,8,9};
  while(1){
    sprintf(path, "./test/data/%d.jpg", img_idx_list[img_idx]);
    // sprintf(path, "./test/data/%s.jpg", "small");
    // sprintf(path, "./test/data/small.jpg");
    uint64_t start = get_microseconds(); // Virtual time
    // "Decode" the image using the JPEG decoder driver
    load_image(&image_data, &image_size, path);
    decode_jpeg(image_data, image_size, dst);
    // printf("Decoding image %d\n", img_idx_list[img_idx]);
    while (!decode_done()) {
      // Wait for the decoder to finish
      custom_sleep_us(100);
      // printf("time now %ld\n", get_microseconds());
    }
    uint64_t end = get_microseconds();
    printf("CPU time: %lu us\n", end - start);
    img_idx++;
    if(img_idx >= 36){
      break;
    }
  }
  // clock_t t2 = clock();

  double toc = get_real_time();
  printf("Real time: %f\n", toc - tic);
  // dump_decoded(dst, "./test/data/dump.ppm");
  free(dst);
  // printf("Clock time: %f\n", (t2 - t1) / (double)CLOCKS_PER_SEC);
  printf("--- App done ---\n");

  return 0;
}
