#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#define THREAD_COUNT 1000             // Number of concurrent threads
#define TOTAL_WRITES 1000          // Total number of writes across all threads
#define PER_WRITES 1000
// ---------------------------
// 1) Driver initialization
// ---------------------------
uintptr_t driver_initialize() {
    // In your original snippet, this always returns 1 and then
    // writes a magic value into some control register. We'll keep it.
    // (In real use you'd read ACCVM_MMIO_BASE and mmap a device, etc.)
    return 1;
}

// ---------------------------
// 2) Global variables
// ---------------------------
static uintptr_t base_address = 0;
static uintptr_t write_address = 0;
static int start_run = 0;
/*
 * Mutex to protect MMIO writes. Because we are writing to the exact same
 * 64-bit MMIO location, we must serialize access so that only one thread
 * does “*(volatile uint64_t*)write_address = …” at a time.
 */
static pthread_mutex_t mmio_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Per-thread argument. We pass a unique thread index (0..THREAD_COUNT-1).
 * Each thread will compute how many writes it should do: either
 * floor(TOTAL_WRITES/THREAD_COUNT) or that + 1 for the last thread.
 */
typedef struct {
    int thread_id;
} thread_arg_t;

// ---------------------------
// 3) Thread function
// ---------------------------
void* thread_func(void *arg) {
//while(!start_run);

    thread_arg_t *targ = (thread_arg_t*)arg;
    int tid = targ->thread_id;

    // Calculate how many writes this thread should do
    int base_chunk = TOTAL_WRITES / THREAD_COUNT;
    int remainder = TOTAL_WRITES % THREAD_COUNT;
    int this_count = base_chunk + ((tid == THREAD_COUNT - 1) ? remainder : 0);

    // The “starting index” in the 0..TOTAL_WRITES-1 range for this thread
    int start_idx = tid * base_chunk;
    // If tid == last, we automatically cover the remainder at the end

    for (int i = 0; i < PER_WRITES; i++) {
        uint64_t value = 0xdeadbeef;//  + (uint64_t)(start_idx + i);

        // Lock before each MMIO write
        //pthread_mutex_lock(&mmio_lock);

        // This is the actual MMIO write
        *(volatile uint64_t*)write_address = value;

        //pthread_mutex_unlock(&mmio_lock);
    }
    printf("I am done %d\n", tid);

    return NULL;
}

// ---------------------------
// 4) Main: set up, spawn threads, measure, join
// ---------------------------
int main() {
    // 4.1 Initialize the driver (exactly as before)
    base_address = driver_initialize();
    if (base_address == 0) {
        printf("Driver initialization failed or TURN_ON_REGS is not defined.\n");
        return 1;
    }
    printf("Base address initialized: 0x%lx\n", (unsigned long)base_address);

    // 4.2 Compute write_address just like in your original code
    uintptr_t offset = 64; // same example offset
    write_address = base_address + offset;

    printf("Write address: 0x%lx\n", (unsigned long)write_address);
    printf("Spawning %d threads to do %d total writes (protected by a mutex)…\n",
           THREAD_COUNT, TOTAL_WRITES);

    // 4.3 Prepare thread handles and args
    pthread_t threads[THREAD_COUNT];
    thread_arg_t targs[THREAD_COUNT];

    // 4.4 Record start time
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // 4.5 Spawn threads
    for (int t = 0; t < THREAD_COUNT; t++) {
        targs[t].thread_id = t;
        if (pthread_create(&threads[t], NULL, thread_func, &targs[t]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
        printf("Thread %d created.\n", t);
    }
    
    usleep(10);
    start_run = 1;

    // 4.6 Wait for threads to finish
    for (int t = 0; t < THREAD_COUNT; t++) {
        pthread_join(threads[t], NULL);
    }

    // 4.7 Record end time
    clock_gettime(CLOCK_MONOTONIC, &end);

    // 4.8 Compute elapsed time in microseconds
    double elapsed_us = (end.tv_sec - start.tv_sec) * 1e6 +
                        (end.tv_nsec - start.tv_nsec) / 1e3;

    printf("%d total writes (across %d threads) completed in %.2f μs.\n",
           TOTAL_WRITES, THREAD_COUNT, elapsed_us);
    printf("Average per-write: %.4f μs\n", elapsed_us / TOTAL_WRITES);

    return 0;
}
