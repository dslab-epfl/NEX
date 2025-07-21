#include <assert.h>
#include <drivers/jpegdecoder/jpeg_decoder_driver.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define JPEG_SIZE 221184170

static uintptr_t dma_start[40] = {0};
// Utility functions

static uintptr_t mmio_base = 0;
void inline tick_nex() {
    __asm__ volatile("ud2");
}
    
void inline nex_jail_break(){
    *(uint64_t*)mmio_base =  0x5000;
    tick_nex();
}

void inline nex_end_jail_break(){
    *(uint64_t*)mmio_base =  0x6000;
    tick_nex();
}

void inline nex_virtual_speedup(int percentage){
    *(uint64_t*)mmio_base =  0x3000 | (percentage & 0x00FF);
    tick_nex();
}

void inline nex_end_virtual_speedup(){
    *(uint64_t*)mmio_base =  0x4000;
    tick_nex();
}

double get_real_time() {
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
    while (get_microseconds() - start < us);
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

// Define a task that holds the image path and output buffer pointer.
typedef struct {
    const char *image_path;
    void *output_buffer;
} task_t;

// Global task queue variables.
static task_t *tasks = NULL;
static int num_tasks = 0;
static int current_task_index = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

// Worker thread function.
// Each thread uses its device id (passed via arg) to launch decoding tasks.
void *worker_thread(void *arg) {
    int dev_id = *(int *)arg;
    free(arg);  // Free the allocated memory for device id.
    
    while (1) {
        int task_idx = -1;
        pthread_mutex_lock(&queue_mutex);
        if (current_task_index < num_tasks) {
            task_idx = current_task_index++;
        }
        pthread_mutex_unlock(&queue_mutex);
        
        if (task_idx == -1) {
            break; // No more tasks.
        }
        
        // Process the task.
        const char *path = tasks[task_idx].image_path;
        
        uint8_t *image_data = (uint8_t*)dma_start[dev_id];
        uint32_t image_size = 0;
        
        // // Load the image.
        load_image(image_data, &image_size, path);

        void *output_buffer = image_data + image_size;

        // // Launch decoding on the specific device.
        printf("Thread (device %d) decoding image \"%s\"\n", dev_id, path);

        decode_jpeg_on_device(dev_id, dma_start[dev_id], (uintptr_t)image_data, image_size, output_buffer);
        while (!decode_done_on_device(dev_id)) {
            custom_sleep_us(400);
            // printf("Dev %d time now %ld\n", dev_id, get_microseconds());
        }
        
        // free(image_data);
        printf("Thread (device %d) finished decoding image \"%s\"\n", dev_id, path);
    }
    return NULL;
}

#ifdef __cplusplus
extern "C" {
#endif

// Exported function to decode multiple JPEG images concurrently.
// input_paths: an array of image file paths.
// output_buffers: an array of pointers to output buffers (each must be allocated to at least JPEG_SIZE bytes).
// num_images: number of images/tasks.
// num_threads: number of worker threads (and device ids) to use.
// output_buffer_size: size (in bytes) of each output buffer.
int decode_images_multithreaded(const char **input_paths, void **output_buffers, int num_images, int num_threads, uint32_t output_buffer_size) {
    // Check that the output buffers are sufficiently large.
    if (output_buffer_size < JPEG_SIZE) {
        fprintf(stderr, "ERROR: Output buffer is too small. Required size: %d bytes\n", JPEG_SIZE);
        return -1;
    }
    // Allocate and initialize the task queue.
    num_tasks = num_images;
    current_task_index = 0;
    tasks = (task_t *)malloc(sizeof(task_t) * num_tasks);
    if (!tasks) {
        fprintf(stderr, "ERROR: Failed to allocate task queue\n");
        return -1;
    }
    for (int i = 0; i < num_tasks; i++) {
        tasks[i].image_path = input_paths[i];
        tasks[i].output_buffer = output_buffers[i];
    }
    
    // Create worker threads.
    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
    if (!threads) {
        fprintf(stderr, "ERROR: Failed to allocate threads\n");
        free(tasks);
        return -1;
    }
    
    for (int i = 0; i < num_threads; i++) {
        int *dev_id = (int *)malloc(sizeof(int));
        *dev_id = i;  // Each thread gets a device id equal to its index.
        if (pthread_create(&threads[i], NULL, worker_thread, dev_id) != 0) {
            fprintf(stderr, "ERROR: Failed to create thread %d\n", i);
            free(threads);
            free(tasks);
            return -1;
        }
    }
    
    // Wait for all threads to finish.
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    free(threads);
    free(tasks);
    
    return 0;
}

#ifdef __cplusplus
}
#endif

#define TEST_MAIN
// A main function for testing purposes (can be removed when compiling as a shared library).
#ifdef TEST_MAIN
int main() {
    // Example image paths and output buffers.
    // In practice, these would be set up appropriately.
    int num_images = 36;

    int img_idx_list[36] = {1,13,14,15,16,20,21,22,23,24,25,26,27,29,32,33,34,36,37,39,40,41,42,43,44,45,46,47,48,49,5,51,52,6,8,9};
    const char *images[36];
    for (int i = 0; i < num_images; i++) {
        char path[200];
        sprintf(path, "./test/data/%d.jpg", img_idx_list[i]);
        images[i] = strdup(path);
    }
   
    void **output_buffers = (void **)malloc(sizeof(void *) * num_images);
    for (int i = 0; i < num_images; i++) {
        output_buffers[i] = calloc(JPEG_SIZE, 1);
    }
    
    int num_threads = 8;

    // Initialize the JPEG decoder driver.
    mmio_base = initialize_jpeg_decoder();

    for(int i=0; i<num_threads; i++){
        dma_start[i] = initialize_jpeg_dma_on_device(i);
    }

    // Use, for example, 4 worker threads.
    double tic = get_real_time(); // Real time
    uint64_t start = get_microseconds();
    printf("start time %ld\n", start);
    int ret = decode_images_multithreaded(images, output_buffers, num_images, num_threads, JPEG_SIZE);
    if (ret != 0) {
        printf("Decoding failed\n");
    } else {
        printf("Decoding succeeded\n");
    }
    uint64_t end = get_microseconds();
    double toc = get_real_time();
    deinit_jpeg_decoder();

    printf("end time %ld\n", end);
    printf("Total time: %lu us\n", end - start);
    printf("Real time: %f\n", toc - tic);
    // Cleanup output buffers.
    for (int i = 0; i < num_images; i++) {
        free(output_buffers[i]);
    }
    free(output_buffers);
    
    return 0;
}
#endif
