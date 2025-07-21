#include <iostream>
#include <vector>
#include <list>
#include <cstdint>
#include "cache.hh"

Cache::Cache(uint64_t cache_size_in_bytes, int associativity, int block_size_in_bytes,
             int latency_hit, int latency_miss, bool first_access_is_hit)
    : associativity(associativity),
      block_size_in_bytes(block_size_in_bytes),
      latency_hit(latency_hit),
      latency_miss(latency_miss),
      first_access_is_hit(first_access_is_hit) {
        
    number_of_sets = cache_size_in_bytes / (associativity * block_size_in_bytes);
    cache_sets.resize(number_of_sets, std::vector<CacheLine>(associativity));
    // Initialize cache lines
    for (auto& set : cache_sets) {
        for (auto& line : set) {
            line.tag = 0;
            line.valid = 0;
            line.lru_counter = 0;
        }
    }
    
}


void Cache::update_lru_counters(uint64_t set_index, int accessed_way) {
    // Increase LRU counters
    for (int i = 0; i < associativity; ++i) {
        if (cache_sets[set_index][i].valid && i != accessed_way) {
            if (cache_sets[set_index][i].lru_counter < 127) {
                cache_sets[set_index][i].lru_counter++;
            }
        }
    }
    // Reset the LRU counter for the accessed way
    cache_sets[set_index][accessed_way].lru_counter = 0;
}

int Cache::select_victim_line(uint64_t set_index) {
    int victim_way = 0;
    uint8_t max_lru = 0;

    for (int i = 0; i < associativity; ++i) {
        if (!cache_sets[set_index][i].valid) {
            // Empty line found
            return i;
        }
        if (cache_sets[set_index][i].lru_counter > max_lru) {
            max_lru = cache_sets[set_index][i].lru_counter;
            victim_way = i;
        }
    }
    return victim_way;
}

int Cache::average_latency() {
        return total_latency / (total_accesses+1);
    }

float Cache::hit_rate() {
    return (float)total_hit / (total_accesses+1);
}

int Cache::simulate_cache_access(uint64_t address) {
    total_accesses++; 
    // Compute block address
    uint64_t block_address = address / block_size_in_bytes;

    // Compute set index
    uint64_t set_index = block_address % number_of_sets;

    // Compute tag
    uint64_t tag = block_address / number_of_sets;

    // Mask tag to fit into 42 bits
    tag &= ((1ULL << 42) - 1);

    // Get the set
    auto& cache_set = cache_sets[set_index];

    // Search for tag in the set
    for (int i = 0; i < associativity; ++i) {
        if (cache_set[i].valid && cache_set[i].tag == tag) {
            // Hit
            update_lru_counters(set_index, i);
            total_latency += latency_hit;
            total_hit++;
            return latency_hit;
        }
    }

    // Miss
    int victim_way = select_victim_line(set_index);

    // Check if first access into an empty cache line is treated as a hit
    if (first_access_is_hit && !cache_set[victim_way].valid) {
        cache_set[victim_way].tag = tag;
        cache_set[victim_way].valid = 1;
        update_lru_counters(set_index, victim_way);
        total_latency += latency_hit;
        total_hit++;
        return latency_hit;
    }

    // Replace victim line
    cache_set[victim_way].tag = tag;
    cache_set[victim_way].valid = 1;
    update_lru_counters(set_index, victim_way);
    total_latency += latency_miss+latency_hit;
    return latency_miss+latency_hit;
}
