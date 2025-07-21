#include "exec/exec.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdint.h>
#include <config/config.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>

#define NEX_LOG_PATH CONFIG_PROJECT_PATH "/out/nex.log"

#define NEX_SPIN_LOCK
#ifdef NEX_SPIN_LOCK
static nex_lock_t nex_log_lock = { ATOMIC_FLAG_INIT };
#define LOG_LOCK nex_lock_acquire(&nex_log_lock)
#define LOG_UNLOCK nex_lock_release(&nex_log_lock)
#else
static pthread_mutex_t nex_log_lock = PTHREAD_MUTEX_INITIALIZER;
#define LOG_LOCK pthread_mutex_lock(&nex_log_lock)
#define LOG_UNLOCK pthread_mutex_unlock(&nex_log_lock)
#endif


// Global file descriptor for the log file.
// This should be initialized before entering any async‑sensitive context.
static int nex_log_fd = -1;


// Call this early in your program (outside of signal/ptrace event handlers)
static void init_nex_log() {
    // Open the log file in append mode, create it if it doesn't exist.
    // O_CLOEXEC can be added if desired.
    char* exp_name = getenv("NEX_EXP_NAME");
    if(exp_name != NULL) {
        char dir_path[200];
        sprintf(dir_path, "%s/out/%s", CONFIG_PROJECT_PATH, exp_name);
        mkdir(dir_path, 0777);
        char log_path[200];
        sprintf(log_path, "%s/out/%s/nex.log", CONFIG_PROJECT_PATH, exp_name);
        nex_log_fd = open(log_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }else{
        nex_log_fd = open(NEX_LOG_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }
    // Handle errors appropriately in your real code.
    if (nex_log_fd < 0) {
        exit(1);
    }
}

// Async‑signal‑safe function to write a string into nex_log_fd.
static void safe_write_str(const char *str) {
    while (*str) {
        if (write(nex_log_fd, str, 1) < 0) {
            return;
        }
        str++;
    }
}

// Write a single character
static void safe_write_char(char c) {
    if(write(nex_log_fd, &c, 1) < 0) {
        return;
    }
}

// Convert and write an int (base 10)
static void safe_write_int(int n) {
    char buf[32];
    int pos = 31;
    buf[pos] = '\0';
    int is_negative = 0;
    if (n < 0) {
        is_negative = 1;
        // Note: Handling INT_MIN properly would require special treatment.
        n = -n;
    }
    if (n == 0) {
        buf[--pos] = '0';
    } else {
        while (n > 0 && pos > 0) {
            buf[--pos] = '0' + (n % 10);
            n /= 10;
        }
    }
    if (is_negative && pos > 0) {
        buf[--pos] = '-';
    }
    safe_write_str(&buf[pos]);
}

// Convert and write a long int (base 10)
static void safe_write_long(long n) {
    char buf[64];
    int pos = 63;
    buf[pos] = '\0';
    int is_negative = 0;
    if (n < 0) {
        is_negative = 1;
        // Note: Handling LONG_MIN properly would require extra care.
        n = -n;
    }
    if (n == 0) {
        buf[--pos] = '0';
    } else {
        while (n > 0 && pos > 0) {
            buf[--pos] = '0' + (n % 10);
            n /= 10;
        }
    }
    if (is_negative && pos > 0) {
        buf[--pos] = '-';
    }
    safe_write_str(&buf[pos]);
}

// Convert and write an unsigned long int (base 10)
static void safe_write_ulong(unsigned long n) {
    char buf[64];
    int pos = 63;
    buf[pos] = '\0';
    if (n == 0) {
        buf[--pos] = '0';
    } else {
        while (n > 0 && pos > 0) {
            buf[--pos] = '0' + (n % 10);
            n /= 10;
        }
    }
    safe_write_str(&buf[pos]);
}

// Convert and write an unsigned long int in hexadecimal (for %p)
static void safe_write_hex(unsigned long n) {
    char buf[64];
    int pos = 63;
    buf[pos] = '\0';
    const char hex_chars[] = "0123456789abcdef";
    if (n == 0) {
        buf[--pos] = '0';
    } else {
        while (n > 0 && pos > 0) {
            buf[--pos] = hex_chars[n & 0xF];
            n >>= 4;
        }
    }
    safe_write_str("0x");
    safe_write_str(&buf[pos]);
}

// Convert and write a double in fixed-point notation with 6 digits precision.
static void safe_write_double(double d) {
    // Handle negative numbers.
    if (d < 0) {
        safe_write_char('-');
        d = -d;
    }

    // Get the integer part.
    long int_part = (long)d;
    safe_write_long(int_part);

    safe_write_char('.');

    // Get the fractional part.
    double frac = d - (double)int_part;
    // Scale the fractional part to 6 decimal places.
    long frac_int = (long)(frac * 1000000);

    // Ensure that leading zeros are printed.
    // For example, for 0.000123, frac_int would be 123 and we need "000123".
    int divisor = 100000;
    for (int i = 0; i < 6; i++) {
        int digit = (frac_int / divisor) % 10;
        safe_write_char('0' + digit);
        divisor /= 10;
    }
}

// Minimal safe_printf that supports %s, %d, %ld, %lu, %p, and now %f.
void safe_printf(const char *fmt, ...) {
    // return;
    LOG_LOCK;
    if(nex_log_fd < 0) {
        init_nex_log();
    }
    va_list args;
    va_start(args, fmt);
    for (const char *p = fmt; *p; p++) {
        if (*p == '%') {
            p++; // Move past '%'
            if (*p == 's') {
                char *str = va_arg(args, char *);
                safe_write_str(str);
            } else if (*p == 'd') {
                int d = va_arg(args, int);
                safe_write_int(d);
            } else if (*p == 'l') {
                p++; // Look at the next specifier
                if (*p == 'd') {
                    long ld = va_arg(args, long);
                    safe_write_long(ld);
                } else if (*p == 'u') {
                    unsigned long lu = va_arg(args, unsigned long);
                    safe_write_ulong(lu);
                } else {
                    // If unknown, print literally.
                    safe_write_char('%');
                    safe_write_char('l');
                    safe_write_char(*p);
                }
            } else if (*p == 'u') {
                // For %u without l, assume unsigned int.
                unsigned int u = va_arg(args, unsigned int);
                // We'll use safe_write_ulong for simplicity.
                safe_write_ulong(u);
            } else if (*p == 'p') {
                void *ptr = va_arg(args, void *);
                // Cast pointer to unsigned long for printing in hex.
                safe_write_hex((unsigned long)ptr);
            } else if (*p == 'f') {
                // Handle floating point numbers.
                double f = va_arg(args, double);
                safe_write_double(f);
            } else {
                // For any unsupported specifier, print it literally.
                safe_write_char('%');
                safe_write_char(*p);
            }
        } else {
            safe_write_char(*p);
        }
    }
    va_end(args);
    LOG_UNLOCK;
}
