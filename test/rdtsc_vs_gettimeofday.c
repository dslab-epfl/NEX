#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <inttypes.h>

static inline uint64_t rdtscp(uint32_t *aux)
{
    uint32_t lo, hi, auxv;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi), "=c"(auxv) ::);
    if (aux) *aux = auxv;
    return ((uint64_t)hi << 32) | (uint64_t)lo;
}

static inline uint64_t rdtsc(void)
{
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | (uint64_t)lo;
}

static inline uint64_t tv_to_us(const struct timeval *tv)
{
    return (uint64_t)tv->tv_sec * 1000000ULL + (uint64_t)tv->tv_usec;
}

uint64_t find_freq_khz(){
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/cpuinfo");
        return 0;
    }

    char line[256];
    uint64_t freq_khz = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "cpu MHz : %" SCNu64, &freq_khz) == 1) {
            freq_khz *= 1000; // Convert MHz to kHz
            break;
        }
    }

    fclose(fp);

    if (freq_khz == 0) {
        fprintf(stderr, "Failed to find CPU frequency in /proc/cpuinfo\n");
    }

    return freq_khz;
}


int main(void)
{

    printf("RDTSCP vs gettimeofday test\n");
    unsigned long long tsc_khz = find_freq_khz();

    printf("Using NEX_TSC_KHZ=%llu kHz\n", tsc_khz);

    for (int i = 0; i < 5; ++i) {
        struct timeval tv0, tv1;
        uint32_t aux0 = 0, aux1 = 0;

        // Measure deltas over ~10ms
        uint64_t c0 = rdtscp(&aux0);
        gettimeofday(&tv0, NULL);
        usleep(10000); // 10 ms
        uint64_t c1 = rdtscp(&aux1);
        gettimeofday(&tv1, NULL);

        uint64_t dcycles = c1 - c0;
        uint64_t dus_gtod = tv_to_us(&tv1) - tv_to_us(&tv0);

        // Convert cycles delta -> microseconds: us = cycles * 1000 / tsc_khz
        // Use 128-bit to avoid overflow on the multiply.
        __uint128_t prod = (__uint128_t)dcycles * 1000ULL;
        uint64_t dus_tsc = (uint64_t)(prod / tsc_khz);

        long long abs_err_us = (long long)(dus_tsc > dus_gtod ? (dus_tsc - dus_gtod) : (dus_gtod - dus_tsc));
        double rel_err = dus_gtod ? (100.0 * (double)abs_err_us / (double)dus_gtod) : 0.0;

        printf("Iter %d: dcycles=%llu, rdtsc≈%llu us, gettimeofday=%llu us, |err|=%lld us (%.3f%%), aux0=%u aux1=%u\n",
               i, (unsigned long long)dcycles,
               (unsigned long long)dus_tsc,
               (unsigned long long)dus_gtod,
               abs_err_us, rel_err,
               aux0, aux1);
    }

    return 0;
}
