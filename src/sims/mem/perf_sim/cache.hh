#ifndef CACHE_H
#define CACHE_H

#include <cstdint>
#include <vector>
#include <list>


class Cache {
public:
    Cache(uint64_t cache_size_in_bytes, int associativity, int block_size_in_bytes,
          int latency_hit, int latency_miss, bool first_access_is_hit);

    int simulate_cache_access(uint64_t address);
    int average_latency();
    float hit_rate();

private:
    struct CacheLine {
        uint64_t tag : 42;        // 42 bits for the tag
        uint64_t valid : 1;       // 1 bit for valid flag
        uint64_t lru_counter : 7; // 7 bits for LRU counter
        uint64_t : 14;            // Padding to make up 64 bits
    };

    uint64_t total_latency;
    uint64_t total_hit;
    uint64_t total_accesses;
    int associativity;
    int block_size_in_bytes;
    int latency_hit;
    int latency_miss;
    bool first_access_is_hit;
    uint64_t number_of_sets;

    // Each set is a vector of CacheLines
    std::vector<std::vector<CacheLine>> cache_sets;

    void update_lru_counters(uint64_t set_index, int accessed_way);
    int select_victim_line(uint64_t set_index);
};

extern Cache cache_sim;

#endif // CACHE_H
