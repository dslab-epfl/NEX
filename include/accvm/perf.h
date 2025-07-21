#ifndef ACCVM_PERF_H
#define ACCVM_PERF_H
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


#define MAX_FUNC_NAME_LENGTH 100
// Assuming function names are lowercase a-z and _
#define ALPHABET_SIZE 27  

typedef struct func_counter {
    char func_name[MAX_FUNC_NAME_LENGTH];
    uint64_t total_time;
    int count;
} func_counter;

typedef struct trie_node {
    struct trie_node *child[ALPHABET_SIZE];
    int c_index;  // Index to the function counter array, -1 if not a leaf node
} trie_node;

extern func_counter func_counters[1000];
extern int func_counter_idx;
extern trie_node *root;

int perf_init();
int perf_deinit();
void perf_clear_ounters();
void perf_ins_func(const char *name);
void perf_incr_func_counter(const char *name);
void perf_print_func_counters();
void perf_incr_func_time(const char *name, uint64_t start, uint64_t end);

#endif