#include <assert.h>
#include <bits/stdint-uintn.h>
#include <drivers/jpegdecoder/jpegdecoder_driver.h>
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
#define NUM_IMGS 1

int main(){

  uint8_t *image_data = NULL;
  uint32_t image_size;


  initialize_jpeg_decoder();
  int processed_idx = 1;
  int inflight_idx = 0;
  void *buf1 = calloc(JPEG_SIZE, 1);
  void *buf2 = calloc(JPEG_SIZE, 1);
  void* two_buf[2] = {buf1, buf2};
  
  int have_processed_img = 0;
  double tic = get_real_time();
  char path[1024];
  char* base_dir = "./test/data/";
  uint64_t start = get_microseconds();

  while(1){
    
    sprintf(path, "%s/%d.jpg", base_dir, 10);
    printf("path %s\n", path);
    load_image(&image_data, &image_size, path);

    launch_decode(image_data, image_size, two_buf[inflight_idx]);
    // if(have_processed_img){
    //   void* dst = two_buf[processed_idx];
    //   int compute_sum = 0;
    //   for(int i=0; i<256; i++){
    //     if(i < 16){
    //       printf("%x ", ((uint8_t*)dst)[i]);
    //     }
    //     compute_sum += ((uint8_t*)dst)[i];
    //   }
    //   printf("Sum of decoded image: %d\n", compute_sum);
    //   have_processed_img = 0;
    // }
    // while(!finished_decode()){
    //   usleep(10);
    // }
    // have_processed_img = 1;
    // finished, swap processed_idx and inflight_idx
    int orig_pi = processed_idx;
    processed_idx = inflight_idx;
    inflight_idx = orig_pi;
    cnt++;
    if(cnt > NUM_IMGS)
      break;
  }

  uint64_t end = get_microseconds();
  double toc = get_real_time();
  printf("CPU time: %lu us, Real time: %f sec\n", end - start, toc - tic);
}