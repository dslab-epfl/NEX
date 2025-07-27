#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <inttypes.h>

static uint64_t matmul(uint64_t a, uint64_t b) {
    // Matrix multiplication that runs for approximately 1ms
    const int size = 200;  // 200x200 matrices
    const int iterations = 10;  // Repeat to reach ~1ms
    
    // Allocate matrices on stack (small enough for stack)
    static uint32_t A[200][200];
    static uint32_t B[200][200];
    static uint32_t C[200][200];
    
    // Initialize matrices with values derived from a and b
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            A[i][j] = (uint32_t)((a + i * j) & 0xFFFFFFFF);
            B[i][j] = (uint32_t)((b + j * i) & 0xFFFFFFFF);
            C[i][j] = 0;
        }
    }
    
    uint64_t result = 0;
    
    // Perform matrix multiplication multiple times
    for (int iter = 0; iter < iterations; iter++) {
        // Standard matrix multiplication: C = A * B
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                uint64_t sum = 0;
                for (int k = 0; k < size; k++) {
                    sum += (uint64_t)A[i][k] * B[k][j];
                }
                C[i][j] = (uint32_t)(sum & 0xFFFFFFFF);
                result ^= sum;  // XOR to prevent optimization
            }
        }
        
        // Add some variation to prevent compiler optimization
        if (iter % 10 == 0) {
            for (int i = 0; i < size; i += 10) {
                A[i][i] = (uint32_t)(result & 0xFFFFFFFF);
            }
        }
    }
    
    // Return a value that depends on the computation
    return result ^ a ^ b;
}


uint64_t get_microseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

int main(int argc, char *argv[]) {
    uint64_t a = 0x12345678;
    uint64_t b = 0x9abcdef0;
    
    uint64_t start = get_microseconds();
    uint64_t result = matmul(a, b);
    uint64_t end = get_microseconds();
    printf("Elapsed time: %" PRIu64 " us\n", end - start);

    return 0;
}