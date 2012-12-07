#include <functional>

#include "logger.h"
#include "lt_common.h"

lt_selector::lt_selector(int s, int nb) {
    seed = s;
    num_blocks = nb;
    max_degree = num_blocks / 2 + 2;

    // Build the random engines
    generator.seed(seed);
    block_count_dist = new std::uniform_int_distribution<int>(1, max_degree);
    block_select_dist = new std::uniform_int_distribution<int>(0, num_blocks);

    glob_log.log(3, "Creating new lt_selector with seed %d, max degree %d, number of blocks %d\n",
            seed, max_degree, num_blocks);

    // Account for the message descriptor chunk, which effectively
    //   makes chunk ids 1-indexed
    block_list_cache.push_back(NULL);
    block_list_sizes.push_back(0);
}

lt_selector::~lt_selector() {
    delete block_count_dist;
    delete block_select_dist;

    for (unsigned i = 0; i < block_list_cache.size(); i++) {
        delete[] block_list_cache[i];
    }
}

/* Note that lt_selector owns the memory associated with out_block_list */
int lt_selector::select_blocks(unsigned int chunk_id, int **out_block_list) {
    glob_log.log(7, "Recieved select block request for chunk %u\n", chunk_id);
    glob_log.log(7, "Currently have the first %zu blocks\n", block_list_cache.size());
    if (block_list_cache.size() > chunk_id) {
        *out_block_list = block_list_cache[chunk_id];
        return block_list_sizes[chunk_id];
    }


    int num_blocks;
    int *selected_blocks = NULL;

    // Note that blc.size starts at 1, and chunks are 1-indexed
    for (unsigned int cid = block_list_cache.size(); cid <= chunk_id; cid++) {
        num_blocks = (*block_count_dist)(generator);
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

