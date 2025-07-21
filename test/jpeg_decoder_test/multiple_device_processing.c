#include <assert.h>
#include <drivers/jpegdecoder/jpeg_decoder_driver.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
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
    int pid = syscall(SYS_gettid);
    *(uint64_t*)mmio_base = ((uint64_t)pid << 32) | 0x5000;
    tick_nex();
}

void inline nex_end_jail_break(){
    int pid = syscall(SYS_gettid);
    *(uint64_t*)mmio_base =  ((uint64_t)pid << 32) | 0x6000;
    tick_nex();
}

void inline nex_virtual_speedup(int percentage){
    int pid = syscall(SYS_gettid);
    *(uint64_t*)mmio_base =  ((uint64_t)pid << 32) | 0x3000 | (percentage & 0x00FF);
    tick_nex();
}

void inline nex_end_virtual_speedup(){
    int pid = syscall(SYS_gettid);
    *(uint64_t*)mmio_base =  ((uint64_t)pid << 32) | 0x4000;
    tick_nex();
}

double get_real_time() {
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return now.tv_sec + now.tv_nsec / 1000000000.0;
}


uint64_t get_real_ns() {
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return now.tv_sec*1000000000 + now.tv_nsec;
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

void load_image(uint8_t* image_data, uint32_t* image_size, const char* filename) {
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

// New function: Matrix kernel filtering simulation.
// This function applies a simple 3-value sliding window average over the output buffer.
// For simplicity, we treat the image as a one-dimensional array.
// Function to compute the number of memory accesses for matrix_kernel_filter_2d.
// For each pixel in the inner (height - 2) x (width - 2) region, there are:
// 9 memory reads + 1 memory write = 10 memory accesses.
uint64_t count_memory_accesses(uint32_t width, uint32_t height) {
    // If the image is too small for a 3x3 kernel, no processing occurs.
    if (width < 3 || height < 3) {
        return 0;
    }
    uint64_t inner_rows = height - 2;
    uint64_t inner_cols = width - 2;
    uint64_t total_pixels = inner_rows * inner_cols;
    
    // Each inner pixel does 10 memory accesses.
    return total_pixels;
}

// Two-dimensional matrix kernel filtering using a 3x3 average filter.
void matrix_kernel_filter_2d(uint8_t *buffer, uint32_t width, uint32_t height) {
    // Allocate a temporary buffer to hold a copy of the original data.
    uint8_t *temp = buffer;

    // Process the image while avoiding the border pixels.
    for (uint32_t row = 1; row < height - 1; row++) {
        for (uint32_t col = 1; col < width - 1; col++) {
            uint32_t sum = 0;
            // Sum all values in the 3x3 neighborhood.
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    sum += temp[(row + i) * width + (col + j)];
                }
            }
            // Assign the average to the current pixel.
            buffer[row * width + col] = sum / 9;
        }
    }
}


int compute_max_speedup(uint8_t *buffer, uint32_t width, uint32_t height){
    uint64_t start = get_real_ns();
    matrix_kernel_filter_2d(buffer, width, height);
    uint64_t end = get_real_ns();
    uint64_t total_time = end - start;
    uint64_t memory_cnt = count_memory_accesses(width, height);
    uint64_t total_memory_time = memory_cnt * 1; // Assuming 1 ns per access

    uint64_t speedup = total_time / total_memory_time;
    printf("speedup %lu\n", 100 - 100/speedup);
    return 100 - 100/speedup;
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
        

        load_image(image_data, &image_size, path);
        // nex_end_virtual_speedup();

        void *output_buffer = image_data + image_size;

        // Launch decoding on the specific device.
        printf("Thread (device %d) decoding image \"%s\"\n", dev_id, path);

        uint64_t start = get_microseconds();
        decode_jpeg_on_device(dev_id, dma_start[dev_id], (uintptr_t)image_data, image_size, output_buffer);
        while (!decode_done_on_device(dev_id)) {
            custom_sleep_us(400);
        }
        #define FIX_ADJ 2048
        
        uint64_t end = get_microseconds();
        // free(image_data);
        printf("Thread (device %d) finished decoding image \"%s\", duration %lu \n", dev_id, path, end-start);
        uint32_t width = image_size/100 > FIX_ADJ ? FIX_ADJ : image_size/100; 
        printf("value of width and height %d, %d\n", width, width);
        matrix_kernel_filter_2d((uint8_t*)output_buffer, width, width); // Assuming a fixed width and height for simplicity.
        uint64_t end_post_processing = get_microseconds();
        printf("Thread (device %d) finished post_processing, duration %lu \n", dev_id, end_post_processing - end);


        // New processing stage: Matrix kernel filtering.

        // printf("Thread (device %d) finished decoding image \"%s\". Now applying matrix kernel filtering...\n", dev_id, path);
        // uint64_t filter_start = get_microseconds();
        // nex_jail_break();
        // int speedup = compute_max_speedup((uint8_t*)output_buffer, image_size/100, image_size/100); // Assuming a fixed width and height for simplicity.
        // nex_end_jail_break();
        // uint64_t jail_break_end = get_microseconds();
        // printf("Thread (device %d) finished jailbreaking in %lu us\n", dev_id, jail_break_end - filter_start);

        // nex_virtual_speedup(speedup);
        // uint32_t width = image_size/100 > FIX_ADJ ? FIX_ADJ : image_size/100; 
        // printf("value of width and height %d, %d\n", width, width);
        // matrix_kernel_filter_2d((uint8_t*)output_buffer, width, width); // Assuming a fixed width and height for simplicity.
        // nex_end_virtual_speedup();
        // uint64_t filter_end = get_microseconds();
        // printf("Thread (device %d) finished matrix filtering in %lu us\n", dev_id, filter_end - filter_start);
    }
    return NULL;
}

#ifdef __cplusplus
extern "C" {
#endif

// Exported function to decode multiple JPEG images concurrently.
int decode_images_multithreaded(const char **input_paths, void **output_buffers, int num_images, int num_threads, uint32_t output_buffer_size) {
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
#ifdef TEST_MAIN
int main() {
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
    
    int num_threads = 1;

    // Initialize the JPEG decoder driver.
    mmio_base = initialize_jpeg_decoder();
    for (int i = 0; i < num_threads; i++){
        dma_start[i] = initialize_jpeg_dma_on_device(i);
    }

    double tic = get_real_time(); // Real time
    uint64_t start = get_microseconds();
    printf("start time %lu\n", start);
    int ret = decode_images_multithreaded(images, output_buffers, num_images, num_threads, JPEG_SIZE);
    if (ret != 0) {
        printf("Decoding failed\n");
    } else {
        printf("Decoding succeeded\n");
    }
    uint64_t end = get_microseconds();
    double toc = get_real_time();
    deinit_jpeg_decoder();

    printf("end time %lu\n", end);
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
