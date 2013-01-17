#ifndef __LT_COMMON_H
#define __LT_COMMON_H

/*
 * Note: math for the distribution comes from
 * Joshi, Rhim, Sun and Wang (2010)
 * www.mit.edu/~dawang/materials/fountain_codes.pdf
 */

#include <stdlib.h>
#include <vector>
#include <random>

#include "enc/encoder.h"

struct LT_Descriptor {
    Message_Descriptor desc;
    size_t total_blocks;
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
        std::uniform_int_distribution<int> *rtab_dist;
        std::uniform_int_distribution<int> *block_select_dist;

        double delta;  // maximum allowable error(0.05)
        double c;      // performance constant (0.2)
        int k;      // number of blocks
        double beta;         // normalization constant
        int R;            // "ripple size", the number of chunks that
                             //   ought to rely on exactly one block
        int *robust_table;   // table to translate random numbers into a robust dist
        size_t rtab_size;    // resolution of said table

        double ideal_dist(int i);
        double extra_dist(int i);
        void setup_distribution();
};


#endif /* __LT_COMMON_H */
