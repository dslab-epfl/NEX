#include "accvm/context.h"
#include <accvm/perf.h>

trie_node* create_trie_node();

int char_to_index(char c) {
    if (c == '_') {
        return 26;  // Assign index 26 to '_'
    } else {
        return c - 'a';  // Assign indices 0-25 to 'a'-'z'
    }
}

trie_node* create_trie_node() {
    trie_node *node = (trie_node*)malloc(sizeof(trie_node));
    if (node) {
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            node->child[i] = NULL;
        }
        node->c_index = -1;
    }
    return node;
}

void perf_ins_func(const char *name) {
    trie_node *crawl = root;
    const char* orig = name;
    while (*name) {
        int index = char_to_index(*name);
        if (!crawl->child[index]) {
            crawl->child[index] = create_trie_node();
        }
        crawl = crawl->child[index];
        name++;
    }
    // Check if the function is already indexed
    if (crawl->c_index == -1) {
        crawl->c_index = func_counter_idx;
        strncpy(func_counters[func_counter_idx].func_name, orig, MAX_FUNC_NAME_LENGTH);
        func_counters[func_counter_idx].count = 0;
        func_counter_idx++;
    }
}

// Function to increment the counter
void perf_incr_func_counter(const char *name) {
    trie_node *crawl = root;
    while (*name) {
        int index = char_to_index(*name);
        if (!crawl->child[index]) {
            return;  // Function not found
        }
        crawl = crawl->child[index];
        name++;
    }
    if (crawl->c_index != -1) {
        func_counters[crawl->c_index].count++;
    }
}

// Function to increment the total time
void perf_incr_func_time(const char *name, uint64_t start, uint64_t end) {
    trie_node *crawl = root;
    while (*name) {
        int index = char_to_index(*name);
        if (!crawl->child[index]) {
            return;  // Function not found
        }
        crawl = crawl->child[index];
        name++;
    }
    if (crawl->c_index != -1) {
        if (end >= start){
            func_counters[crawl->c_index].total_time += end-start;
        }
        else{
            printf("Thread<%d>End time %lu is less than start time %lu\n", self_ctx->tid, end, start);
            // assert(0);
        }
            
    }
}

// Printing all function counters
void perf_print_func_counters() {
    for (int i = 0; i < func_counter_idx; i++) {
        if (func_counters[i].count > 0)
            printf("%s has been called %d times; total time (virtual) in milli %lu \n", func_counters[i].func_name, func_counters[i].count, func_counters[i].total_time/1000000);
    }
}

void perf_clear_ounters() {
    for (int i = 0; i < func_counter_idx; i++) {
        func_counters[i].count = 0;
        func_counters[i].total_time = 0;
    }
}

int perf_init(){
    func_counter_idx = 0;
    root = create_trie_node();
    return 0;
}

void free_trie_node(trie_node *node) {
    if (node == NULL) {
        return;
    }
    for (int i = 0; i < 27; i++) {
        if (node->child[i] != NULL) {
            free_trie_node(node->child[i]);
        }
    }
    free(node);
}

int perf_deinit(){
    free_trie_node(root);
    return 0;
}