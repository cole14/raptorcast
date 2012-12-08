#include <functional>
#include <math.h>

#include "logger.h"
#include "lt_common.h"

lt_selector::lt_selector(int s, int nb):
    seed(s),
    num_blocks(nb),
    max_degree(num_blocks / 2 + 1),
    delta(0.1),
    c(0.1),
    k(nb)
{

    // Build the random engines
    setup_distribution();


    glob_log.log(3, "Creating new lt_selector with seed %d, max degree %d, number of blocks %d\n",
            seed, max_degree, num_blocks);

    // Account for the message descriptor chunk, which effectively
    //   makes chunk ids 1-indexed
    block_list_cache.push_back(NULL);
    block_list_sizes.push_back(0);
}

lt_selector::~lt_selector() {
    delete rtab_dist;
    delete block_select_dist;
    delete[] robust_table;

    for (unsigned i = 0; i < block_list_cache.size(); i++) {
        delete[] block_list_cache[i];
    }
}

/*
 * The mathematically ideal distribution for i.  Performs
 * well in theory, but poorly in practice.  Serves as a
 * basis for the robust distribution
 */
double lt_selector::ideal_dist(int i) {
    if (i <= 0 || i > k)
        return 0;
    else if (i == 1)
        return 1.0 / k;
    else // 1 < i <= k
        return 1.0 / (i * (i - 1));
}

/*
 * The "extra" distribution, added to the ideal distribution
 * to form the robust distribution
 */
double lt_selector::extra_dist(int i) {
    if (i <= 0 || i > k)
        return 0;
    else if (i < R)
        return 1.0 * R / (i * k);
    else if (i == R)
        return R * log(1.0 * R / delta) / k;
    else // R < i
        return 0;
}

/*
 * Build the distribution of blocks per chunk.
 * This depends on k, so it must be done dynamically.
 */
void lt_selector::setup_distribution() {
    R = (int) (c * log(1.0 * k / delta) * sqrt(1.0 * k));
    // Build our normalization constant
    beta = 0;
    for (int i = 1; i <= k; i++) {
        beta += ideal_dist(i) + extra_dist(i);
    }

    int *rtp;
    rtab_size = 1024;
    robust_table = new int[rtab_size];
    rtp = robust_table;
    for (int i = 1; i <= k; i++) {
        int num_hits = (int) (1024 * (ideal_dist(i) + extra_dist(i)) / beta);
        for (int h = 0; h < num_hits ; h++) {
            *rtp = i;
            rtp++;
        }
    }
    for (; rtp < robust_table + rtab_size; rtp++) {
        *rtp = 1;
    }

    generator.seed(seed);
    rtab_dist = new std::uniform_int_distribution<int>(0, rtab_size);
    block_select_dist = new std::uniform_int_distribution<int>(0, num_blocks-1);
}


/* Note that lt_selector owns the memory associated with out_block_list */
int lt_selector::select_blocks(unsigned int chunk_id, int **out_block_list) {
    if (block_list_cache.size() > chunk_id) {
        *out_block_list = block_list_cache[chunk_id];
        return block_list_sizes[chunk_id];
    }

    int num_blocks;
    int *selected_blocks = NULL;

    // Note that blc.size starts at 1, and chunks are 1-indexed
    for (unsigned int cid = block_list_cache.size(); cid <= chunk_id; cid++) {
        num_blocks = robust_table[(*rtab_dist)(generator)];
        selected_blocks = new int[num_blocks];
        glob_log.log(5, "Generating block list for chunk %u: ", cid);
        for (int i = 0; i < num_blocks; i++) {
            bool valid = false;
            int block_id;
            while (!valid) {
                block_id = (*block_select_dist)(generator);
                valid = true;
                for (int j = 0; j < i; j++) {
                    if (selected_blocks[j] == block_id) {
                        valid = false;
                        break;
                    }
                }
            }
            glob_log.log(5, "%d ", block_id);
            selected_blocks[i] = block_id;
        }
        glob_log.log(5, "\n");
        block_list_cache.push_back(selected_blocks);
        block_list_sizes.push_back(num_blocks);
    }
    *out_block_list = block_list_cache[chunk_id];
    return block_list_sizes[chunk_id];
}

