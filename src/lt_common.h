#ifndef __LT_COMMON_H
#define __LT_COMMON_H

#include <stdlib.h>
#include <vector>
#include <random>

struct lt_descriptor {
    size_t total_blocks;
    size_t max_degree;
    size_t num_peers;
    size_t total_chunks;
    size_t chunk_len;
    unsigned int seed;
};

class lt_selector {
    public:
        lt_selector(int seed, int num_blocks);
        ~lt_selector();

        int select_blocks(unsigned int chunk_id, int **out_block_list);

    private:
        std::vector<int*>block_list_cache;
        std::vector<int> block_list_sizes;

        int seed;  // Seed for the random number generator
        int num_blocks;
        int max_degree;

        std::default_random_engine generator;
        std::uniform_int_distribution<int> *block_count_dist;
        std::uniform_int_distribution<int> *block_select_dist;
};


#endif /* __LT_COMMON_H */
